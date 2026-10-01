// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowNodeClassReplacement.h"

#include "AddOns/FlowNodeAddOn.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "FlowAsset.h"
#include "FlowCourierConverter.h"
#include "FlowGraphExporter.h"
#include "FlowGraphReconciler.h"
#include "FlowGraphRegrapher.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace
{
	bool HasPin(const TArray<FFlowPin>& Pins, FName Name, const FString& Type, const FString& SubCategoryPath)
	{
		for (const FFlowPin& Pin : Pins)
		{
			if (Pin.PinName == Name)
			{
				const UObject* Category = Pin.GetPinSubCategoryObject().Get();
				return Pin.GetPinTypeName().ToString().Equals(Type, ESearchCase::IgnoreCase)
					&& (SubCategoryPath.IsEmpty() || (Category && Category->GetPathName() == SubCategoryPath));
			}
		}
		return false;
	}

	bool HasExportedPins(const TArray<FFlowCourierPin>& Before, const TArray<FFlowCourierPin>& After,
		const TMap<FName, FName>& Mappings)
	{
		for (const FFlowCourierPin& Pin : Before)
		{
			const FName OldName(*Pin.Name);
			const FName TargetName = Mappings.FindRef(OldName).IsNone() ? OldName : Mappings.FindRef(OldName);
			if (!After.ContainsByPredicate([&](const FFlowCourierPin& Candidate)
			{
				return FName(*Candidate.Name) == TargetName && Candidate.Type == Pin.Type
					&& Candidate.SubCategoryPath == Pin.SubCategoryPath;
			}))
			{
				return false;
			}
		}
		return true;
	}

	bool ReinstanceOwnedProperties(const FFlowNodeClassReplacementPlan& Plan, UFlowNode& Destination,
		TArray<FString>& Findings)
	{
		FObjectInstancingGraph InstancingGraph(&Destination);
		InstancingGraph.SetDestinationRoot(&Destination, Plan.Original);
		for (const TPair<FString, FString>& Entry : Plan.Properties)
		{
			const FString& SourceName = Plan.SourcePropertyNames.FindChecked(Entry.Key);
			FProperty* SourceProperty = FindFProperty<FProperty>(Plan.Original->GetClass(), *SourceName);
			FProperty* DestinationProperty = FindFProperty<FProperty>(Destination.GetClass(), *Entry.Key);
			if (!SourceProperty || !DestinationProperty || !DestinationProperty->SameType(SourceProperty))
			{
				Findings.Add(FString::Printf(TEXT("Property '%s' cannot be preserved across node classes."),
					*SourceName));
				return false;
			}
			void* Value = DestinationProperty->ContainerPtrToValuePtr<void>(&Destination);
			if (DestinationProperty->HasAnyPropertyFlags(CPF_InstancedReference | CPF_ContainsInstancedReference)
				|| DestinationProperty->IsA<FStructProperty>())
			{
				DestinationProperty->InstanceSubobjects(
					Value, SourceProperty->ContainerPtrToValuePtr<void>(Plan.Original), &Destination, &InstancingGraph);
			}
			FString CopiedValue;
			DestinationProperty->ExportTextItem_Direct(CopiedValue, Value, Value, nullptr, PPF_None);
			if (CopiedValue.Contains(Plan.Original->GetPathName()))
			{
				Findings.Add(FString::Printf(TEXT("Property '%s' still references a subobject of the old node."),
					*Entry.Key));
				return false;
			}
		}
		return true;
	}

	bool CheckPinMappings(const FFlowCourierOp& Original, UFlowNode& Preview,
		const TMap<FName, FName>& Mappings, TArray<FString>& Findings)
	{
		Preview.TryUpdateAutoDataPins();
		TArray<FFlowPin> Inputs;
		TArray<FFlowPin> Outputs;
		Preview.GetCatalogPins(Inputs, Outputs);
		for (const FFlowPin& Pin : Preview.GetContextInputs())
		{
			Inputs.AddUnique(Pin);
		}
		for (const FFlowPin& Pin : Preview.GetContextOutputs())
		{
			Outputs.AddUnique(Pin);
		}
		TSet<FName> SourceNames;
		TSet<FName> TargetNames;
		bool bValid = true;
		for (const TPair<const TArray<FFlowCourierPin>*, const TArray<FFlowPin>*>& Side :
			{TPair<const TArray<FFlowCourierPin>*, const TArray<FFlowPin>*>(&Original.InputPins, &Inputs),
			 TPair<const TArray<FFlowCourierPin>*, const TArray<FFlowPin>*>(&Original.OutputPins, &Outputs)})
		{
			for (const FFlowCourierPin& Pin : *Side.Key)
			{
				const FName OldName(*Pin.Name);
				bool bAlreadySeen = false;
				SourceNames.Add(OldName, &bAlreadySeen);
				if (bAlreadySeen)
				{
					Findings.Add(FString::Printf(TEXT("Source has duplicate pin name '%s'."), *Pin.Name));
					bValid = false;
				}
				const FName NewName = Mappings.FindRef(OldName).IsNone() ? OldName : Mappings.FindRef(OldName);
				if (!HasPin(*Side.Value, NewName, Pin.Type, Pin.SubCategoryPath))
				{
					Findings.Add(FString::Printf(TEXT("Pin '%s' has no compatible destination '%s' (%s)"),
						*Pin.Name, *NewName.ToString(), *Pin.Type));
					bValid = false;
				}
			}
			for (const FFlowPin& Pin : *Side.Value)
			{
				bool bAlreadySeen = false;
				TargetNames.Add(Pin.PinName, &bAlreadySeen);
				if (bAlreadySeen)
				{
					Findings.Add(FString::Printf(TEXT("Target has duplicate pin name '%s'."), *Pin.PinName.ToString()));
					bValid = false;
				}
			}
		}
		for (const TPair<FName, FName>& Mapping : Mappings)
		{
			if (Mapping.Key.IsNone() || Mapping.Value.IsNone() || !SourceNames.Contains(Mapping.Key))
			{
				Findings.Add(FString::Printf(TEXT("Pin mapping '%s' has no valid source pin or destination name."),
					*Mapping.Key.ToString()));
				bValid = false;
			}
		}
		return bValid;
	}

	bool VerifyAddOns(const UFlowNodeBase& Parent, const TArray<FFlowGraphParsedNodeAddOn>& Expected)
	{
		const TArray<UFlowNodeAddOn*>& Actual = Parent.GetFlowNodeAddOnChildren();
		if (Actual.Num() != Expected.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Expected.Num(); ++Index)
		{
			if (!IsValid(Actual[Index]) || Actual[Index]->GetGuid() != Expected[Index].AddOnGuid
				|| Actual[Index]->GetClass()->GetPathName() != Expected[Index].AddOnType
				|| !VerifyAddOns(*Actual[Index], Expected[Index].AddOns))
			{
				return false;
			}
		}
		return true;
	}

	bool CheckAddonOwnedReferences(const TArray<FFlowGraphParsedNodeAddOn>& AddOns,
		const FString& SourceNodePath, TArray<FString>& Findings)
	{
		for (const FFlowGraphParsedNodeAddOn& AddOn : AddOns)
		{
			for (const TPair<FString, FString>& Property : AddOn.Properties)
			{
				if (Property.Value.Contains(SourceNodePath))
				{
					Findings.Add(FString::Printf(TEXT("Addon %s property '%s' references the old node's subobjects."),
						*AddOn.AddOnGuid.ToString(), *Property.Key));
						return false;
				}
			}
			if (!CheckAddonOwnedReferences(AddOn.AddOns, SourceNodePath, Findings))
			{
				return false;
			}
		}
		return true;
	}

	bool CheckEditorAddOns(const UFlowNodeBase& Source, const UFlowNodeBase& Preview,
		TArray<FString>& Findings)
	{
		const TArray<UFlowNodeAddOn*>& SourceChildren = Source.GetFlowNodeAddOnChildren();
		const TArray<UFlowNodeAddOn*>& PreviewChildren = Preview.GetFlowNodeAddOnChildren();
		if (SourceChildren.Num() != PreviewChildren.Num())
		{
			Findings.Add(TEXT("Addon count differs between the source and replacement."));
			return false;
		}
		for (int32 Index = 0; Index < SourceChildren.Num(); ++Index)
		{
			const UFlowNodeAddOn* Old = SourceChildren[Index];
			const UFlowNodeAddOn* New = PreviewChildren[Index];
			const UFlowGraphNode* Editor = IsValid(Old) ? Cast<UFlowGraphNode>(Old->GetGraphNode()) : nullptr;
			if (!IsValid(New) || !IsValid(Editor) || New->GetGuid() != Old->GetGuid()
				|| !Editor->IsA(UFlowGraphSchema::GetAssignedGraphNodeClass(New->GetClass())))
			{
				Findings.Add(FString::Printf(TEXT("Addon %s: replacement=%s, editor=%s, editor GUID=%s, required editor=%s."),
					*GetNameSafe(Old), *GetNameSafe(New), *GetNameSafe(Editor),
					IsValid(Editor) ? *Editor->NodeGuid.ToString() : TEXT("none"),
					IsValid(New) ? *GetNameSafe(UFlowGraphSchema::GetAssignedGraphNodeClass(New->GetClass()).Get()) : TEXT("none")));
				return false;
			}
			if (Preview.CheckAcceptFlowNodeAddOnChild(New, {}) == EFlowAddOnAcceptResult::Reject)
			{
				Findings.Add(FString::Printf(TEXT("Replacement parent rejects addon %s."), *New->GetName()));
				return false;
			}
			if (!CheckEditorAddOns(*Old, *New, Findings))
			{
				return false;
			}
		}
		return true;
	}

	void GatherExpectedAddOns(const TArray<FFlowGraphParsedNodeAddOn>& AddOns,
		TMap<FGuid, const FFlowGraphParsedNodeAddOn*>& OutAddOns)
	{
		for (const FFlowGraphParsedNodeAddOn& AddOn : AddOns)
		{
			OutAddOns.Add(AddOn.AddOnGuid, &AddOn);
			GatherExpectedAddOns(AddOn.AddOns, OutAddOns);
		}
	}

	FString ConnectionSignature(const FFlowCourierOp& Op, const FFlowNodeClassReplacementPlan& Plan,
		bool bApplyMappings)
	{
		FGuid SourceGuid;
		FGuid TargetGuid;
		if (!FGuid::Parse(Op.Source.NodeGuid, SourceGuid) || !FGuid::Parse(Op.Target.NodeGuid, TargetGuid))
		{
			return FString();
		}
		FName SourcePin(*Op.Source.Pin);
		FName TargetPin(*Op.Target.Pin);
		if (bApplyMappings && SourceGuid == Plan.Guid)
		{
			if (const FName* Replacement = Plan.PinMappings.Find(SourcePin))
			{
				SourcePin = *Replacement;
			}
		}
		if (bApplyMappings && TargetGuid == Plan.Guid)
		{
			if (const FName* Replacement = Plan.PinMappings.Find(TargetPin))
			{
				TargetPin = *Replacement;
			}
		}
		return FString::Printf(TEXT("%s.%s>%s.%s"), *SourceGuid.ToString(EGuidFormats::Digits),
			*SourcePin.ToString(), *TargetGuid.ToString(EGuidFormats::Digits), *TargetPin.ToString());
	}

	bool VerifyExport(const FFlowNodeClassReplacementPlan& Plan, TArray<FString>& Findings)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		FString Error;
		if (!FFlowCourierConverter::ParseDocument(UFlowGraphExporter::ExportFlowGraphToString(Plan.Asset),
			Document, Issues, Error))
		{
			Findings.Add(Error);
			return false;
		}
		TSet<FString> ExpectedConnections;
		TSet<FString> ActualConnections;
		for (const FFlowCourierOp& Connection : Plan.AllConnections)
		{
			ExpectedConnections.Add(ConnectionSignature(Connection, Plan, true));
		}
		bool bFoundNode = false;
		TMap<FGuid, const FFlowGraphParsedNodeAddOn*> ExpectedAddOns;
		GatherExpectedAddOns(Plan.AddOns, ExpectedAddOns);
		TSet<FGuid> FoundAddOns;
		for (const FFlowCourierOp& Op : Document.Ops)
		{
			FGuid Guid;
			if (Op.Kind == EFlowCourierOpKind::AddConnection)
			{
				ActualConnections.Add(ConnectionSignature(Op, Plan, false));
			}
			else if (FGuid::Parse(Op.Guid, Guid) && Guid == Plan.Guid
				&& Op.Kind == EFlowCourierOpKind::UpsertNode)
			{
				bFoundNode = true;
				if (Op.Type != Plan.TargetClass->GetPathName())
				{
					Findings.Add(TEXT("The replacement class did not persist on the node GUID."));
				}
				if (!HasExportedPins(Plan.InputPins, Op.InputPins, Plan.PinMappings)
					|| !HasExportedPins(Plan.OutputPins, Op.OutputPins, Plan.PinMappings))
				{
					Findings.Add(TEXT("One or more node pins changed or disappeared during replacement."));
				}
				for (const TPair<FString, FString>& Property : Plan.Properties)
				{
					const FString Expected = Property.Value.Replace(
						*Plan.Original->GetPathName(), *Plan.Asset->GetNode(Plan.Guid)->GetPathName());
					if (Op.Properties.FindRef(Property.Key) != Expected)
					{
						Findings.Add(FString::Printf(TEXT("Node property '%s' changed during class replacement."),
							*Property.Key));
					}
				}
			}
			else if (Op.Kind == EFlowCourierOpKind::UpsertAddon && ExpectedAddOns.Contains(Guid))
			{
				FoundAddOns.Add(Guid);
				const FFlowGraphParsedNodeAddOn* Expected = ExpectedAddOns[Guid];
				if (Op.Type != Expected->AddOnType)
				{
					Findings.Add(FString::Printf(TEXT("Addon %s has the wrong class."), *Op.Guid));
				}
				for (const TPair<FString, FString>& Property : Expected->Properties)
				{
					if (Op.Properties.FindRef(Property.Key) != Property.Value)
					{
						Findings.Add(FString::Printf(TEXT("Addon %s lost property '%s'."),
								*Op.Guid, *Property.Key));
						}
				}
			}
		}
		bool bConnectionsMatch = ActualConnections.Num() == ExpectedConnections.Num();
		for (const FString& Signature : ExpectedConnections)
		{
			bConnectionsMatch &= ActualConnections.Contains(Signature);
		}
		if (!bFoundNode || FoundAddOns.Num() != ExpectedAddOns.Num() || !bConnectionsMatch)
		{
			Findings.Add(TEXT("Node, addon, or connection inventory changed during class replacement."));
		}
		return Findings.IsEmpty();
	}
}

bool FFlowNodeClassReplacement::MapAddOns(
	TArray<FFlowGraphParsedNodeAddOn>& AddOns,
	const UFlowAsset& Asset,
	const FFlowMCPReplaceFlowNodeClassRequest& Request,
	TSet<FString>& SeenClassMappings,
	TSet<FString>& SeenPropertyMappings,
	TArray<FGuid>& ReplacedAddOnGuids,
	TArray<FString>& OutFindings)
{
	for (FFlowGraphParsedNodeAddOn& AddOn : AddOns)
	{
		for (const TPair<FString, FString>& Mapping : Request.AddOnClassMappings)
		{
			FGuid Guid;
			if (FGuid::Parse(Mapping.Key, Guid) && Guid == AddOn.AddOnGuid)
			{
				SeenClassMappings.Add(Mapping.Key);
				if (AddOn.AddOnType != Mapping.Value)
				{
					UClass* NewClass = UFlowGraphImporter::ResolveNodeClass(Mapping.Value);
					if (!NewClass || !NewClass->IsChildOf(UFlowNodeAddOn::StaticClass())
						|| NewClass->HasAnyClassFlags(CLASS_Abstract))
					{
						OutFindings.Add(FString::Printf(TEXT("Invalid addon replacement class '%s' for GUID %s"),
							*Mapping.Value, *Mapping.Key));
						return false;
					}
					FText FailureReason;
					if (!Asset.IsNodeOrAddOnClassAllowed(NewClass, &FailureReason))
					{
						OutFindings.Add(FString::Printf(TEXT("Addon replacement class '%s' is not allowed in asset '%s' for GUID %s: %s"),
							*Mapping.Value, *Asset.GetClass()->GetName(), *Mapping.Key, *FailureReason.ToString()));
						return false;
					}
					AddOn.AddOnType = NewClass->GetPathName();
					ReplacedAddOnGuids.Add(AddOn.AddOnGuid);
				}
			}
		}

		for (const TPair<FString, FString>& Mapping : Request.AddOnPropertyMappings)
		{
			FString GuidText;
			FString PropertyName;
			FGuid Guid;
			if (Mapping.Key.Split(TEXT("."), &GuidText, &PropertyName)
				&& FGuid::Parse(GuidText, Guid) && Guid == AddOn.AddOnGuid)
			{
				SeenPropertyMappings.Add(Mapping.Key);
				if (const FString* Value = AddOn.Properties.Find(PropertyName))
				{
					if (Mapping.Value != PropertyName && AddOn.Properties.Contains(Mapping.Value))
					{
						OutFindings.Add(FString::Printf(TEXT("Addon %s has conflicting property destination '%s'"),
							*GuidText, *Mapping.Value));
						return false;
					}
					const FString Copy = *Value;
					AddOn.Properties.Remove(PropertyName);
					AddOn.Properties.Add(Mapping.Value, Copy);
				}
				else
				{
					OutFindings.Add(FString::Printf(TEXT("Addon property '%s' is not exported on %s"),
						*PropertyName, *GuidText));
					return false;
				}
			}
		}
		if (!MapAddOns(AddOn.AddOns, Asset, Request, SeenClassMappings, SeenPropertyMappings,
			ReplacedAddOnGuids, OutFindings))
		{
			return false;
		}
	}
	return true;
}

bool FFlowNodeClassReplacement::BuildPlan(
	const FFlowMCPReplaceFlowNodeClassRequest& Request,
	FFlowNodeClassReplacementPlan& OutPlan,
	TArray<FString>& OutFindings)
{
	if (!FGuid::Parse(Request.NodeGuid, OutPlan.Guid) || !OutPlan.Guid.IsValid())
	{
		OutFindings.Add(TEXT("NodeGuid must identify an existing node."));
		return false;
	}
	OutPlan.Asset = FFlowGraphReconciler::FindOrLoadFlowAsset(Request.AssetPath);
	OutPlan.Original = IsValid(OutPlan.Asset) ? OutPlan.Asset->GetNode(OutPlan.Guid) : nullptr;
	OutPlan.TargetClass = UFlowGraphImporter::ResolveNodeClass(Request.NewNodeClass);
	if (!IsValid(OutPlan.Asset) || !IsValid(OutPlan.Original) || !IsValid(OutPlan.TargetClass)
		|| !OutPlan.TargetClass->IsChildOf(UFlowNode::StaticClass())
		|| OutPlan.TargetClass->HasAnyClassFlags(CLASS_Abstract)
		|| !OutPlan.Asset->IsNodeOrAddOnClassAllowed(OutPlan.TargetClass))
	{
		OutFindings.Add(TEXT("The asset, source node, or placeable target node class is invalid."));
		return false;
	}
	UFlowGraphRegrapher::CollectGraphParityIssues(OutPlan.Asset, OutFindings);
	OutPlan.EditorNode = Cast<UFlowGraphNode>(OutPlan.Original->GetGraphNode());
	if (!OutFindings.IsEmpty() || !IsValid(OutPlan.EditorNode)
		|| OutPlan.EditorNode->NodeGuid != OutPlan.Guid || !IsValid(OutPlan.Asset->GetGraph()))
	{
		OutFindings.Add(TEXT("The editor graph and runtime node are not synchronized."));
		return false;
	}
	if (!OutPlan.EditorNode->IsA(UFlowGraphSchema::GetAssignedGraphNodeClass(OutPlan.TargetClass)))
	{
		OutFindings.Add(TEXT("The target node requires a different editor graph node class."));
		return false;
	}

	FFlowCourierDocument Document;
	TArray<FFlowCourierIssue> Issues;
	FString Error;
	if (!FFlowCourierConverter::ParseDocument(UFlowGraphExporter::ExportFlowGraphToString(OutPlan.Asset),
		Document, Issues, Error))
	{
		OutFindings.Add(Error);
		return false;
	}
	for (const FFlowCourierIssue& Issue : Issues)
	{
		if (Issue.Severity == EFlowValidationSeverity::Error)
		{
			OutFindings.Add(Issue.Message);
		}
	}
	if (!OutFindings.IsEmpty())
	{
		return false;
	}

	FFlowCourierDocument ScopedDocument = Document;
	ScopedDocument.Ops.Reset();
	TSet<FGuid> ScopedOwnerGuids;
	ScopedOwnerGuids.Add(OutPlan.Guid);
	TSet<FGuid> ScopedAddonGuids;
	TSet<int32> ScopedAddonIndices;
	for (const FFlowCourierOp& Op : Document.Ops)
	{
		FGuid Guid;
		if (Op.Kind == EFlowCourierOpKind::UpsertNode && FGuid::Parse(Op.Guid, Guid) && Guid == OutPlan.Guid)
		{
			ScopedDocument.Ops.Add(Op);
		}
	}
	bool bAddedAddon;
	do
	{
		bAddedAddon = false;
		for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
		{
			const FFlowCourierOp& Op = Document.Ops[Index];
			FGuid ParentGuid;
			if (Op.Kind != EFlowCourierOpKind::UpsertAddon || ScopedAddonIndices.Contains(Index)
				|| !FGuid::Parse(Op.ParentGuid, ParentGuid) || !ScopedOwnerGuids.Contains(ParentGuid))
			{
				continue;
			}
			FGuid Guid;
			if (!FGuid::Parse(Op.Guid, Guid) || !Guid.IsValid() || ScopedOwnerGuids.Contains(Guid))
			{
				OutFindings.Add(FString::Printf(TEXT("The target node has an invalid or duplicate addon GUID '%s'."), *Op.Guid));
				return false;
			}
			ScopedDocument.Ops.Add(Op);
			ScopedAddonIndices.Add(Index);
			ScopedAddonGuids.Add(Guid);
			ScopedOwnerGuids.Add(Guid);
			bAddedAddon = true;
		}
	} while (bAddedAddon);
	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const FFlowCourierOp& Op = Document.Ops[Index];
		FGuid Guid;
		if ((Op.Kind == EFlowCourierOpKind::UpsertNode || Op.Kind == EFlowCourierOpKind::UpsertAddon)
			&& FGuid::Parse(Op.Guid, Guid) && ((Op.Kind == EFlowCourierOpKind::UpsertAddon && Guid == OutPlan.Guid)
				|| (ScopedAddonGuids.Contains(Guid) && !ScopedAddonIndices.Contains(Index))))
		{
			OutFindings.Add(FString::Printf(TEXT("The target node shares an addon GUID '%s' with another graph entry."), *Op.Guid));
			return false;
		}
	}

	if (!Request.bAllowDuplicateAddonGuidRepair)
	{
		TSet<FGuid> SeenAddOnGuids;
		for (const FFlowCourierOp& Op : Document.Ops)
		{
			FGuid Guid;
			if (Op.Kind != EFlowCourierOpKind::UpsertAddon || !FGuid::Parse(Op.Guid, Guid) || !Guid.IsValid())
			{
				continue;
			}
			bool bAlreadySeen = false;
			SeenAddOnGuids.Add(Guid, &bAlreadySeen);
			if (bAlreadySeen)
			{
				OutFindings.Add(FString::Printf(
					TEXT("Asset has duplicate addon GUID '%s' outside the target node; FlowAsset will re-mint duplicates on save. ")
					TEXT("Set bAllowDuplicateAddonGuidRepair=true only with approval."), *Guid.ToString()));
				return false;
			}
		}
	}

	TArray<FFlowGraphParsedNode> Nodes;
	TArray<FFlowGraphParsedConnection> Connections;
	TArray<FGuid> ScopedGuids;
	TMap<FString, FGuid> Aliases;
	FFlowCourierConverter::ConvertToParsedGraph(ScopedDocument, Nodes, Connections, ScopedGuids, Aliases, Issues);
	for (const FFlowCourierIssue& Issue : Issues)
	{
		if (Issue.Severity == EFlowValidationSeverity::Error)
		{
			OutFindings.Add(Issue.Message);
		}
	}
	if (!OutFindings.IsEmpty())
	{
		return false;
	}
	const FFlowGraphParsedNode* Source = Nodes.FindByPredicate([&OutPlan](const FFlowGraphParsedNode& Node)
	{
		return Node.NodeGuid == OutPlan.Guid;
	});
	if (!Source)
	{
		OutFindings.Add(TEXT("The selected node was not exported."));
		return false;
	}
	OutPlan.Properties = Source->Properties;
	for (const TPair<FString, FString>& Property : Source->Properties)
	{
		OutPlan.SourcePropertyNames.Add(Property.Key, Property.Key);
	}
	for (const TPair<FString, FString>& Mapping : Request.PropertyMappings)
	{
		const FString* Value = OutPlan.Properties.Find(Mapping.Key);
		if (!Value || (Mapping.Key != Mapping.Value && OutPlan.Properties.Contains(Mapping.Value)))
		{
			OutFindings.Add(FString::Printf(TEXT("Property '%s' has no unambiguous exported value."), *Mapping.Key));
			return false;
		}
		const FString Copy = *Value;
		OutPlan.Properties.Remove(Mapping.Key);
		OutPlan.Properties.Add(Mapping.Value, Copy);
		OutPlan.SourcePropertyNames.Remove(Mapping.Key);
		OutPlan.SourcePropertyNames.Add(Mapping.Value, Mapping.Key);
	}

	OutPlan.AddOns = Source->AddOns;
	TSet<FString> SeenClassMappings;
	TSet<FString> SeenPropertyMappings;
	if (!MapAddOns(OutPlan.AddOns, *OutPlan.Asset, Request, SeenClassMappings, SeenPropertyMappings,
		OutPlan.ReplacedAddOnGuids, OutFindings)
		|| !CheckAddonOwnedReferences(OutPlan.AddOns, OutPlan.Original->GetPathName(), OutFindings))
	{
		return false;
	}
	for (const TPair<FString, FString>& Mapping : Request.AddOnClassMappings)
	{
		if (!SeenClassMappings.Contains(Mapping.Key))
		{
			OutFindings.Add(FString::Printf(TEXT("Addon class mapping does not match this node: %s"), *Mapping.Key));
		}
	}
	for (const TPair<FString, FString>& Mapping : Request.AddOnPropertyMappings)
	{
		if (!SeenPropertyMappings.Contains(Mapping.Key))
		{
			OutFindings.Add(FString::Printf(TEXT("Addon property mapping does not match this node: %s"), *Mapping.Key));
		}
	}
	for (const TPair<FString, FString>& Mapping : Request.PinMappings)
	{
		OutPlan.PinMappings.Add(FName(*Mapping.Key), FName(*Mapping.Value));
	}
	for (const FFlowCourierOp& Op : Document.Ops)
	{
		if (Op.Kind != EFlowCourierOpKind::AddConnection)
		{
			continue;
		}
		OutPlan.AllConnections.Add(Op);
		FGuid SourceGuid;
		FGuid TargetGuid;
		if ((FGuid::Parse(Op.Source.NodeGuid, SourceGuid) && SourceGuid == OutPlan.Guid)
			|| (FGuid::Parse(Op.Target.NodeGuid, TargetGuid) && TargetGuid == OutPlan.Guid))
		{
			OutPlan.Connections.Add(Op);
		}
	}
	if (!OutFindings.IsEmpty())
	{
		return false;
	}
	OutPlan.bNeedsReplacement = OutPlan.Original->GetClass() != OutPlan.TargetClass
		|| !OutPlan.ReplacedAddOnGuids.IsEmpty() || !Request.PropertyMappings.IsEmpty()
		|| !Request.AddOnPropertyMappings.IsEmpty() || !Request.PinMappings.IsEmpty();
	if (!OutPlan.bNeedsReplacement)
	{
		return true;
	}

	UFlowNode* Preview = NewObject<UFlowNode>(OutPlan.Asset, OutPlan.TargetClass, NAME_None, RF_Transient);
	Preview->SetGuid(OutPlan.Guid);
	if (!UFlowGraphImporter::SetNodeProperties(Preview, OutPlan.Properties, Error)
		|| !UFlowGraphImporter::CreateAddOnsRecursive(Preview, OutPlan.AddOns, Error))
	{
		OutFindings.Add(Error);
		return false;
	}
	if (!ReinstanceOwnedProperties(OutPlan, *Preview, OutFindings)
		|| !CheckEditorAddOns(*OutPlan.Original, *Preview, OutFindings))
	{
		return false;
	}
	for (const FFlowCourierOp& Op : Document.Ops)
	{
		FGuid ExportedGuid;
		if (Op.Kind == EFlowCourierOpKind::UpsertNode
			&& FGuid::Parse(Op.Guid, ExportedGuid) && ExportedGuid == OutPlan.Guid)
		{
			OutPlan.InputPins = Op.InputPins;
			OutPlan.OutputPins = Op.OutputPins;
			CheckPinMappings(Op, *Preview, OutPlan.PinMappings, OutFindings);
			break;
		}
	}
	OutPlan.AddOnCount = OutPlan.AddOns.Num();
	return OutFindings.IsEmpty();
}

bool FFlowNodeClassReplacement::PairEditorAddOns(
	const UFlowNodeBase& Original,
	UFlowNodeBase& Replacement,
	UFlowNode& RootNode,
	TArray<FString>& OutFindings)
{
	const TArray<UFlowNodeAddOn*>& Before = Original.GetFlowNodeAddOnChildren();
	const TArray<UFlowNodeAddOn*>& After = Replacement.GetFlowNodeAddOnChildren();
	if (Before.Num() != After.Num())
	{
		OutFindings.Add(TEXT("Addon count changed during replacement."));
		return false;
	}
	for (int32 Index = 0; Index < Before.Num(); ++Index)
	{
		UFlowNodeAddOn* OldAddOn = Before[Index];
		UFlowNodeAddOn* NewAddOn = After[Index];
		UFlowGraphNode* EditorAddOn = IsValid(OldAddOn) ? Cast<UFlowGraphNode>(OldAddOn->GetGraphNode()) : nullptr;
		if (!IsValid(NewAddOn) || !IsValid(EditorAddOn) || OldAddOn->GetGuid() != NewAddOn->GetGuid()
			|| !EditorAddOn->IsA(UFlowGraphSchema::GetAssignedGraphNodeClass(NewAddOn->GetClass())))
		{
			OutFindings.Add(TEXT("An addon has no matching GUID or compatible editor node."));
			return false;
		}
		EditorAddOn->Modify();
		EditorAddOn->SetNodeTemplate(NewAddOn);
		NewAddOn->SetGraphNode(EditorAddOn);
		NewAddOn->SetFlowNodeForEditor(&RootNode);
		if (!PairEditorAddOns(*OldAddOn, *NewAddOn, RootNode, OutFindings))
		{
			return false;
		}
	}
	return true;
}

bool FFlowNodeClassReplacement::ApplyPlan(
	const FFlowNodeClassReplacementPlan& Plan,
	TArray<FString>& OutFindings)
{
	if (!Plan.bNeedsReplacement)
	{
		return true;
	}
	UFlowAsset* Asset = Plan.Asset;
	UFlowNode* Original = Plan.Original;
	UFlowGraphNode* EditorNode = Plan.EditorNode;
	UFlowGraph* Graph = Cast<UFlowGraph>(Asset->GetGraph());
	if (!IsValid(Graph) || Asset->GetNode(Plan.Guid) != Original)
	{
		OutFindings.Add(TEXT("The source graph changed after the replacement plan was computed."));
		return false;
	}

	FString Error;
	UFlowNode* Replacement = NewObject<UFlowNode>(Asset, Plan.TargetClass, NAME_None, RF_Transactional);
	Replacement->SetGuid(Plan.Guid);
	if (!UFlowGraphImporter::SetNodeProperties(Replacement, Plan.Properties, Error)
		|| !UFlowGraphImporter::CreateAddOnsRecursive(Replacement, Plan.AddOns, Error))
	{
		OutFindings.Add(Error);
		return false;
	}
	if (!ReinstanceOwnedProperties(Plan, *Replacement, OutFindings))
	{
		return false;
	}

	for (const FName Name : {FName(TEXT("Connections")), FName(TEXT("SignalMode"))})
	{
		if (FProperty* Property = FindFProperty<FProperty>(UFlowNode::StaticClass(), Name))
		{
			Property->CopyCompleteValue_InContainer(Replacement, Original);
		}
	}
	EditorNode->Modify();
	Graph->Modify();
	if (!PairEditorAddOns(*Original, *Replacement, *Replacement, OutFindings))
	{
		return false;
	}
	Replacement->SetGraphNode(EditorNode);
	EditorNode->SetNodeTemplate(Replacement);
	for (UEdGraphPin* Pin : EditorNode->Pins)
	{
		if (Pin)
		{
			if (const FName* NewName = Plan.PinMappings.Find(Pin->PinName))
			{
				Pin->PinName = *NewName;
			}
		}
	}
	Asset->RegisterNode(Plan.Guid, Replacement);
	Replacement->TryUpdateAutoDataPins();
	EditorNode->ReconstructNode();
	EditorNode->RebuildRuntimeAddOnsFromEditorSubNodes(false);
	Asset->HarvestNodeConnections();
	if (Asset->GetNode(Plan.Guid) != Replacement || EditorNode->GetNodeTemplate() != Replacement
		|| !VerifyAddOns(*Replacement, Plan.AddOns))
	{
		OutFindings.Add(TEXT("Runtime node, editor node, or addon identities diverged after replacement."));
		return false;
	}
	UFlowGraphRegrapher::CollectGraphParityIssues(Asset, OutFindings);
	if (OutFindings.IsEmpty())
	{
		VerifyExport(Plan, OutFindings);
	}
	return OutFindings.IsEmpty();
}
