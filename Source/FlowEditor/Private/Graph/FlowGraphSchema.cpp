// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphSchema.h"

#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_Start.h"
#include "Nodes/Route/FlowNode_Reroute.h"

#include "AssetRegistryModule.h"
#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "EdGraph/EdGraph.h"
#include "Misc/HotReloadInterface.h"
#include "ScopedTransaction.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "FlowGraphSchema"

TArray<UClass*> UFlowGraphSchema::NativeFlowNodes;
TMap<FName, FAssetData> UFlowGraphSchema::BlueprintFlowNodes;
TMap<UClass*, UClass*> UFlowGraphSchema::AssignedGraphNodeClasses;

bool UFlowGraphSchema::bBlueprintCompilationPending;

FFlowGraphSchemaRefresh UFlowGraphSchema::OnNodeListChanged;

UFlowGraphSchema::UFlowGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowGraphSchema::SubscribeToAssetChanges()
{
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistry.Get().OnFilesLoaded().AddStatic(&UFlowGraphSchema::GatherFlowNodes);
	AssetRegistry.Get().OnAssetAdded().AddStatic(&UFlowGraphSchema::OnAssetAdded);
	AssetRegistry.Get().OnAssetRemoved().AddStatic(&UFlowGraphSchema::OnAssetRemoved);

	IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
	HotReloadSupport.OnHotReload().AddStatic(&UFlowGraphSchema::OnHotReload);

	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().AddStatic(&UFlowGraphSchema::OnBlueprintPreCompile);
		GEditor->OnBlueprintCompiled().AddStatic(&UFlowGraphSchema::OnBlueprintCompiled);
	}
}

void UFlowGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UClass* AssetClass, const FString& CategoryName)
{
	GetFlowNodeActions(ActionMenuBuilder, AssetClass->GetDefaultObject<UFlowAsset>(), CategoryName);
	GetCommentAction(ActionMenuBuilder);
}

void UFlowGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetFlowNodeActions(ContextMenuBuilder, GetAssetClassDefaults(ContextMenuBuilder.CurrentGraph), FString());
	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	if (!ContextMenuBuilder.FromPin && FFlowGraphUtils::GetFlowAssetEditor(ContextMenuBuilder.CurrentGraph)->CanPasteNodes())
	{
		const TSharedPtr<FFlowGraphSchemaAction_Paste> NewAction(new FFlowGraphSchemaAction_Paste(FText::GetEmpty(), LOCTEXT("PasteHereAction", "Paste here"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(NewAction);
	}
}

void UFlowGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	// Start node
	UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(&Graph, nullptr, UFlowNode_Start::StaticClass(), FVector2D::ZeroVector);
	SetNodeMetaData(NewGraphNode, FNodeMetadata::DefaultGraphNode);

	const UFlowAsset* AssetClassDefaults = GetAssetClassDefaults(&Graph);
	if (AssetClassDefaults && AssetClassDefaults->bStartNodePlacedAsGhostNode)
	{
		NewGraphNode->MakeAutomaticallyPlacedGhostNode();
	}

	CastChecked<UFlowGraph>(&Graph)->GetFlowAsset()->HarvestNodeConnections();
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
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	if (PinA->bOrphanedPin || PinB->bOrphanedPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot make new connections to orphaned pin"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions are not compatible"));
	}

	// Break existing connections on outputs only - multiple input connections are acceptable
	if (OutputPin->LinkedTo.Num() > 0)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UFlowGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	const bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		PinA->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}

	return bModified;
}

bool UFlowGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UFlowGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

void UFlowGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	TargetNode.GetGraph()->NotifyGraphChanged();
}

void UFlowGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(LOCTEXT("GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	if (TargetPin.bOrphanedPin)
	{
		// this calls NotifyGraphChanged()
		Cast<UFlowGraphNode>(TargetPin.GetOwningNode())->RemoveOrphanedPin(&TargetPin);
	}
	else if (bSendsNodeNotification)
	{
		TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

int32 UFlowGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FFlowGraphUtils::GetFlowAssetEditor(Graph)->GetNumberOfSelectedNodes();
}

TSharedPtr<FEdGraphSchemaAction> UFlowGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FFlowGraphSchemaAction_NewComment));
}

void UFlowGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("CreateFlowRerouteNodeOnWire", "Create Flow Reroute Node"));

	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();
	UFlowGraphNode* NewReroute = FFlowGraphSchemaAction_NewNode::CreateNode(ParentGraph, nullptr, UFlowNode_Reroute::StaticClass(), KnotTopLeft, false);

	PinA->BreakLinkTo(PinB);
	PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
	PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
}

TArray<TSharedPtr<FString>> UFlowGraphSchema::GetFlowNodeCategories()
{
	if (NativeFlowNodes.Num() == 0)
	{
		GatherFlowNodes();
	}

	TSet<FString> UnsortedCategories;
	for (const UClass* FlowNodeClass : NativeFlowNodes)
	{
		if (const UFlowNode* DefaultObject = FlowNodeClass->GetDefaultObject<UFlowNode>())
		{
			UnsortedCategories.Emplace(DefaultObject->GetNodeCategory());
		}
	}

	for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeBlueprint(AssetData.Value))
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

UClass* UFlowGraphSchema::GetAssignedGraphNodeClass(const UClass* FlowNodeClass)
{
	if (UClass* AssignedGraphNode = AssignedGraphNodeClasses.FindRef(FlowNodeClass))
	{
		return AssignedGraphNode;
	}

	return UFlowGraphNode::StaticClass();
}

bool UFlowGraphSchema::IsClassContained(const TArray<TSubclassOf<UFlowNode>> Classes, const UClass* Class)
{
	for (const UClass* CurrentClass : Classes)
	{
		if (Class->IsChildOf(CurrentClass))
		{
			return true;
		}
	}

	return false;
}

void UFlowGraphSchema::GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UFlowAsset* AssetClassDefaults, const FString& CategoryName)
{
	if (NativeFlowNodes.Num() == 0)
	{
		GatherFlowNodes();
	}

	TArray<UFlowNode*> FlowNodes;
	FlowNodes.Reserve(NativeFlowNodes.Num() + BlueprintFlowNodes.Num());

	for (const UClass* FlowNodeClass : NativeFlowNodes)
	{
		// Flow Asset type might limit which nodes are placeable 
		if (IsClassContained(AssetClassDefaults->DeniedNodeClasses, FlowNodeClass))
		{
			continue;
		}

		if (IsClassContained(AssetClassDefaults->AllowedNodeClasses, FlowNodeClass))
		{
			FlowNodes.Emplace(FlowNodeClass->GetDefaultObject<UFlowNode>());
		}
	}
	for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeBlueprint(AssetData.Value))
		{
			for (const UClass* AllowedClass : AssetClassDefaults->AllowedNodeClasses)
			{
				if (Blueprint->GeneratedClass->IsChildOf(AllowedClass))
				{
					FlowNodes.Emplace(Blueprint->GeneratedClass->GetDefaultObject<UFlowNode>());
				}
			}
		}
	}
	FlowNodes.Shrink();

	for (const UFlowNode* FlowNode : FlowNodes)
	{
		if ((CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->GetNodeCategory())) && !UFlowGraphSettings::Get()->NodesHiddenFromPalette.Contains(FlowNode->GetClass()))
		{
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(FlowNode));
			ActionMenuBuilder.AddAction(NewNodeAction);
		}
	}
}

void UFlowGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph /*= nullptr*/)
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FFlowGraphUtils::GetFlowAssetEditor(CurrentGraph)->GetNumberOfSelectedNodes() > 0) : false;
		const FText MenuDescription = bIsManyNodesSelected ? LOCTEXT("CreateCommentAction", "Create Comment from Selection") : LOCTEXT("AddCommentAction", "Add Comment...");
		const FText ToolTip = LOCTEXT("CreateCommentToolTip", "Creates a comment.");

		const TSharedPtr<FFlowGraphSchemaAction_NewComment> NewAction(new FFlowGraphSchemaAction_NewComment(FText::GetEmpty(), MenuDescription, ToolTip, 0));
		ActionMenuBuilder.AddAction(NewAction);
	}
}

bool UFlowGraphSchema::IsFlowNodePlaceable(const UClass* Class)
{
	if (Class->HasAnyClassFlags(CLASS_Abstract) || Class->HasAnyClassFlags(CLASS_NotPlaceable) || Class->HasAnyClassFlags(CLASS_Deprecated))
	{
		return false;
	}

	if (const UFlowNode* DefaultObject = Class->GetDefaultObject<UFlowNode>())
	{
		return !DefaultObject->bNodeDeprecated;
	}

	return true;
}

void UFlowGraphSchema::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	if (Blueprint && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UFlowNode::StaticClass()))
	{
		bBlueprintCompilationPending = true;
	}
}

void UFlowGraphSchema::OnBlueprintCompiled()
{
	if (bBlueprintCompilationPending)
	{
		GatherFlowNodes();
	}

	bBlueprintCompilationPending = false;
}

void UFlowGraphSchema::GatherFlowNodes()
{
	// prevent asset crunching during PIE
	if (GEditor && GEditor->PlayWorld)
	{
		return;
	}

	// collect C++ nodes once per editor session
	if (NativeFlowNodes.Num() == 0)
	{
		TArray<UClass*> FlowNodes;
		GetDerivedClasses(UFlowNode::StaticClass(), FlowNodes);
		for (UClass* Class : FlowNodes)
		{
			if (Class->ClassGeneratedBy == nullptr && IsFlowNodePlaceable(Class))
			{
				NativeFlowNodes.Emplace(Class);
			}
		}

		TArray<UClass*> GraphNodes;
		GetDerivedClasses(UFlowGraphNode::StaticClass(), GraphNodes);
		for (UClass* Class : GraphNodes)
		{
			const UFlowGraphNode* DefaultObject = Class->GetDefaultObject<UFlowGraphNode>();
			for (UClass* AssignedClass : DefaultObject->AssignedNodeClasses)
			{
				if (AssignedClass->IsChildOf(UFlowNode::StaticClass()))
				{
					AssignedGraphNodeClasses.Emplace(AssignedClass, Class);
				}
			}
		}
	}

	// retrieve all blueprint nodes
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	FARFilter Filter;
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UBlueprintGeneratedClass::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FoundAssets;
	AssetRegistryModule.Get().GetAssets(Filter, FoundAssets);
	for (const FAssetData& AssetData : FoundAssets)
	{
		AddAsset(AssetData, true);
	}

	OnNodeListChanged.Broadcast();
}

void UFlowGraphSchema::OnHotReload(bool bWasTriggeredAutomatically)
{
	GatherFlowNodes();
}

void UFlowGraphSchema::OnAssetAdded(const FAssetData& AssetData)
{
	AddAsset(AssetData, false);
}

void UFlowGraphSchema::AddAsset(const FAssetData& AssetData, const bool bBatch)
{
	if (!BlueprintFlowNodes.Contains(AssetData.PackageName))
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		if (AssetRegistryModule.Get().IsLoadingAssets())
		{
			return;
		}

		TArray<FName> AncestorClassNames;
		AssetRegistryModule.Get().GetAncestorClassNames(AssetData.AssetClass, AncestorClassNames);
		if (!AncestorClassNames.Contains(UBlueprintCore::StaticClass()->GetFName()))
		{
			return;
		}

		FString NativeParentClassPath;
		AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath);
		if (!NativeParentClassPath.IsEmpty())
		{
			UObject* Outer = nullptr;
			ResolveName(Outer, NativeParentClassPath, false, false);
			const UClass* NativeParentClass = FindObject<UClass>(ANY_PACKAGE, *NativeParentClassPath);

			// accept only Flow Node blueprints
			if (NativeParentClass && NativeParentClass->IsChildOf(UFlowNode::StaticClass()))
			{
				BlueprintFlowNodes.Emplace(AssetData.PackageName, AssetData);

				if (!bBatch)
				{
					OnNodeListChanged.Broadcast();
				}
			}
		}
	}
}

void UFlowGraphSchema::OnAssetRemoved(const FAssetData& AssetData)
{
	if (BlueprintFlowNodes.Contains(AssetData.PackageName))
	{
		BlueprintFlowNodes.Remove(AssetData.PackageName);
		BlueprintFlowNodes.Shrink();

		OnNodeListChanged.Broadcast();
	}
}

UBlueprint* UFlowGraphSchema::GetPlaceableNodeBlueprint(const FAssetData& AssetData)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
	if (Blueprint && IsFlowNodePlaceable(Blueprint->GeneratedClass))
	{
		return Blueprint;
	}

	return nullptr;
}

const UFlowAsset* UFlowGraphSchema::GetAssetClassDefaults(const UEdGraph* Graph)
{
	const UClass* AssetClass = UFlowAsset::StaticClass();

	if (Graph)
	{
		if (const UFlowAsset* FlowAsset = Graph->GetTypedOuter<UFlowAsset>())
		{
			AssetClass = FlowAsset->GetClass();
		}
	}

	return AssetClass->GetDefaultObject<UFlowAsset>();
}

#undef LOCTEXT_NAMESPACE
