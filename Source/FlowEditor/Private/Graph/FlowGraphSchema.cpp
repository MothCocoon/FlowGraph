// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphSchema.h"

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "FlowEditorLogChannels.h"
#include "FlowSettings.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Nodes/Route/FlowNode_Reroute.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/MemberReference.h"
#include "Kismet/BlueprintTypeConversions.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphSchema)

#define LOCTEXT_NAMESPACE "FlowGraphSchema"

bool UFlowGraphSchema::bInitialGatherPerformed = false;
TArray<UClass*> UFlowGraphSchema::NativeFlowNodes;
TArray<UClass*> UFlowGraphSchema::NativeFlowNodeAddOns;
TMap<FName, FAssetData> UFlowGraphSchema::BlueprintFlowNodes;
TMap<FName, FAssetData> UFlowGraphSchema::BlueprintFlowNodeAddOns;
TMap<TSubclassOf<UFlowNodeBase>, TSubclassOf<UEdGraphNode>> UFlowGraphSchema::GraphNodesByFlowNodes;

bool UFlowGraphSchema::bBlueprintCompilationPending;
int32 UFlowGraphSchema::CurrentCacheRefreshID = 0;

FFlowGraphSchemaRefresh UFlowGraphSchema::OnNodeListChanged;

const UScriptStruct* UFlowGraphSchema::VectorStruct = nullptr;
const UScriptStruct* UFlowGraphSchema::RotatorStruct = nullptr;
const UScriptStruct* UFlowGraphSchema::TransformStruct = nullptr;

namespace FlowGraphSchema::Private
{
	// Adapted from UE::EdGraphSchemaK2::Private, because it's Private
	
	template <class... T>
	constexpr bool TAlwaysFalse = false;

	template <typename TProperty>
	UClass* GetAuthoritativeClass(const TProperty& Property)
	{
		UClass* PropertyClass = nullptr;
		if constexpr (std::is_same_v<TProperty, FObjectPropertyBase>)
		{
			PropertyClass = Property.PropertyClass;
		}
		else if constexpr (std::is_same_v<TProperty, FSoftObjectProperty>)
		{
			PropertyClass = Property.PropertyClass;
		}
		else if constexpr (std::is_same_v<TProperty, FInterfaceProperty>)
		{
			PropertyClass = Property.InterfaceClass;
		}
		else if constexpr (std::is_same_v<TProperty, FClassProperty>)
		{
			PropertyClass = Property.MetaClass;
		}
		else if constexpr (std::is_same_v<TProperty, FSoftClassProperty>)
		{
			PropertyClass = Property.MetaClass;
		}
		else
		{
			static_assert(TAlwaysFalse<TProperty>, "Invalid property used.");
		}

		if (PropertyClass && PropertyClass->ClassGeneratedBy)
		{
			PropertyClass = PropertyClass->GetAuthoritativeClass();
		}

		if (PropertyClass && FKismetEditorUtilities::IsClassABlueprintSkeleton(PropertyClass))
		{
			UE_LOG(LogFlowEditor, Warning, TEXT("'%s' is a skeleton class. SubCategoryObject will serialize to a null value."), *PropertyClass->GetFullName());
		}

		return PropertyClass;
	}

	static UClass* GetOriginalClassToFixCompatibility(const UClass* InClass)
	{
		const UBlueprint* BP = InClass ? Cast<const UBlueprint>(InClass->ClassGeneratedBy) : nullptr;
		return BP ? BP->OriginalClass : nullptr;
	}

	// During compilation, pins are moved around for node expansion and the Blueprints may still inherit from REINST_ classes
	// which causes problems for IsChildOf. Because we do not want to modify IsChildOf we must use a separate function
	// that can check to see if classes have an AuthoritativeClass that IsChildOf a Target class.
	static bool IsAuthoritativeChildOf(const UStruct* InSourceStruct, const UStruct* InTargetStruct)
	{
		bool bResult = false;
		bool bIsNonNativeClass = false;
		if (const UClass* TargetAsClass = Cast<const UClass>(InTargetStruct))
		{
			InTargetStruct = TargetAsClass->GetAuthoritativeClass();
		}
		if (UClass* SourceAsClass = const_cast<UClass*>(Cast<UClass>(InSourceStruct)))
		{
			if (SourceAsClass->ClassGeneratedBy)
			{
				// We have a non-native (Blueprint) class which means it can exist in a semi-compiled state and inherit from a REINST_ class.
				bIsNonNativeClass = true;
				while (SourceAsClass)
				{
					if (SourceAsClass->GetAuthoritativeClass() == InTargetStruct)
					{
						bResult = true;
						break;
					}
					SourceAsClass = SourceAsClass->GetSuperClass();
				}
			}
		}

		// We have a native (C++) class, do a normal IsChildOf check
		if (!bIsNonNativeClass)
		{
			bResult = InSourceStruct && InSourceStruct->IsChildOf(InTargetStruct);
		}

		return bResult;
	}

	static bool ExtendedIsChildOf(const UClass* Child, const UClass* Parent)
	{
		if (Child && Child->IsChildOf(Parent))
		{
			return true;
		}

		const UClass* OriginalChild = GetOriginalClassToFixCompatibility(Child);
		if (OriginalChild && OriginalChild->IsChildOf(Parent))
		{
			return true;
		}

		const UClass* OriginalParent = GetOriginalClassToFixCompatibility(Parent);
		if (OriginalParent && Child && Child->IsChildOf(OriginalParent))
		{
			return true;
		}

		return false;
	}

	static bool ExtendedImplementsInterface(const UClass* Class, const UClass* Interface)
	{
		if (Class->ImplementsInterface(Interface))
		{
			return true;
		}

		const UClass* OriginalClass = GetOriginalClassToFixCompatibility(Class);
		if (OriginalClass && OriginalClass->ImplementsInterface(Interface))
		{
			return true;
		}

		const UClass* OriginalInterface = GetOriginalClassToFixCompatibility(Interface);
		if (OriginalInterface && Class->ImplementsInterface(OriginalInterface))
		{
			return true;
		}

		return false;
	}
}

UFlowGraphSchema::UFlowGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		GetMutableDefault<UFlowSettings>()->OnAdaptiveNodeTitlesChanged.BindLambda([]()
		{
			GetDefault<UFlowGraphSchema>()->ForceVisualizationCacheClear();
		});
	}

	// Initialize cached static references to well-known struct types
	if (VectorStruct == nullptr)
	{
		VectorStruct = TBaseStructure<FVector>::Get();
		RotatorStruct = TBaseStructure<FRotator>::Get();
		TransformStruct = TBaseStructure<FTransform>::Get();
	}
}

void UFlowGraphSchema::SubscribeToAssetChanges()
{
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistry.Get().OnFilesLoaded().AddStatic(&UFlowGraphSchema::GatherNodes);
	AssetRegistry.Get().OnAssetAdded().AddStatic(&UFlowGraphSchema::OnAssetAdded);
	AssetRegistry.Get().OnAssetRemoved().AddStatic(&UFlowGraphSchema::OnAssetRemoved);
	AssetRegistry.Get().OnAssetRenamed().AddStatic(&UFlowGraphSchema::OnAssetRenamed);

	FCoreUObjectDelegates::ReloadCompleteDelegate.AddStatic(&UFlowGraphSchema::OnHotReload);

	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().AddStatic(&UFlowGraphSchema::OnBlueprintPreCompile);
		GEditor->OnBlueprintCompiled().AddStatic(&UFlowGraphSchema::OnBlueprintCompiled);
	}
}

void UFlowGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UFlowAsset* EditedFlowAsset, const FString& CategoryName)
{
	GetFlowNodeActions(ActionMenuBuilder, EditedFlowAsset, CategoryName);
	GetCommentAction(ActionMenuBuilder);
}

void UFlowGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetFlowNodeActions(ContextMenuBuilder, GetEditedAssetOrClassDefault(ContextMenuBuilder.CurrentGraph), FString());
	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	if (!ContextMenuBuilder.FromPin && FFlowGraphUtils::GetFlowGraphEditor(ContextMenuBuilder.CurrentGraph)->CanPasteNodes())
	{
		const TSharedPtr<FFlowGraphSchemaAction_Paste> NewAction(new FFlowGraphSchemaAction_Paste(FText::GetEmpty(), LOCTEXT("PasteHereAction", "Paste here"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(NewAction);
	}
}

void UFlowGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	const UFlowAsset* AssetClassDefaults = GetEditedAssetOrClassDefault(&Graph);
	static const FVector2D NodeOffsetIncrement = FVector2D(0, 128);
	FVector2D NodeOffset = FVector2D::ZeroVector;

	// Start node
	CreateDefaultNode(Graph, UFlowNode_Start::StaticClass(), NodeOffset, AssetClassDefaults->bStartNodePlacedAsGhostNode);

	// Add default nodes for all the CustomInputs
	if (IsValid(AssetClassDefaults))
	{
		for (const FName& CustomInputName : AssetClassDefaults->CustomInputs)
		{
			NodeOffset += NodeOffsetIncrement;
			const UFlowGraphNode* NewFlowGraphNode = CreateDefaultNode(Graph, UFlowNode_CustomInput::StaticClass(), NodeOffset, true);

			UFlowNode_CustomInput* CustomInputNode = CastChecked<UFlowNode_CustomInput>(NewFlowGraphNode->GetFlowNodeBase());
			CustomInputNode->SetEventName(CustomInputName);
		}
	}

	UFlowAsset* FlowAsset = CastChecked<UFlowGraph>(&Graph)->GetFlowAsset();
	FlowAsset->HarvestNodeConnections();
}

UFlowGraphNode* UFlowGraphSchema::CreateDefaultNode(UEdGraph& Graph, const TSubclassOf<UFlowNode>& NodeClass, const FVector2D& Offset, const bool bPlacedAsGhostNode)
{
	UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(&Graph, nullptr, NodeClass, Offset);
	SetNodeMetaData(NewGraphNode, FNodeMetadata::DefaultGraphNode);

	if (bPlacedAsGhostNode)
	{
		NewGraphNode->MakeAutomaticallyPlacedGhostNode();
	}

	return NewGraphNode;
}

bool UFlowGraphSchema::ArePinsCompatible(const UEdGraphPin* PinA, const UEdGraphPin* PinB, const UClass* CallingContext, bool bIgnoreArray /*= false*/) const
{
	// Adapted from UEdGraphSchema_K2
	if ((PinA->Direction == EGPD_Input) && (PinB->Direction == EGPD_Output))
	{
		return ArePinTypesCompatible(PinB->PinType, PinA->PinType, CallingContext, bIgnoreArray);
	}
	else if ((PinB->Direction == EGPD_Input) && (PinA->Direction == EGPD_Output))
	{
		return ArePinTypesCompatible(PinA->PinType, PinB->PinType, CallingContext, bIgnoreArray);
	}
	else
	{
		return false;
	}
}

bool UFlowGraphSchema::ArePinCategoriesEffectivelyMatching(const FName& InputPinCategory, const FName& OutputPinCategory, bool bAllowImplicitCasts)
{
	if (InputPinCategory == OutputPinCategory)
	{
		return true;
	}

	if (!bAllowImplicitCasts)
	{
		return false;
	}

	// Must handle pin connectivity for all added EFlowPinTypes
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// We could extend the compatibility here to accept more implicit conversions (eg, null objects convertible to bools)
	// but we'd need to also add support the conversion in the Supply/Resolve side as well.

	if (FFlowPin::IsBoolPinCategory(InputPinCategory) && FFlowPin::IsConvertableToBoolPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsIntPinCategory(InputPinCategory) && FFlowPin::IsConvertableToIntPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsFloatPinCategory(InputPinCategory) && FFlowPin::IsConvertableToFloatPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsEnumPinCategory(InputPinCategory) && FFlowPin::IsConvertableToEnumPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsTextPinCategory(InputPinCategory) && FFlowPin::IsConvertableToTextPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsObjectPinCategory(InputPinCategory) && FFlowPin::IsConvertableToObjectPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsClassPinCategory(InputPinCategory) && FFlowPin::IsConvertableToClassPinCategory(OutputPinCategory))
	{
		return true;
	}

	if (FFlowPin::IsStructPinCategory(InputPinCategory) && FFlowPin::IsConvertableToStructPinCategory(OutputPinCategory))
	{
		return true;
	}

	return false;
}

bool UFlowGraphSchema::ArePinTypesCompatible(const FEdGraphPinType& Output, const FEdGraphPinType& Input, const UClass* CallingContext, bool bIgnoreArray /*= false*/) const
{
	// NOTE - Adapted from UEdGraphSchema_K2::ArePinTypesCompatible()

	using namespace FlowGraphSchema::Private;
	using namespace UE::Kismet::BlueprintTypeConversions;

	if (ArePinCategoriesEffectivelyMatching(Input.PinCategory, Output.PinCategory))
	{
		const UScriptStruct* OutputStruct = Cast<UScriptStruct>(Output.PinSubCategoryObject.Get());
		const UScriptStruct* InputStruct = Cast<UScriptStruct>(Input.PinSubCategoryObject.Get());
		if (OutputStruct != InputStruct)
		{
			const bool bAreConvertibleStructs =
				FStructConversionTable::Get().GetConversionFunction(OutputStruct, InputStruct).IsSet();

			if (bAreConvertibleStructs)
			{
				return true;
			}
		}

		if ((Output.PinSubCategory == Input.PinSubCategory)
			&& (Output.PinSubCategoryObject == Input.PinSubCategoryObject)
			&& (Output.PinSubCategoryMemberReference == Input.PinSubCategoryMemberReference))
		{
			// If the sub-category also matches exactly, then the pins are compatible
			return true;
		}
		
		if ((Output.PinCategory == FFlowPin::PC_Object) || (Output.PinCategory == FFlowPin::PC_Struct) || (Output.PinCategory == FFlowPin::PC_Class))
		{
			// Subcategory mismatch, but the two could be castable
			// Only allow a match if the input is a superclass of the output

			UStruct const* OutputObject = (Output.PinSubCategory == UEdGraphSchema_K2::PSC_Self) ? CallingContext : Cast<UStruct>(Output.PinSubCategoryObject.Get());
			UStruct const* InputObject = (Input.PinSubCategory == UEdGraphSchema_K2::PSC_Self) ? CallingContext : Cast<UStruct>(Input.PinSubCategoryObject.Get());

			if (OutputObject && InputObject)
			{
				if (Output.PinCategory == FFlowPin::PC_Struct)
				{
					return OutputObject->IsChildOf(InputObject) && FStructUtils::TheSameLayout(OutputObject, InputObject);
				}

				UClass const* OutputClass = Cast<const UClass>(OutputObject);
				UClass const* InputClass = Cast<const UClass>(InputObject);

				return 
					(IsAuthoritativeChildOf(OutputObject, InputObject) || 
					(OutputClass && InputClass && ExtendedIsChildOf(OutputClass, InputClass)));
			}

			return false;
		}
		
		return false;
	}

	return false;
}

const FPinConnectionResponse UFlowGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	const UFlowGraphNode* OwningNodeA = Cast<UFlowGraphNode>(PinA->GetOwningNodeUnchecked());
	const UFlowGraphNode* OwningNodeB = Cast<UFlowGraphNode>(PinB->GetOwningNodeUnchecked());

	if (!OwningNodeA || !OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid nodes"));
	}

	// Make sure the pins are not on the same node
	if (OwningNodeA == OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	if (PinA->bOrphanedPin || PinB->bOrphanedPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot make new connections to orphaned pin"));
	}

	FString NodeResponseMessage;

	// node can disallow the connection
	if (OwningNodeA && OwningNodeA->IsConnectionDisallowed(PinA, PinB, NodeResponseMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NodeResponseMessage);
	}
	if (OwningNodeB && OwningNodeB->IsConnectionDisallowed(PinB, PinA, NodeResponseMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, NodeResponseMessage);
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions are not compatible"));
	}

	check(InputPin);
	check(OutputPin);

	// Use the owning flow node's class as the CallingContext
	constexpr bool bIgnoreArray = false;
	UClass* CallingContext = nullptr;
	if (OwningNodeA)
	{
		UFlowNodeBase* FlowNodeBase = OwningNodeA->GetFlowNodeBase();
		if (FlowNodeBase)
		{
			CallingContext = FlowNodeBase->GetClass();
		}
	}

	// Compare the pin types
	const bool bArePinsCompatible = ArePinsCompatible(OutputPin, InputPin, CallingContext, bIgnoreArray);
	if (!bArePinsCompatible)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pins are not compatible"));
	}

	FPinConnectionResponse ConnectionResponse = DetermineConnectionResponseOfCompatibleTypedPins(PinA, PinB, InputPin, OutputPin);
	if (ConnectionResponse.Message.IsEmpty())
	{
		ConnectionResponse.Message = FText::FromString(NodeResponseMessage);
	}
	else if (!NodeResponseMessage.IsEmpty())
	{
		ConnectionResponse.Message = FText::Format(LOCTEXT("MultiMsgConnectionResponse", "{0} - {1}"), ConnectionResponse.Message, FText::FromString(NodeResponseMessage));
	}

	return ConnectionResponse;
}

const FPinConnectionResponse UFlowGraphSchema::DetermineConnectionResponseOfCompatibleTypedPins(const UEdGraphPin* PinA, const UEdGraphPin* PinB, const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	const bool bIsExistingConnection = PinA->LinkedTo.Contains(PinB);
	if (bIsExistingConnection)
	{
		// Don't error for queries about existing connections
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}

	checkf(!PinB->LinkedTo.Contains(PinA), TEXT("This should be caught with the bIsExistingConnection test above"));

	// Break existing connections on outputs for Exec Pins
	if (FFlowPin::IsExecPinCategory(InputPin->PinType.PinCategory) && OutputPin->LinkedTo.Num() > 0)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing exec connection"));
	}

	// Break existing connections on inputs for Data Pins
	if (FFlowPin::IsDataPinCategory(InputPin->PinType.PinCategory) && InputPin->LinkedTo.Num() > 0)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (InputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing data connection"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UFlowGraphSchema::IsPIESimulating()
{
	return GEditor->bIsSimulatingInEditor || (GEditor->PlayWorld != nullptr);
}

const FPinConnectionResponse UFlowGraphSchema::CanMergeNodes(const UEdGraphNode* NodeA, const UEdGraphNode* NodeB) const
{
	if (IsPIESimulating())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("The Play-in-Editor is simulating"));
	}

	// Make sure the nodes are not the same 
	if (NodeA == NodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are the same node"));
	}

	const UFlowGraphNode* FlowGraphNodeA = Cast<UFlowGraphNode>(NodeA);
	const UFlowGraphNode* FlowGraphNodeB = Cast<UFlowGraphNode>(NodeB);

	FString ReasonString;
	if (FlowGraphNodeA && FlowGraphNodeB)
	{
		TSet<const UEdGraphNode*> OtherGraphNodes; 
		if (!FlowGraphNodeB->CanAcceptSubNodeAsChild(*FlowGraphNodeA, OtherGraphNodes, &ReasonString))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ReasonString);
		}
		else
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, ReasonString);
		}
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Incompatible graph node types"));
	}
}

bool UFlowGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);
	
	if (bModified)
	{
		UFlowGraphNode* FlowGraphNodeA = Cast<UFlowGraphNode>(PinA->GetOwningNode());
		UFlowGraphNode* FlowGraphNodeB = Cast<UFlowGraphNode>(PinB->GetOwningNode());

		UEdGraph* EdGraph = FlowGraphNodeA->GetGraph();

		EdGraph->NotifyNodeChanged(FlowGraphNodeA);
		EdGraph->NotifyNodeChanged(FlowGraphNodeB);
	}

	return bModified;
}

bool UFlowGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UFlowGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	// NOTE - Adapted from UEdGraphSchema_K2::GetPinTypeColor()
	// (because we cannot directly inherit from it, but want the same color language)

	const FName& PinCategory = PinType.PinCategory;
	const UGraphEditorSettings* Settings = GetDefault<UGraphEditorSettings>();

	if (FFlowPin::IsExecPinCategory(PinCategory))
	{
		return Settings->ExecutionPinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Object)
	{
		return Settings->ObjectPinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Boolean)
	{
		return Settings->BooleanPinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Byte)
	{
		return Settings->BytePinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Int)
	{
		return Settings->IntPinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Int64)
	{
		return Settings->Int64PinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Struct)
	{
		if (PinType.PinSubCategoryObject == VectorStruct)
		{
			// vector
			return Settings->VectorPinTypeColor;
		}
		else if (PinType.PinSubCategoryObject == RotatorStruct)
		{
			// rotator
			return Settings->RotatorPinTypeColor;
		}
		else if (PinType.PinSubCategoryObject == TransformStruct)
		{
			// transform
			return Settings->TransformPinTypeColor;
		}
		else
		{
			return Settings->StructPinTypeColor;
		}
	}
	else if (PinCategory == FFlowPin::PC_String)
	{
		return Settings->StringPinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Text)
	{
		return Settings->TextPinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Name)
	{
		return Settings->NamePinTypeColor;
	}
	else if (PinCategory == FFlowPin::PC_Class)
	{
		return Settings->ClassPinTypeColor;
	}

	// Type does not have a defined color!
	return Settings->DefaultPinTypeColor;
}

FText UFlowGraphSchema::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	FText ResultPinName;
	check(Pin != nullptr);
	if (Pin->PinFriendlyName.IsEmpty())
	{
		// We don't want to display "None" for no name
		if (Pin->PinName.IsNone())
		{
			return FText::GetEmpty();
		}

		 // this option is only difference between this override and UEdGraphSchema::GetPinDisplayName
		if (GetDefault<UFlowGraphEditorSettings>()->bEnforceFriendlyPinNames)
		{
			ResultPinName = FText::FromString(FName::NameToDisplayString(Pin->PinName.ToString(), true));
		}
		else
		{
			ResultPinName = FText::FromName(Pin->PinName);
		}
	}
	else
	{
		ResultPinName = Pin->PinFriendlyName;

		bool bShouldUseLocalizedNodeAndPinNames = false;
		GConfig->GetBool(TEXT("Internationalization"), TEXT("ShouldUseLocalizedNodeAndPinNames"), bShouldUseLocalizedNodeAndPinNames, GEditorSettingsIni);

		if (!bShouldUseLocalizedNodeAndPinNames)
		{
			ResultPinName = FText::FromString(ResultPinName.BuildSourceString());
		}
	}

	return ResultPinName;
}

void UFlowGraphSchema::ConstructBasicPinTooltip(const UEdGraphPin& Pin, const FText& PinDescription, FString& TooltipOut) const
{
	if (Pin.bWasTrashed)
	{
		return;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("PinType"), UEdGraphSchema_K2::TypeToText(Pin.PinType));

	if (UEdGraphNode* PinNode = Pin.GetOwningNode())
	{
		UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(PinNode->GetSchema());
		if (ensure(K2Schema != nullptr)) // ensure that this node belongs to this schema
		{
			Args.Add(TEXT("DisplayName"), GetPinDisplayName(&Pin));
			Args.Add(TEXT("LineFeed1"), FText::FromString(TEXT("\n")));
		}
	}
	else
	{
		Args.Add(TEXT("DisplayName"), FText::GetEmpty());
		Args.Add(TEXT("LineFeed1"), FText::GetEmpty());
	}


	if (!PinDescription.IsEmpty())
	{
		Args.Add(TEXT("Description"), PinDescription);
		Args.Add(TEXT("LineFeed2"), FText::FromString(TEXT("\n\n")));
	}
	else
	{
		Args.Add(TEXT("Description"), FText::GetEmpty());
		Args.Add(TEXT("LineFeed2"), FText::GetEmpty());
	}

	TooltipOut = FText::Format(LOCTEXT("PinTooltip", "{DisplayName}{LineFeed1}{PinType}{LineFeed2}{Description}"), Args).ToString(); 
}

bool UFlowGraphSchema::CanShowDataTooltipForPin(const UEdGraphPin& Pin) const
{
	return !FFlowPin::IsExecPinCategory(Pin.PinType.PinCategory);
}

bool UFlowGraphSchema::IsTitleBarPin(const UEdGraphPin& Pin) const
{
	return FFlowPin::IsExecPinCategory(Pin.PinType.PinCategory);
}

void UFlowGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);
}

void UFlowGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(LOCTEXT("GraphEd_BreakPinLinks", "Break Pin Links"));

	TArray<UEdGraphPin*> CachedLinkedTo = TargetPin.LinkedTo;

	UFlowGraphNode* OwningFlowGraphNode = Cast<UFlowGraphNode>(TargetPin.GetOwningNodeUnchecked());
	UEdGraph* EdGraph = (OwningFlowGraphNode) ? OwningFlowGraphNode->GetGraph() : nullptr;

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	if (TargetPin.bOrphanedPin)
	{
		if (OwningFlowGraphNode)
		{
			// this calls NotifyNodeChanged()
			OwningFlowGraphNode->RemoveOrphanedPin(&TargetPin);
		}
	}
	else if (bSendsNodeNotification)
	{
		if (IsValid(EdGraph))
		{
			EdGraph->NotifyNodeChanged(OwningFlowGraphNode);
		}
	}

	for (UEdGraphPin* OtherPin : CachedLinkedTo)
	{
		UFlowGraphNode* OtherOwningFlowGraphNode = Cast<UFlowGraphNode>(OtherPin->GetOwningNodeUnchecked());
		
		if (OtherPin->bOrphanedPin)
		{
			// this calls NotifyNodeChanged()
			 OtherOwningFlowGraphNode->RemoveOrphanedPin(OtherPin);
		}
		else if (bSendsNodeNotification)
		{
			EdGraph->NotifyNodeChanged(OtherOwningFlowGraphNode);
		}
	}
}

int32 UFlowGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FFlowGraphUtils::GetFlowGraphEditor(Graph)->GetNumberOfSelectedNodes();
}

TSharedPtr<FEdGraphSchemaAction> UFlowGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FFlowGraphSchemaAction_NewComment));
}

void UFlowGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	if (!FFlowPin::IsExecPinCategory(PinA->PinType.PinCategory) || !FFlowPin::IsExecPinCategory(PinB->PinType.PinCategory))
	{
		// Disallowing Reroute node creation for non-exec connections (until we have a good solution for it)

		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateFlowRerouteNodeOnWire", "Create Flow Reroute Node"));

	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();
	UFlowGraphNode* NewReroute = FFlowGraphSchemaAction_NewNode::CreateNode(ParentGraph, nullptr, UFlowNode_Reroute::StaticClass(), KnotTopLeft, false);

	PinA->BreakLinkTo(PinB);
	PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
	PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
}

bool UFlowGraphSchema::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UFlowGraphSchema::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UFlowGraphSchema::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
}

void UFlowGraphSchema::UpdateGeneratedDisplayNames()
{
	for (UClass* FlowNodeClass : NativeFlowNodes)
	{
		UpdateGeneratedDisplayName(FlowNodeClass, true);
	}

	for (UClass* FlowNodeAddOnClass : NativeFlowNodeAddOns)
	{
		UpdateGeneratedDisplayName(FlowNodeAddOnClass, true);
	}

	for (TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.Value.GetAsset()))
		{
			UClass* NodeClass = Blueprint->GeneratedClass;
			UpdateGeneratedDisplayName(NodeClass, true);
		}
	}

	for (TPair<FName, FAssetData>& AssetData : BlueprintFlowNodeAddOns)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.Value.GetAsset()))
		{
			UClass* NodeAddOnClass = Blueprint->GeneratedClass;
			UpdateGeneratedDisplayName(NodeAddOnClass, true);
		}
	}
	
	OnNodeListChanged.Broadcast();

	// Refresh node titles
	GetDefault<UFlowGraphSchema>()->ForceVisualizationCacheClear();
}

void UFlowGraphSchema::UpdateGeneratedDisplayName(UClass* NodeClass, bool bBatch)
{
	static const FName NAME_GeneratedDisplayName("GeneratedDisplayName");

	if (NodeClass->IsChildOf(UFlowNodeBase::StaticClass()) == false)
	{
		return;
	}

	FString NameWithoutPrefix = FFlowGraphUtils::RemovePrefixFromNodeText(NodeClass->GetDisplayNameText());
	NodeClass->SetMetaData(NAME_GeneratedDisplayName, *NameWithoutPrefix);
	
	if (!bBatch)
	{
		OnNodeListChanged.Broadcast();

		// Refresh node titles
		GetDefault<UFlowGraphSchema>()->ForceVisualizationCacheClear();
	}
}

TArray<TSharedPtr<FString>> UFlowGraphSchema::GetFlowNodeCategories()
{
	if (!bInitialGatherPerformed)
	{
		GatherNodes();
	}

	TSet<FString> UnsortedCategories;
	for (const TSubclassOf<UFlowNode> FlowNodeClass : NativeFlowNodes)
	{
		if (const UFlowNode* DefaultObject = FlowNodeClass->GetDefaultObject<UFlowNode>())
		{
			UnsortedCategories.Emplace(DefaultObject->GetNodeCategory());
		}
	}

	for (const TSubclassOf<UFlowNodeAddOn> FlowNodeAddOnClass : NativeFlowNodeAddOns)
	{
		if (const UFlowNodeAddOn* DefaultObject = FlowNodeAddOnClass->GetDefaultObject<UFlowNodeAddOn>())
		{
			UnsortedCategories.Emplace(DefaultObject->GetNodeCategory());
		}
	}

	for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeOrAddOnBlueprint(AssetData.Value))
		{
			UnsortedCategories.Emplace(Blueprint->BlueprintCategory);
		}
	}

	for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodeAddOns)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeOrAddOnBlueprint(AssetData.Value))
		{
			UnsortedCategories.Emplace(Blueprint->BlueprintCategory);
		}
	}

	TArray<FString> SortedCategories = UnsortedCategories.Array();
	SortedCategories.Sort();

	// create list of categories
	TArray<TSharedPtr<FString>> Result;
	for (const FString& Category : SortedCategories)
	{
		if (!Category.IsEmpty())
		{
			Result.Emplace(MakeShareable(new FString(Category)));
		}
	}

	return Result;
}

TSubclassOf<UEdGraphNode> UFlowGraphSchema::GetAssignedGraphNodeClass(const TSubclassOf<UFlowNodeBase>& FlowNodeClass)
{
	TArray<TSubclassOf<UFlowNodeBase>> FoundParentClasses;
	UClass* ReturnClass = nullptr;

	// Collect all possible parents and their corresponding GraphNodeClasses
	for (const TPair<TSubclassOf<UFlowNodeBase>, TSubclassOf<UEdGraphNode>>& GraphNodeByFlowNode : GraphNodesByFlowNodes)
	{
		if (FlowNodeClass == GraphNodeByFlowNode.Key)
		{
			return GraphNodeByFlowNode.Value;
		}

		if (FlowNodeClass->IsChildOf(GraphNodeByFlowNode.Key))
		{
			FoundParentClasses.Add(GraphNodeByFlowNode.Key);
		}
	}

	// Of only one parent found set the return to its GraphNodeClass
	if (FoundParentClasses.Num() == 1)
	{
		ReturnClass = GraphNodesByFlowNodes.FindRef(FoundParentClasses[0]);
	}
	// If multiple parents found, find the closest one and set the return to its GraphNodeClass
	else if (FoundParentClasses.Num() > 1)
	{
		TPair<int32, UClass*> ClosestParentMatch = {1000, nullptr};
		for (const auto& ParentClass : FoundParentClasses)
		{
			int32 StepsTillExactMatch = 0;
			const UClass* LocalParentClass = FlowNodeClass;

			while (IsValid(LocalParentClass) && LocalParentClass != ParentClass && LocalParentClass != UFlowNode::StaticClass())
			{
				StepsTillExactMatch++;
				LocalParentClass = LocalParentClass->GetSuperClass();
			}

			if (StepsTillExactMatch != 0 && StepsTillExactMatch < ClosestParentMatch.Key)
			{
				ClosestParentMatch = {StepsTillExactMatch, ParentClass};
			}
		}

		ReturnClass = GraphNodesByFlowNodes.FindRef(ClosestParentMatch.Value);
	}

	return IsValid(ReturnClass) ? ReturnClass : UFlowGraphNode::StaticClass();
}

void UFlowGraphSchema::ApplyNodeOrAddOnFilter(const UFlowAsset* EditedFlowAsset, const UClass* FlowNodeClass, TArray<UFlowNodeBase*>& FilteredNodes)
{
	if (FlowNodeClass == nullptr)
	{
		return;
	}

	if (EditedFlowAsset == nullptr)
	{
		return;
	}

	if (!EditedFlowAsset->IsNodeOrAddOnClassAllowed(FlowNodeClass))
	{
		return;
	}
	
	UFlowNodeBase* NodeDefaults = FlowNodeClass->GetDefaultObject<UFlowNodeBase>();
	FilteredNodes.Emplace(NodeDefaults);
}

void UFlowGraphSchema::GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UFlowAsset* EditedFlowAsset, const FString& CategoryName)
{
	TArray<UFlowNodeBase*> FilteredNodes = GetFilteredPlaceableNodesOrAddOns(EditedFlowAsset, NativeFlowNodes, BlueprintFlowNodes);

	const UFlowGraphSettings& FlowGraphSettings = *UFlowGraphSettings::Get();
	for (const UFlowNodeBase* FlowNode : FilteredNodes)
	{
		if ((CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->GetNodeCategory())) && !FlowGraphSettings.NodesHiddenFromPalette.Contains(FlowNode->GetClass()))
		{
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(FlowNode, FlowGraphSettings));
			ActionMenuBuilder.AddAction(NewNodeAction);
		}
	}
}

TArray<UFlowNodeBase*> UFlowGraphSchema::GetFilteredPlaceableNodesOrAddOns(const UFlowAsset* EditedFlowAsset, const TArray<UClass*>& InNativeNodesOrAddOns, const TMap<FName, FAssetData>& InBlueprintNodesOrAddOns)
{
	if (!bInitialGatherPerformed)
	{
		GatherNodes();
	}

	// Flow Asset type might limit which nodes or addons are placeable 
	TArray<UFlowNodeBase*> FilteredNodes;

	FilteredNodes.Reserve(InNativeNodesOrAddOns.Num() + BlueprintFlowNodes.Num());

	for (const UClass* FlowNodeClass : InNativeNodesOrAddOns)
	{
		ApplyNodeOrAddOnFilter(EditedFlowAsset, FlowNodeClass, FilteredNodes);
	}

	for (const TPair<FName, FAssetData>& AssetData : InBlueprintNodesOrAddOns)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeOrAddOnBlueprint(AssetData.Value))
		{
			ApplyNodeOrAddOnFilter(EditedFlowAsset, Blueprint->GeneratedClass, FilteredNodes);
		}
	}

	FilteredNodes.Shrink();

	return FilteredNodes;
}

void UFlowGraphSchema::GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder, int32 SubNodeFlags) const
{
	UEdGraph* Graph = const_cast<UEdGraph*>(ContextMenuBuilder.CurrentGraph);
	UClass* GraphNodeClass = UFlowGraphNode::StaticClass();

	const UFlowAsset* EditedFlowAsset = GetEditedAssetOrClassDefault(ContextMenuBuilder.CurrentGraph);

	TArray<UFlowNodeBase*> FilteredNodes = GetFilteredPlaceableNodesOrAddOns(EditedFlowAsset, NativeFlowNodeAddOns, BlueprintFlowNodeAddOns);

	for (UFlowNodeBase* FlowNodeBase : FilteredNodes)
	{
		UFlowNodeAddOn* FlowNodeAddOnTemplate = CastChecked<UFlowNodeAddOn>(FlowNodeBase);

		// Add-Ons are futher filtered by what they are potentially being attached to 
		// (in addition to the filtering in GetFilteredPlaceableNodesOrAddOns)
		const bool bAllowAddOn = IsAddOnAllowedForSelectedObjects(ContextMenuBuilder.SelectedObjects, FlowNodeAddOnTemplate);
		if (!bAllowAddOn)
		{
			continue;
		}

		UFlowGraphNode* OpNode = NewObject<UFlowGraphNode>(Graph, GraphNodeClass);
		OpNode->NodeInstanceClass = FlowNodeAddOnTemplate->GetClass();

		TSharedPtr<FFlowSchemaAction_NewSubNode> AddOpAction =
			FFlowSchemaAction_NewSubNode::AddNewSubNodeAction(
				ContextMenuBuilder,
				FText::FromString(FlowNodeBase->GetNodeCategory()),
				FlowNodeBase->GetNodeTitle(),
				FlowNodeBase->GetNodeToolTip());

		AddOpAction->ParentNode = Cast<UFlowGraphNode>(ContextMenuBuilder.SelectedObjects[0]);
		AddOpAction->NodeTemplate = OpNode;
	}
}

bool UFlowGraphSchema::IsAddOnAllowedForSelectedObjects(const TArray<UObject*>& SelectedObjects, const UFlowNodeAddOn* AddOnTemplate)
{
	FLOW_ASSERT_ENUM_MAX(EFlowAddOnAcceptResult, 3);

	// An empty array of other addons to consider to use with CheckAcceptFlowNodeAddOnChild() below
	const TArray<UFlowNodeAddOn*> OtherAddOns;

	EFlowAddOnAcceptResult CombinedResult = EFlowAddOnAcceptResult::Undetermined;

	for (const UObject* SelectedObject : SelectedObjects)
	{
		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(SelectedObject);
		if (!IsValid(FlowGraphNode))
		{
			return false;
		}

		const UFlowNodeBase* FlowNodeOuter = Cast<UFlowNodeBase>(FlowGraphNode->GetFlowNodeBase());
		if (!IsValid(FlowNodeOuter))
		{
			continue;
		}

		const EFlowAddOnAcceptResult SelectedObjectResult = FlowNodeOuter->CheckAcceptFlowNodeAddOnChild(AddOnTemplate, OtherAddOns);

		CombinedResult = CombineFlowAddOnAcceptResult(SelectedObjectResult, CombinedResult);
		if (CombinedResult == EFlowAddOnAcceptResult::Reject)
		{
			// Any Rejection rejects the entire operation
			return false;
		}
	}

	if (CombinedResult == EFlowAddOnAcceptResult::TentativeAccept)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UFlowGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph /*= nullptr*/)
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FFlowGraphUtils::GetFlowGraphEditor(CurrentGraph)->GetNumberOfSelectedNodes() > 0) : false;
		const FText MenuDescription = bIsManyNodesSelected ? LOCTEXT("CreateCommentAction", "Create Comment from Selection") : LOCTEXT("AddCommentAction", "Add Comment...");
		const FText ToolTip = LOCTEXT("CreateCommentToolTip", "Creates a comment.");

		const TSharedPtr<FFlowGraphSchemaAction_NewComment> NewAction(new FFlowGraphSchemaAction_NewComment(FText::GetEmpty(), MenuDescription, ToolTip, 0));
		ActionMenuBuilder.AddAction(NewAction);
	}
}

bool UFlowGraphSchema::IsFlowNodeOrAddOnPlaceable(const UClass* Class)
{
	if (Class == nullptr || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_NotPlaceable | CLASS_Deprecated))
	{
		return false;
	}

	if (const UFlowNodeBase* DefaultObject = Class->GetDefaultObject<UFlowNodeBase>())
	{
		return !DefaultObject->bNodeDeprecated;
	}

	return true;
}

void UFlowGraphSchema::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	if (Blueprint && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UFlowNodeBase::StaticClass()))
	{
		bBlueprintCompilationPending = true;
	}
}

void UFlowGraphSchema::OnBlueprintCompiled()
{
	if (bBlueprintCompilationPending)
	{
		GatherNodes();
	}

	bBlueprintCompilationPending = false;
}

void UFlowGraphSchema::OnHotReload(EReloadCompleteReason ReloadCompleteReason)
{
	GatherNodes();
}

void UFlowGraphSchema::GatherNativeNodesOrAddOns(const TSubclassOf<UFlowNodeBase>& FlowNodeBaseClass, TArray<UClass*>& InOutNodesOrAddOnsArray)
{
	// collect C++ Nodes or AddOns once per editor session
	if (InOutNodesOrAddOnsArray.Num() > 0)
	{
		return;
	}

	TArray<UClass*> FlowNodesOrAddOns;
	GetDerivedClasses(FlowNodeBaseClass, FlowNodesOrAddOns);
	for (UClass* Class : FlowNodesOrAddOns)
	{
		if (Class->ClassGeneratedBy == nullptr && IsFlowNodeOrAddOnPlaceable(Class))
		{
			InOutNodesOrAddOnsArray.Emplace(Class);
		}
	}

	TArray<UClass*> GraphNodes;
	GetDerivedClasses(UFlowGraphNode::StaticClass(), GraphNodes);
	for (UClass* GraphNodeClass : GraphNodes)
	{
		const UFlowGraphNode* GraphNodeCDO = GraphNodeClass->GetDefaultObject<UFlowGraphNode>();
		for (UClass* AssignedClass : GraphNodeCDO->AssignedNodeClasses)
		{
			if (AssignedClass->IsChildOf(FlowNodeBaseClass))
			{
				GraphNodesByFlowNodes.Emplace(AssignedClass, GraphNodeClass);
			}
		}
	}
}

void UFlowGraphSchema::GatherNodes()
{
	// prevent asset crunching during PIE
	if (GEditor && GEditor->PlayWorld)
	{
		return;
	}

	// prevent adding assets while compiling blueprints
	//  (because adding assets can cause blueprint compiles to be queued as a side effect (via GetPlaceableNodeOrAddOnBlueprint))
	if (GCompilingBlueprint)
	{
		return;
	}

	bInitialGatherPerformed = true;

	GatherNativeNodesOrAddOns(UFlowNode::StaticClass(), NativeFlowNodes);
	GatherNativeNodesOrAddOns(UFlowNodeAddOn::StaticClass(), NativeFlowNodeAddOns);

	// retrieve all blueprint nodes & addons
	FARFilter Filter;
	Filter.ClassPaths.Add(UFlowNodeBlueprint::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UFlowNodeAddOnBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FoundAssets;
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistryModule.Get().GetAssets(Filter, FoundAssets);
	for (const FAssetData& AssetData : FoundAssets)
	{
		AddAsset(AssetData, true);
	}

	UpdateGeneratedDisplayNames();
}

void UFlowGraphSchema::OnAssetAdded(const FAssetData& AssetData)
{
	AddAsset(AssetData, false);
}

void UFlowGraphSchema::AddAsset(const FAssetData& AssetData, const bool bBatch)
{
	const bool bIsAssetAlreadyKnown =
		BlueprintFlowNodes.Contains(AssetData.PackageName) ||
		BlueprintFlowNodeAddOns.Contains(AssetData.PackageName);

	if (bIsAssetAlreadyKnown)
	{
		return;
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		return;
	}
	
	bool bAddedToMap = false;
	if (ShouldAddToBlueprintFlowNodesMap(AssetData, UFlowNodeBlueprint::StaticClass(), UFlowNode::StaticClass()))
	{
		BlueprintFlowNodes.Emplace(AssetData.PackageName, AssetData);
		bAddedToMap = true;
	}
	else if (ShouldAddToBlueprintFlowNodesMap(AssetData, UFlowNodeAddOnBlueprint::StaticClass(), UFlowNodeAddOn::StaticClass()))
	{
		BlueprintFlowNodeAddOns.Emplace(AssetData.PackageName, AssetData);
		bAddedToMap = true;
	}

	if (bAddedToMap && !bBatch)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			UClass* NodeClass = Blueprint->GeneratedClass;
			UpdateGeneratedDisplayName(NodeClass, false);
		}
		OnNodeListChanged.Broadcast();
	}
}

bool UFlowGraphSchema::ShouldAddToBlueprintFlowNodesMap(const FAssetData& AssetData, const TSubclassOf<UBlueprint>& BlueprintClass, const TSubclassOf<UFlowNodeBase>& FlowNodeBaseClass)
{
	if (!AssetData.GetClass()->IsChildOf(BlueprintClass))
	{
		return false;
	}

	const UBlueprint* Blueprint = GetPlaceableNodeOrAddOnBlueprint(AssetData);
	if (!IsValid(Blueprint))
	{
		return false;
	}

	UClass* GeneratedClass = Blueprint->GeneratedClass;
	if (!GeneratedClass || !GeneratedClass->IsChildOf(FlowNodeBaseClass))
	{
		return false;
	}

	return true;
}

void UFlowGraphSchema::OnAssetRemoved(const FAssetData& AssetData)
{
	if (BlueprintFlowNodes.Contains(AssetData.PackageName))
	{
		BlueprintFlowNodes.Remove(AssetData.PackageName);
		BlueprintFlowNodes.Shrink();

		OnNodeListChanged.Broadcast();
	}
	else if (BlueprintFlowNodeAddOns.Contains(AssetData.PackageName))
	{
		BlueprintFlowNodeAddOns.Remove(AssetData.PackageName);
		BlueprintFlowNodeAddOns.Shrink();

		OnNodeListChanged.Broadcast();
	}
}

void UFlowGraphSchema::OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	FString OldPackageName;
	FString OldAssetName;
	if (OldObjectPath.Split(TEXT("."), &OldPackageName, &OldAssetName))
	{
		const FName NAME_OldPackageName{OldPackageName};
		if (BlueprintFlowNodes.Contains(NAME_OldPackageName))
		{
			BlueprintFlowNodes.Remove(NAME_OldPackageName);
			BlueprintFlowNodes.Shrink();
		}
		else if (BlueprintFlowNodeAddOns.Contains(NAME_OldPackageName))
		{
			BlueprintFlowNodeAddOns.Remove(NAME_OldPackageName);
			BlueprintFlowNodeAddOns.Shrink();
		}
	}

	AddAsset(AssetData, false);
}

UBlueprint* UFlowGraphSchema::GetPlaceableNodeOrAddOnBlueprint(const FAssetData& AssetData)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
	if (Blueprint && IsFlowNodeOrAddOnPlaceable(Blueprint->GeneratedClass))
	{
		return Blueprint;
	}

	return nullptr;
}

const UFlowAsset* UFlowGraphSchema::GetEditedAssetOrClassDefault(const UEdGraph* EdGraph)
{
	if (const UFlowGraph* FlowGraph = Cast<UFlowGraph>(EdGraph))
	{
		UFlowAsset* FlowAsset = FlowGraph->GetFlowAsset();

		if (FlowAsset)
		{
			return FlowGraph->GetFlowAsset();
		}
	}

	const UClass* AssetClass = UFlowAsset::StaticClass();
	return AssetClass->GetDefaultObject<UFlowAsset>();
}

#undef LOCTEXT_NAMESPACE
