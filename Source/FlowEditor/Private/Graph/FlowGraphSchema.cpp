#include "Graph/FlowGraphSchema.h"

#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_Start.h"

#include "AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Misc/HotReloadInterface.h"
#include "ScopedTransaction.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "FlowGraphSchema"

TArray<UClass*> UFlowGraphSchema::NativeFlowNodes;
TMap<FName, FAssetData> UFlowGraphSchema::BlueprintFlowNodes;

TSet<FString> UFlowGraphSchema::UnsortedCategories;
TArray<TSharedPtr<FString>> UFlowGraphSchema::FlowNodeCategories;

TMap<UClass*, UClass*> UFlowGraphSchema::AssignedGraphNodeClasses;

FFlowGraphSchemaRefresh UFlowGraphSchema::OnNodeListChanged;

UFlowGraphSchema::UFlowGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowGraphSchema::SubscribeToAssetChanges()
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistry.Get().OnFilesLoaded().AddStatic(&UFlowGraphSchema::GatherFlowNodes);
	AssetRegistry.Get().OnAssetAdded().AddStatic(&UFlowGraphSchema::OnAssetAdded);
	AssetRegistry.Get().OnAssetRemoved().AddStatic(&UFlowGraphSchema::RemoveAsset);

	IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
	HotReloadSupport.OnHotReload().AddStatic(&UFlowGraphSchema::OnHotReload);

	if (GEditor)
	{
		GEditor->OnBlueprintCompiled().AddStatic(&UFlowGraphSchema::GatherFlowNodes);
		GEditor->OnClassPackageLoadedOrUnloaded().AddStatic(&UFlowGraphSchema::GatherFlowNodes);
	}
}

void UFlowGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName)
{
	GetFlowNodeActions(ActionMenuBuilder, CategoryName);
	GetCommentAction(ActionMenuBuilder);
}

void UFlowGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetFlowNodeActions(ContextMenuBuilder, FString());
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

TArray<TSharedPtr<FString>> UFlowGraphSchema::GetFlowNodeCategories()
{
	if (FlowNodeCategories.Num() == 0)
	{
		GatherFlowNodes();
	}

	return FlowNodeCategories;
}

UClass* UFlowGraphSchema::GetAssignedGraphNodeClass(const UClass* FlowNodeClass)
{
	if (UClass* AssignedGraphNode = AssignedGraphNodeClasses.FindRef(FlowNodeClass))
	{
		return AssignedGraphNode;
	}

	return UFlowGraphNode::StaticClass();
}

void UFlowGraphSchema::GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName)
{
	if (NativeFlowNodes.Num() == 0)
	{
		GatherFlowNodes();
	}

	for (UClass* FlowNodeClass : NativeFlowNodes)
	{
		const UFlowNode* FlowNode = FlowNodeClass->GetDefaultObject<UFlowNode>();
		if (CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->GetNativeCategory()))
		{
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(FlowNode));
			ActionMenuBuilder.AddAction(NewNodeAction);
		}
	}

	for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
	{
		UBlueprint* Blueprint = GetNodeBlueprint(AssetData.Value);
		if (CategoryName.IsEmpty() || CategoryName.Equals(Blueprint->BlueprintCategory))
		{
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(Blueprint));
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
	return !Class->HasAnyClassFlags(CLASS_Abstract) && !Class->HasAnyClassFlags(CLASS_NotPlaceable) && !Class->HasAnyClassFlags(CLASS_Deprecated);
}

void UFlowGraphSchema::GatherFlowNodes()
{
	if (GEditor && GEditor->PlayWorld)
	{
		// prevent heavy asset crunching during PIE
		return;
	}

	// collect C++ nodes once per editor session
	if (NativeFlowNodes.Num() == 0)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UFlowNode::StaticClass()))
			{
				if (It->ClassGeneratedBy == nullptr && IsFlowNodePlaceable(*It))
				{
					NativeFlowNodes.Emplace(*It);

					const UFlowNode* DefaultObject = It->GetDefaultObject<UFlowNode>();
					UnsortedCategories.Emplace(DefaultObject->GetNativeCategory());
				}
			}
			else if (It->IsChildOf(UFlowGraphNode::StaticClass()))
			{
				const UFlowGraphNode* DefaultObject = It->GetDefaultObject<UFlowGraphNode>();
				for (UClass* AssignedClass : DefaultObject->AssignedNodeClasses)
				{
					if (AssignedClass->IsChildOf(UFlowNode::StaticClass()))
					{
						AssignedGraphNodeClasses.Emplace(AssignedClass, *It);
					}
				}
			}
		}
	}

	// retrieve all blueprint nodes
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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

	RefreshNodeList();
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
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
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
			UClass* NativeParentClass = FindObject<UClass>(ANY_PACKAGE, *NativeParentClassPath);

			// accept only Flow Node blueprints
			if (NativeParentClass && NativeParentClass->IsChildOf(UFlowNode::StaticClass()))
			{
				UBlueprint* Blueprint = GetNodeBlueprint(AssetData);
				if (Blueprint && IsFlowNodePlaceable(Blueprint->GeneratedClass))
				{
					BlueprintFlowNodes.Emplace(AssetData.PackageName, AssetData);
					UnsortedCategories.Emplace(Blueprint->BlueprintCategory);

					if (!bBatch)
					{
						RefreshNodeList();
					}
				}
			}
		}
	}
}

void UFlowGraphSchema::RemoveAsset(const FAssetData& AssetData)
{
	if (BlueprintFlowNodes.Contains(AssetData.PackageName))
	{
		BlueprintFlowNodes.Remove(AssetData.PackageName);
		BlueprintFlowNodes.Shrink();

		RefreshNodeList();
	}
}

void UFlowGraphSchema::RefreshNodeList()
{
	// sort categories
	TArray<FString> SortedCategories = UnsortedCategories.Array();
	SortedCategories.Sort();

	// create list of categories
	FlowNodeCategories.Empty();
	for (const FString& Category : SortedCategories)
	{
		FlowNodeCategories.Emplace(MakeShareable(new FString(Category)));
	}

	OnNodeListChanged.Broadcast();
}

UBlueprint* UFlowGraphSchema::GetNodeBlueprint(const FAssetData& AssetData)
{
	return Cast<UBlueprint>(StaticLoadObject(AssetData.GetClass(), /*Outer =*/nullptr, *AssetData.ObjectPath.ToString(), nullptr, LOAD_NoWarn | LOAD_DisableCompileOnLoad));
}

#undef LOCTEXT_NAMESPACE
