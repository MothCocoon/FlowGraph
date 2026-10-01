// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphExporter.h"
#include "FlowLogChannels.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"
#include "Misc/FileHelper.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"
#include "EdGraph/EdGraphNode.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool UFlowGraphExporter::ExportFlowGraphToText(UFlowAsset* FlowAsset, const FString& OutputFilePath)
{
	if (!FlowAsset)
	{
		UE_LOG(LogFlow, Error, TEXT("FlowGraphExporter: FlowAsset is null"));
		return false;
	}

	const FString ExportText = ExportFlowGraphToString(FlowAsset);

	if (FFileHelper::SaveStringToFile(ExportText, *OutputFilePath))
	{
		UE_LOG(LogFlow, Log, TEXT("FlowGraphExporter: Successfully exported to %s"), *OutputFilePath);
		return true;
	}
	else
	{
		UE_LOG(LogFlow, Error, TEXT("FlowGraphExporter: Failed to write to file %s"), *OutputFilePath);
		return false;
	}
}

FString UFlowGraphExporter::ExportFlowGraphToString(UFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return TEXT("");
	}

	FFlowCourierDocument Document;
	BuildDocumentHeader(FlowAsset, Document);
	BuildDocumentNodes(FlowAsset, Document);
	BuildDocumentConnections(FlowAsset, Document);

	return SerializeDocumentToJson(Document);
}

FString UFlowGraphExporter::ExportFlowGraphSubsetToString(UFlowAsset* FlowAsset, const TSet<FGuid>& NodeGuidFilter)
{
	if (!FlowAsset)
	{
		return TEXT("");
	}

	FFlowCourierDocument Document;
	BuildDocumentHeader(FlowAsset, Document);
	BuildDocumentNodes(FlowAsset, Document, &NodeGuidFilter);
	BuildDocumentConnections(FlowAsset, Document, &NodeGuidFilter);

	return SerializeDocumentToJson(Document);
}

void UFlowGraphExporter::BuildDocumentHeader(const UFlowAsset* FlowAsset, FFlowCourierDocument& OutDocument)
{
	OutDocument.FormatVersion = 2;
	OutDocument.Mode = EFlowCourierMode::Full;
	OutDocument.AssetClass = FlowAsset->GetClass()->GetPathName();
	OutDocument.bWorldBound = FlowAsset->bWorldBound;

	const UClass* ExpectedOwnerClass = FlowAsset->GetExpectedOwnerClass();
	OutDocument.ExpectedOwnerClass = ExpectedOwnerClass ? ExpectedOwnerClass->GetName() : FString();
}

void UFlowGraphExporter::BuildDocumentNodes(const UFlowAsset* FlowAsset, FFlowCourierDocument& OutDocument, const TSet<FGuid>* NodeGuidFilter)
{
	const TMap<FGuid, UFlowNode*>& Nodes = FlowAsset->GetNodes();

	// Deterministic ordering (sorted by GUID string) so exports/diffs are stable across runs,
	// independent of TMap iteration order.
	TArray<FGuid> SortedNodeGuids;
	Nodes.GenerateKeyArray(SortedNodeGuids);
	SortedNodeGuids.Sort([](const FGuid& A, const FGuid& B) { return A.ToString() < B.ToString(); });

	for (const FGuid& NodeGuid : SortedNodeGuids)
	{
		if (NodeGuidFilter && !NodeGuidFilter->Contains(NodeGuid))
		{
			continue;
		}

		UFlowNode* Node = Nodes.FindRef(NodeGuid);
		if (!Node)
		{
			continue;
		}

		FFlowCourierOp& Op = OutDocument.Ops.AddDefaulted_GetRef();
		Op.Kind = EFlowCourierOpKind::UpsertNode;
		Op.Guid = NodeGuid.ToString();
		Op.Type = Node->GetClass()->GetPathName();
		// A full export is authoritative over every node's addon list - BuildAddonOps below
		// appends the complete set for this node.
		Op.bReplaceAddons = true;

		#if WITH_EDITOR
		if (const UEdGraphNode* EdGraphNode = Node->GetGraphNode())
		{
			Op.bHasPosition = true;
			Op.Position = FIntPoint(EdGraphNode->NodePosX, EdGraphNode->NodePosY);
			Op.Comment = EdGraphNode->NodeComment;
		}
		#endif

		const UFlowNode* DefaultNode = Cast<UFlowNode>(Node->GetClass()->GetDefaultObject());

		for (TFieldIterator<FProperty> PropIt(Node->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			// Only editable state is authored content; generated/runtime state (e.g. a pin
			// definition map with no EditAnywhere) is rebuilt by the node itself, not carried
			// in the document. See agent-docs/CourierTextFormat.md.
			if (!Property || Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated) || !Property->HasAnyPropertyFlags(CPF_Edit))
			{
				continue;
			}

			if (Property->GetName() == TEXT("NodeGuid") ||
				Property->GetName() == TEXT("InputPins") ||
				Property->GetName() == TEXT("OutputPins") ||
				Property->GetName() == TEXT("Connections"))
			{
				continue;
			}

			// Skip properties defined on base classes (UFlowNode or UFlowNodeBase)
			UClass* PropertyOwnerClass = Property->GetOwnerClass();
			if (PropertyOwnerClass == UFlowNode::StaticClass() || PropertyOwnerClass == UFlowNodeBase::StaticClass())
			{
				continue;
			}

			const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Node);
			const void* DefaultValuePtr = DefaultNode ? Property->ContainerPtrToValuePtr<void>(DefaultNode) : nullptr;

			if (DefaultValuePtr && Property->Identical(ValuePtr, DefaultValuePtr))
			{
				continue;
			}

			FString ValueString = GetPropertyValueAsString(Property, ValuePtr);
			if (!ValueString.IsEmpty())
			{
				Op.Properties.Add(Property->GetName(), ValueString);
			}
		}

		for (const FFlowPin& Pin : Node->GetInputPins())
		{
			FFlowCourierPin& CourierPin = Op.InputPins.AddDefaulted_GetRef();
			CourierPin.Name = Pin.PinName.ToString();
			CourierPin.Type = GetPinTypeName(Pin);
			CourierPin.SubCategoryPath = GetPinSubCategoryPath(Pin);
		}

		for (const FFlowPin& Pin : Node->GetOutputPins())
		{
			FFlowCourierPin& CourierPin = Op.OutputPins.AddDefaulted_GetRef();
			CourierPin.Name = Pin.PinName.ToString();
			CourierPin.Type = GetPinTypeName(Pin);
			CourierPin.SubCategoryPath = GetPinSubCategoryPath(Pin);
		}

		BuildAddonOps(Node, NodeGuid, OutDocument);
	}
}

void UFlowGraphExporter::BuildAddonOps(const UFlowNodeBase* OwnerNode, const FGuid& OwnerGuid, FFlowCourierDocument& OutDocument)
{
	const TArray<UFlowNodeAddOn*>& NodeAddOns = OwnerNode->GetFlowNodeAddOnChildren();

	for (const UFlowNodeAddOn* AddOn : NodeAddOns)
	{
		if (!AddOn)
		{
			continue;
		}

		FFlowCourierOp& Op = OutDocument.Ops.AddDefaulted_GetRef();
		Op.Kind = EFlowCourierOpKind::UpsertAddon;
		Op.Guid = AddOn->GetGuid().ToString();
		Op.ParentGuid = OwnerGuid.ToString();
		Op.Type = AddOn->GetClass()->GetPathName();

		const UFlowNodeAddOn* DefaultAddOn = Cast<UFlowNodeAddOn>(AddOn->GetClass()->GetDefaultObject());

		for (TFieldIterator<FProperty> PropIt(AddOn->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			if (!Property || Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated) || !Property->HasAnyPropertyFlags(CPF_Edit))
			{
				continue;
			}

			// Skip base-class infrastructure properties
			UClass* OwnerClass = Property->GetOwnerClass();
			if (OwnerClass == UFlowNodeBase::StaticClass() || OwnerClass == UFlowNodeAddOn::StaticClass())
			{
				continue;
			}

			const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(AddOn);
			const void* DefaultValuePtr = DefaultAddOn ? Property->ContainerPtrToValuePtr<void>(DefaultAddOn) : nullptr;

			if (DefaultValuePtr && Property->Identical(ValuePtr, DefaultValuePtr))
			{
				continue;
			}

			FString ValueString = GetPropertyValueAsString(Property, ValuePtr);
			if (!ValueString.IsEmpty())
			{
				Op.Properties.Add(Property->GetName(), ValueString);
			}
		}

		// Recurse before continuing the sibling loop so an addon-of-addon is parented to its
		// immediate addon owner (ParentGuid = AddOn's own GUID), not the top-level node.
		BuildAddonOps(AddOn, AddOn->GetGuid(), OutDocument);
	}
}

void UFlowGraphExporter::BuildDocumentConnections(const UFlowAsset* FlowAsset, FFlowCourierDocument& OutDocument, const TSet<FGuid>* NodeGuidFilter)
{
	const TMap<FGuid, UFlowNode*>& Nodes = FlowAsset->GetNodes();

	struct FExportedConnection
	{
		FString SourceGuid;
		FString SourcePin;
		FString TargetGuid;
		FString TargetPin;
	};

	// Collect all connections first, then sort deterministically so output is stable regardless
	// of TMap iteration order.
	TArray<FExportedConnection> Connections;

	for (const TPair<FGuid, UFlowNode*>& NodePair : Nodes)
	{
		const FGuid& NodeGuid = NodePair.Key;
		UFlowNode* Node = NodePair.Value;

		if (!Node || (NodeGuidFilter && !NodeGuidFilter->Contains(NodeGuid)))
		{
			continue;
		}

		// Exec output pin connections (stored on source node with output pin name as key)
		for (const FFlowPin& OutputPin : Node->GetOutputPins())
		{
			if (!OutputPin.IsExecPin())
			{
				continue;
			}

			FConnectedPin Connection = Node->GetConnection(OutputPin.PinName);
			if (Connection.NodeGuid.IsValid() && (!NodeGuidFilter || NodeGuidFilter->Contains(Connection.NodeGuid)))
			{
				Connections.Add({ NodeGuid.ToString(), OutputPin.PinName.ToString(), Connection.NodeGuid.ToString(), Connection.PinName.ToString() });
			}
		}

		// Data input pin connections (stored on the receiving node, keyed by input pin name).
		// Direction is reversed from storage: SourceNode.SourcePin -> ThisNode.InputPin.
		for (const FFlowPin& InputPin : Node->GetInputPins())
		{
			if (InputPin.IsExecPin())
			{
				continue;
			}

			FConnectedPin Connection = Node->GetConnection(InputPin.PinName);
			if (Connection.NodeGuid.IsValid() && (!NodeGuidFilter || NodeGuidFilter->Contains(Connection.NodeGuid)))
			{
				Connections.Add({ Connection.NodeGuid.ToString(), Connection.PinName.ToString(), NodeGuid.ToString(), InputPin.PinName.ToString() });
			}
		}
	}

	Connections.Sort([](const FExportedConnection& A, const FExportedConnection& B)
	{
		if (A.SourceGuid != B.SourceGuid) return A.SourceGuid < B.SourceGuid;
		if (A.SourcePin != B.SourcePin) return A.SourcePin < B.SourcePin;
		if (A.TargetGuid != B.TargetGuid) return A.TargetGuid < B.TargetGuid;
		return A.TargetPin < B.TargetPin;
	});

	for (const FExportedConnection& Connection : Connections)
	{
		FFlowCourierOp& Op = OutDocument.Ops.AddDefaulted_GetRef();
		Op.Kind = EFlowCourierOpKind::AddConnection;
		Op.Source.NodeGuid = Connection.SourceGuid;
		Op.Source.Pin = Connection.SourcePin;
		Op.Target.NodeGuid = Connection.TargetGuid;
		Op.Target.Pin = Connection.TargetPin;
	}
}

FString UFlowGraphExporter::SerializeDocumentToJson(const FFlowCourierDocument& Document)
{

	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	if (!FJsonObjectConverter::UStructToJsonObject(FFlowCourierDocument::StaticStruct(), &Document, JsonObject, 0, 0))
	{
		return TEXT("");
	}

	// FJsonObjectConverter omits empty default arrays. Courier documents must still carry an explicit
	// ops array when exporting an empty asset so the result remains a valid complete document.
	if (!JsonObject->HasField(TEXT("ops")))
	{
		JsonObject->SetArrayField(TEXT("ops"), TArray<TSharedPtr<FJsonValue>>());
	}

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject, Writer);
	JsonString.ReplaceInline(TEXT("\"ops\":[]"), TEXT("\"ops\": []"));
	return JsonString;
}

// True when Object is a subobject this property owns rather than a shared asset it merely points
// at. Ownership is what makes the bare path unusable in an exported document, so that - not the
// property flags alone - is the test.
bool UFlowGraphExporter::IsInstancedSubobjectValue(const FObjectProperty& ObjectProperty, const UObject& Object)
{
	if (Object.IsAsset() || Object.HasAnyFlags(RF_ClassDefaultObject))
	{
		return false;
	}

	const bool bDeclaredInstanced = ObjectProperty.HasAnyPropertyFlags(CPF_InstancedReference | CPF_ExportObject)
		|| (ObjectProperty.PropertyClass && ObjectProperty.PropertyClass->HasAnyClassFlags(CLASS_EditInlineNew));

	return bDeclaredInstanced && Object.GetOuter() != nullptr && !Object.GetOuter()->IsA<UPackage>();
}

// Emits "ClassPath(Field=Value,...)" for an owned subobject, listing only the properties that
// differ from the class default so the document stays as small as the rest of the Courier grammar.
FString UFlowGraphExporter::ExportInstancedSubobject(const UObject& Object)
{
	const UClass* Class = Object.GetClass();
	const UObject* Defaults = Class->GetDefaultObject();

	TArray<FString> Fields;
	for (TFieldIterator<FProperty> PropertyIterator(Class); PropertyIterator; ++PropertyIterator)
	{
		FProperty* Property = *PropertyIterator;
		if (!Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_Transient | CPF_EditConst))
		{
			continue;
		}

		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(&Object);
		const void* DefaultPtr = Property->ContainerPtrToValuePtr<void>(Defaults);
		if (Property->Identical(ValuePtr, DefaultPtr))
		{
			continue;
		}

		Fields.Add(FString::Printf(TEXT("%s=%s"),
			*Property->GetName(), *GetPropertyValueAsString(Property, ValuePtr)));
	}

	return FString::Printf(TEXT("%s(%s)"), *Class->GetPathName(), *FString::Join(Fields, TEXT(",")));
}

FString UFlowGraphExporter::GetPropertyValueAsString(const FProperty* Property, const void* ValuePtr)
{
	if (!Property || !ValuePtr)
	{
		return TEXT("");
	}

	if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		return BoolProp->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false");
	}
	else if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		return FString::FromInt(IntProp->GetPropertyValue(ValuePtr));
	}
	else if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		return FString::SanitizeFloat(FloatProp->GetPropertyValue(ValuePtr));
	}
	else if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		// Escape embedded quotes/backslashes/newlines so the value round-trips through a JSON
		// string cleanly rather than producing invalid JSON or a silently truncated value.
		return FString::Printf(TEXT("\"%s\""), *StrProp->GetPropertyValue(ValuePtr).ReplaceCharWithEscapedChar());
	}
	else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		return NameProp->GetPropertyValue(ValuePtr).ToString();
	}
	else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		return FString::Printf(TEXT("\"%s\""), *TextProp->GetPropertyValue(ValuePtr).ToString().ReplaceCharWithEscapedChar());
	}
	else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		const UEnum* EnumType = EnumProp->GetEnum();
		if (EnumType)
		{
			int64 EnumValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			return EnumType->GetNameStringByValue(EnumValue);
		}
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (ByteProp->Enum)
		{
			uint8 ByteValue = ByteProp->GetPropertyValue(ValuePtr);
			return ByteProp->Enum->GetNameStringByValue(ByteValue);
		}
		else
		{
			return FString::FromInt(ByteProp->GetPropertyValue(ValuePtr));
		}
	}
	else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		UObject* Object = ObjProp->GetObjectPropertyValue(ValuePtr);
		if (Object)
		{
			// An owned subobject's path points inside its owning asset, so exporting the bare path
			// produces a document that cannot be imported anywhere else - the reference resolves to
			// the source asset's private object, or to nothing. Emit the class-plus-fields form the
			// importer understands so a full export round-trips into a different asset.
			if (IsInstancedSubobjectValue(*ObjProp, *Object))
			{
				return ExportInstancedSubobject(*Object);
			}

			return Object->GetPathName();
		}
		return TEXT("None");
	}
	else if (const FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		UClass* Class = Cast<UClass>(ClassProp->GetObjectPropertyValue(ValuePtr));
		if (Class)
		{
			return Class->GetPathName();
		}
		return TEXT("None");
	}
	else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		FString StructString;
		StructProp->Struct->ExportText(StructString, ValuePtr, ValuePtr, nullptr, PPF_None, nullptr);
		return StructString;
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		// Use Unreal's native ExportText for arrays to get proper format: (elem1,elem2,elem3)
		// This handles nested structs and TInstancedStruct correctly
		FString ArrayString;
		Property->ExportTextItem_Direct(ArrayString, ValuePtr, ValuePtr, nullptr, PPF_None);
		return ArrayString;
	}

	// For any other property types, use the generic ExportText
	FString ExportedText;
	Property->ExportTextItem_Direct(ExportedText, ValuePtr, ValuePtr, nullptr, PPF_None);
	return ExportedText;
}

FString UFlowGraphExporter::GetPinTypeName(const FFlowPin& Pin)
{
	return Pin.GetPinTypeName().ToString();
}

FString UFlowGraphExporter::GetPinSubCategoryPath(const FFlowPin& Pin)
{
	const UObject* SubCatObject = Pin.GetPinSubCategoryObject().Get();
	return SubCatObject ? SubCatObject->GetPathName() : FString();
}
