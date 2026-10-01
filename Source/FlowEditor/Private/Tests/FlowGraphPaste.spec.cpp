// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "AddOns/FlowNodeAddOn_PredicateNOT.h"
#include "Asset/FlowAssetEditor.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/Route/FlowNode_Branch.h"
#include "Nodes/Route/FlowNode_Reroute.h"

namespace FlowGraphPasteTests
{
using FAssetClasses = TArray<TSubclassOf<UFlowAsset>>;

FAssetClasses& GetBranchDeniedAssetClasses()
{
	const FArrayProperty* Property = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("DeniedAssetClasses"));
	check(Property);
	return *Property->ContainerPtrToValuePtr<FAssetClasses>(GetMutableDefault<UFlowNode_Branch>());
}

class STestFlowGraphEditor : public SFlowGraphEditor
{
public:
	void Construct(const FArguments& Arguments, UFlowAsset* Asset, TSharedRef<FFlowAssetEditor> AssetEditor)
	{
		FlowAsset = Asset;
		FlowAssetEditor = AssetEditor;
		bCanEditInPIE = false;
		SGraphEditor::Construct(SGraphEditor::FArguments().GraphToEdit(Asset->GetGraph()));
	}

	virtual bool IsTabFocused() const override { return true; }
	using SFlowGraphEditor::CopySelectedNodes;
};

struct FTestGraph
{
	TStrongObjectPtr<UFlowAsset> Asset{NewObject<UFlowAsset>(GetTransientPackage(), NAME_None, RF_Transactional)};
	TSharedRef<FFlowAssetEditor> AssetEditor = MakeShared<FFlowAssetEditor>();
	UFlowGraph* Graph = nullptr;
	TSharedPtr<STestFlowGraphEditor> Editor;

	FTestGraph()
	{
		// The toolkit destructor expects a matching editor-subsystem registration.
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->NotifyAssetOpened(Asset.Get(), &AssetEditor.Get());
		UFlowGraph::CreateGraph(Asset.Get(), UFlowGraphSchema::StaticClass());
		Graph = CastChecked<UFlowGraph>(Asset->GetGraph());
		// Remove schema defaults so each paste test starts with an empty graph and asset.
		TArray<UEdGraphNode*> DefaultNodes = Graph->Nodes;
		for (UEdGraphNode* DefaultNode : DefaultNodes)
		{
			if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(DefaultNode))
			{
				Asset->UnregisterNode(FlowGraphNode->NodeGuid);
			}
			DefaultNode->DestroyNode();
		}
		Editor = SNew(STestFlowGraphEditor, Asset.Get(), AssetEditor);
	}

	UFlowGraphNode* AddNode(UClass* NodeClass)
	{
		UFlowGraphNode* Node = NewObject<UFlowGraphNode>(Graph, NAME_None, RF_Transactional);
		Node->CreateNewGuid();
		Node->SetNodeTemplate(Asset->CreateNode(NodeClass, Node));
		Graph->AddNode(Node, false, false);
		Node->AllocateDefaultPins();
		return Node;
	}

	UFlowGraphNode* AddSubNode(UFlowGraphNode* Parent, UClass* AddOnClass)
	{
		UFlowGraphNode* Node = NewObject<UFlowGraphNode>(Graph, NAME_None, RF_Transactional);
		Node->SetNodeTemplate(NewObject<UFlowNodeAddOn>(Asset.Get(), AddOnClass, NAME_None, RF_Transactional));
		Parent->AddSubNode(Node, Graph);
		return Node;
	}

	void Copy(const TArray<UFlowGraphNode*>& Nodes)
	{
		Editor->ClearSelectionSet();
		for (UFlowGraphNode* Node : Nodes)
		{
			Editor->SetNodeSelection(Node, true);
		}
		Editor->CopySelectedNodes();
	}
};
}

BEGIN_DEFINE_SPEC(FFlowGraphPasteSpec, "FlowEditor.Paste",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	FString OriginalClipboard;
END_DEFINE_SPEC(FFlowGraphPasteSpec)

void FFlowGraphPasteSpec::Define()
{
	using namespace FlowGraphPasteTests;

	BeforeEach([this]() { FPlatformApplicationMisc::ClipboardPaste(OriginalClipboard); });
	AfterEach([this]() { FPlatformApplicationMisc::ClipboardCopy(*OriginalClipboard); });

	It("AvailabilityDoesNotImportIncompatibleNodes", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Node = Source.AddNode(UFlowNode_Branch::StaticClass());
		TGuardValue<FAssetClasses> DeniedClasses(GetBranchDeniedAssetClasses(), {UFlowAsset::StaticClass()});
		Source.Copy({Node});
		TestFalse(TEXT("The copied runtime type is incompatible"), Node->CanPasteHere(Target.Graph));

		TArray<UObject*> ObjectsBefore;
		GetObjectsWithOuter(Target.Asset.Get(), ObjectsBefore);
		for (int32 PollIndex = 0; PollIndex < 5; ++PollIndex)
		{
			TestTrue(TEXT("Availability allows syntactically importable nodes"), Target.Editor->CanPasteNodes());
		}
		TArray<UObject*> ObjectsAfter;
		GetObjectsWithOuter(Target.Asset.Get(), ObjectsAfter);
		TestEqual(TEXT("Polling creates no objects in the destination"), ObjectsAfter.Num(), ObjectsBefore.Num());
		TestTrue(TEXT("Polling leaves the graph empty"), Target.Graph->Nodes.IsEmpty());
		TestTrue(TEXT("Polling registers no runtime nodes"), Target.Asset->GetNodes().IsEmpty());
	});

	It("PasteReleasesUpdateLockWhenAllNodesAreRejected", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Node = Source.AddNode(UFlowNode_Branch::StaticClass());
		TGuardValue<FAssetClasses> DeniedClasses(GetBranchDeniedAssetClasses(), {UFlowAsset::StaticClass()});
		Source.Copy({Node});
		TestFalse(TEXT("The copied runtime type is incompatible"), Node->CanPasteHere(Target.Graph));
		TestTrue(TEXT("The paste command remains available"), Target.Editor->CanPasteNodes());

		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);
		TestFalse(TEXT("An entirely rejected paste releases the graph update lock"), Target.Graph->IsLocked());
		TestTrue(TEXT("No rejected graph nodes remain"), Target.Graph->Nodes.IsEmpty());
		TestTrue(TEXT("No rejected runtime nodes are registered"), Target.Asset->GetNodes().IsEmpty());
	});

	It("PasteKeepsValidNodesAndDropsRejectedParentTrees", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* RejectedParent = Source.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* Child = Source.AddSubNode(RejectedParent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.AddSubNode(Child, UFlowNodeAddOn_PredicateNOT::StaticClass());
		UFlowGraphNode* ValidNode = Source.AddNode(UFlowNode_Reroute::StaticClass());
		TGuardValue<FAssetClasses> DeniedClasses(GetBranchDeniedAssetClasses(), {UFlowAsset::StaticClass()});
		Source.Copy({RejectedParent, ValidNode});

		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);
		TestEqual(TEXT("Only the compatible node is pasted"), Target.Graph->Nodes.Num(), 1);
		TestEqual(TEXT("Only the compatible runtime node is registered"), Target.Asset->GetNodes().Num(), 1);
		TestFalse(TEXT("Paste releases the graph update lock"), Target.Graph->IsLocked());
		TArray<UObject*> Objects;
		GetObjectsWithOuter(Target.Graph, Objects);
		for (UObject* Object : Objects)
		{
			if (const UFlowGraphNode* Node = Cast<UFlowGraphNode>(Object))
			{
				TestFalse(TEXT("Rejected descendants are removed from the graph outer"), Node->IsSubNode());
			}
		}
	});

	It("PasteRejectsAddOnWithoutACompatibleSelectedParent", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Parent = Source.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* Child = Source.AddSubNode(Parent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.Copy({Child});
		UFlowGraphNode* IncompatibleTarget = Target.AddNode(UFlowNode_Reroute::StaticClass());
		Target.Editor->SetNodeSelection(IncompatibleTarget, true);
		TestTrue(TEXT("Availability does not reject an incompatible attachment"), Target.Editor->CanPasteNodes());
		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);
		TestTrue(TEXT("The incompatible parent has no pasted add-ons"), IncompatibleTarget->SubNodes.IsEmpty());
		TestEqual(TEXT("An add-on is never left on the canvas"), Target.Graph->Nodes.Num(), 1);

		Target.Editor->ClearSelectionSet();
		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);
		TestEqual(TEXT("Pasting without a selected parent leaves the graph unchanged"), Target.Graph->Nodes.Num(), 1);
	});

	It("PastePreservesACompatibleAddOnTree", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Parent = Source.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* Child = Source.AddSubNode(Parent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.AddSubNode(Child, UFlowNodeAddOn_PredicateNOT::StaticClass());
		Source.Copy({Child});
		UFlowGraphNode* CompatibleTarget = Target.AddNode(UFlowNode_Branch::StaticClass());
		Target.Editor->SetNodeSelection(CompatibleTarget, true);
		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);
		TestEqual(TEXT("The root add-on is attached"), CompatibleTarget->SubNodes.Num(), 1);
		if (CompatibleTarget->SubNodes.Num() == 1)
		{
			TestEqual(TEXT("The nested add-on is preserved"), CompatibleTarget->SubNodes[0]->SubNodes.Num(), 1);
		}
		TestEqual(TEXT("Only the parent is on the canvas"), Target.Graph->Nodes.Num(), 1);
	});

	It("PasteValidatesSiblingLimitsAsABatch", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Parent = Source.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* FirstChild = Source.AddSubNode(Parent, UFlowNodeAddOn_PredicateAND::StaticClass());
		UFlowGraphNode* SecondChild = Source.AddSubNode(Parent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.Copy({FirstChild, SecondChild});
		UFlowGraphNode* TargetParent = Target.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* SingleChildTarget = Target.AddSubNode(TargetParent, UFlowNodeAddOn_PredicateNOT::StaticClass());
		Target.Editor->SetNodeSelection(SingleChildTarget, true);
		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);
		TestTrue(TEXT("A one-child add-on rejects the two-child batch"), SingleChildTarget->SubNodes.IsEmpty());
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
