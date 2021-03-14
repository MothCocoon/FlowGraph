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
TMap<FString, UClass*> UFlowGraphSchema::BlueprintFlowNodes;
TArray<UClass*> UFlowGraphSchema::FlowNodeClasses;
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
	if (FlowNodeClasses.Num() == 0)
	{
		GatherFlowNodes();
	}

	for (UClass* FlowNodeClass : FlowNodeClasses)
	{
		const UFlowNode* FlowNode = FlowNodeClass->GetDefaultObject<UFlowNode>();
		if (CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->GetCategory()))
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
	return !Class->HasAnyClassFlags(CLASS_Abstract) && !Class->HasAnyClassFlags(CLASS_NotPlaceable) && !Class->HasAnyClassFlags(CLASS_Deprecated);
}

void UFlowGraphSchema::GatherFlowNodes()
{
	if (GEditor && GEditor->PlayWorld)
	{
		// prevent heavy asset crunching during PIE
		return;
	}

	FlowNodeClasses.Empty();

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
	FlowNodeClasses.Append(NativeFlowNodes);

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
			FString GeneratedClassPath;
			AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassPath);

			// add class once
			if (!GeneratedClassPath.IsEmpty())
			{
				UObject* NodeOuter = nullptr;
				ResolveName(NodeOuter, GeneratedClassPath, false, false);
				UClass* NodeClass = FindObject<UClass>(ANY_PACKAGE, *GeneratedClassPath);

				if (NodeClass == nullptr)
				{
					// this should be fine - assuming that Flow Node blueprints shouldn't include hard references to heavy assets!
					NodeClass = LoadObject<UClass>(nullptr, *GeneratedClassPath);
				}

				if (NodeClass && IsFlowNodePlaceable(NodeClass) && FlowNodeClasses.Contains(NodeClass) == false)
				{
					// filter out intermediate blueprint classes
					const FString ClassName = NodeClass->GetName();
					if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_")))
					{
						return;
					}

					BlueprintFlowNodes.Emplace(GeneratedClassPath, NodeClass);
					FlowNodeClasses.Emplace(NodeClass);
				}

				if (!bBatch)
				{
					RefreshNodeList();
				}
			}
		}
	}
}

void UFlowGraphSchema::RemoveAsset(const FAssetData& AssetData)
{
	FString GeneratedClassPath;
	if (AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassPath))
	{
		GeneratedClassPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPath);

		if (GeneratedClassPath == TEXT("None"))
		{
			// This can happen if the generated class was already deleted prior to the notification being sent
			// Let's try to reconstruct the generated class name from the object path.
			GeneratedClassPath = AssetData.ObjectPath.ToString() + TEXT("_C");
		}
	}

	if (BlueprintFlowNodes.Contains(GeneratedClassPath))
	{
		FlowNodeClasses.Remove(BlueprintFlowNodes[GeneratedClassPath]);
		FlowNodeClasses.Shrink();

		BlueprintFlowNodes.Remove(GeneratedClassPath);
		BlueprintFlowNodes.Compact();

		RefreshNodeList();
	}
}

void UFlowGraphSchema::RefreshNodeList()
{
	// sort node classes
	FlowNodeClasses.Sort();

	// collect categories
	TSet<FString> UnsortedCategories;
	for (UClass* FlowNodeClass : FlowNodeClasses)
	{
		const UFlowNode* DefaultObject = FlowNodeClass->GetDefaultObject<UFlowNode>();
		UnsortedCategories.Emplace(DefaultObject->GetCategory());
	}

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

int32 UFlowGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FFlowGraphUtils::GetFlowAssetEditor(Graph)->GetNumberOfSelectedNodes();
}

TSharedPtr<FEdGraphSchemaAction> UFlowGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FFlowGraphSchemaAction_NewComment));
}

#undef LOCTEXT_NAMESPACE
