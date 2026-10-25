// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Commands/UICommandList.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "AddOns/FlowNodeAddOn_PredicateNOT.h"
#include "Asset/FlowAssetEditor.h"
#include "FlowAsset.h"
#include "FlowEditorCommands.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Extraction/FlowSubGraphExtractionByGuid.h"
#include "Graph/Extraction/FlowSubGraphExtractionPlan.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/Developer/FlowNode_Log.h"
#include "Nodes/Route/FlowNode_Branch.h"
#include "Nodes/Route/FlowNode_ExecutionSequence.h"
#include "Nodes/Route/FlowNode_Reroute.h"
#include "Nodes/Route/FlowNode_Timer.h"

namespace FlowGraphPasteTests
{
using FAssetClasses = TArray<TSubclassOf<UFlowAsset>>;

FAssetClasses& GetDeniedAssetClasses(UFlowNode& Node)
{
	const FArrayProperty* Property = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("DeniedAssetClasses"));
	check(Property);
	return *Property->ContainerPtrToValuePtr<FAssetClasses>(&Node);
}

FAssetClasses& GetBranchDeniedAssetClasses()
{
	return GetDeniedAssetClasses(*GetMutableDefault<UFlowNode_Branch>());
}

// Native instances don't pick up CDO changes made after startup.
// A value differing from the CDO is exported as an empty array that fails to import on paste.
void MatchBranchDeniedAssetClasses(const UFlowGraphNode& Node)
{
	GetDeniedAssetClasses(*CastChecked<UFlowNode>(Node.GetFlowNodeBase())) = GetBranchDeniedAssetClasses();
}

// Shared by all test editors, so nodes copied in one graph can be pasted into another.
FString TestClipboard;

class STestFlowGraphEditor : public SFlowGraphEditor
{
public:
	void Construct(const FArguments& Arguments, UFlowAsset* Asset, TSharedRef<FFlowAssetEditor> AssetEditor)
	{
		FlowAsset = Asset;
		FlowAssetEditor = AssetEditor;
		SGraphEditor::Construct(SGraphEditor::FArguments().GraphToEdit(Asset->GetGraph()));
	}

	virtual bool IsTabFocused() const override { return true; }
	virtual void ClipboardCopy(const FString& Text) const override { TestClipboard = Text; }
	virtual void ClipboardPaste(FString& OutText) const override { OutText = TestClipboard; }
	using SFlowGraphEditor::CopySelectedNodes;
	using SFlowGraphEditor::DuplicateNodes;
	using SFlowGraphEditor::BindGraphCommands;
	using SFlowGraphEditor::CommandList;
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
		Parent->AddSubNode(Node);
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
END_DEFINE_SPEC(FFlowGraphPasteSpec)

void FFlowGraphPasteSpec::Define()
{
	using namespace FlowGraphPasteTests;

	BeforeEach([]() { TestClipboard.Reset(); });

	It("GraphCommandsBindRegisteredExtractionAction", [this]()
	{
		FTestGraph Source;
		Source.Editor->BindGraphCommands();
		const TSharedPtr<FUICommandInfo>& ExtractCommand = FFlowGraphCommands::Get().ExtractToSubGraph;
		if (!TestTrue(TEXT("The extraction command is registered"), ExtractCommand.IsValid()))
		{
			return;
		}

		TestNotNull(TEXT("The graph editor binds the extraction action"),
			Source.Editor->CommandList->GetActionForCommand(ExtractCommand));
	});

	It("GraphCreationHonorsDefaultNodePolicy", [this]()
	{
		TStrongObjectPtr<UFlowAsset> EmptyAsset(NewObject<UFlowAsset>());
		UFlowGraph::CreateGraph(EmptyAsset.Get(), UFlowGraphSchema::StaticClass(), /*bCreateDefaultNodes=*/false);
		TestTrue(TEXT("Graph creation without defaults leaves the graph empty"), EmptyAsset->GetGraph()->Nodes.IsEmpty());
		TestTrue(TEXT("Graph creation without defaults registers no runtime nodes"), EmptyAsset->GetNodes().IsEmpty());

		TStrongObjectPtr<UFlowAsset> DefaultAsset(NewObject<UFlowAsset>());
		UFlowGraph::CreateGraph(DefaultAsset.Get(), UFlowGraphSchema::StaticClass());
		TestFalse(TEXT("Ordinary graph creation still creates default graph nodes"), DefaultAsset->GetGraph()->Nodes.IsEmpty());
		TestFalse(TEXT("Ordinary graph creation still registers default runtime nodes"), DefaultAsset->GetNodes().IsEmpty());
	});

	It("CatalogReportsDeclaredPinsFromTheNodeClass", [this]()
	{
		const UFlowNode_Log* NodeCDO = GetDefault<UFlowNode_Log>();
		TArray<FFlowPin> InputPins;
		TArray<FFlowPin> OutputPins;
		NodeCDO->GetCatalogPins(InputPins, OutputPins);
		TestFalse(TEXT("A built-in node reports its input declarations"), InputPins.IsEmpty());
		TestFalse(TEXT("A built-in node reports its output declarations"), OutputPins.IsEmpty());
	});

	It("ExtractionByGuidRejectsUnknownNodesAndApplyRechecksTheLiveGraph", [this]()
	{
		FTestGraph Source;
		UFlowGraphNode* Entry = Source.AddNode(UFlowNode_Reroute::StaticClass());
		UFlowGraphNode* Sequence = Source.AddNode(UFlowNode_ExecutionSequence::StaticClass());
		UFlowGraphNode* Timer = Source.AddNode(UFlowNode_Timer::StaticClass());
		UFlowGraphNode* Log = Source.AddNode(UFlowNode_Log::StaticClass());
		UFlowGraphNode* TimerTarget = Source.AddNode(UFlowNode_Reroute::StaticClass());
		UFlowGraphNode* LogTarget = Source.AddNode(UFlowNode_Reroute::StaticClass());

		const UEdGraphSchema* Schema = Source.Graph->GetSchema();
		Schema->TryCreateConnection(Entry->FindPin(TEXT("Out"), EGPD_Output), Sequence->FindPin(TEXT("In"), EGPD_Input));
		Schema->TryCreateConnection(Sequence->FindPin(TEXT("0"), EGPD_Output), Timer->FindPin(TEXT("In"), EGPD_Input));
		Schema->TryCreateConnection(Sequence->FindPin(TEXT("1"), EGPD_Output), Log->FindPin(TEXT("In"), EGPD_Input));
		Schema->TryCreateConnection(Timer->FindPin(TEXT("Completed"), EGPD_Output), TimerTarget->FindPin(TEXT("In"), EGPD_Input));
		Schema->TryCreateConnection(Log->FindPin(TEXT("Out"), EGPD_Output), LogTarget->FindPin(TEXT("In"), EGPD_Input));

		TArray<FGuid> SelectionGuids{Sequence->NodeGuid, Timer->NodeGuid, Log->NodeGuid};
		FlowSubGraphExtraction::FExtractionPlanSummary Summary;
		FString Error;
		TestFalse(TEXT("An unknown node GUID is rejected"), FlowSubGraphExtraction::PlanExtractionByGuid(
			Source.Asset.Get(), {FGuid::NewGuid()}, FlowSubGraphExtraction::GetDefaultNewAssetName(), Summary, Error));
		TestTrue(TEXT("A valid live selection can be planned"), FlowSubGraphExtraction::PlanExtractionByGuid(
			Source.Asset.Get(), SelectionGuids, FlowSubGraphExtraction::GetDefaultNewAssetName(), Summary, Error));
		TestTrue(TEXT("The plan can extract the connected selection"), Summary.bCanApply);

		Entry->FindPin(TEXT("Out"), EGPD_Output)->BreakLinkTo(Sequence->FindPin(TEXT("In"), EGPD_Input));
		UFlowAsset* NewAsset = nullptr;
		UFlowGraphNode* NewSubGraphNode = nullptr;
		TestFalse(TEXT("A stale summary cannot authorize a changed graph"), FlowSubGraphExtraction::ApplyExtractionByGuidWithoutSave(
			Source.Asset.Get(), SelectionGuids, FlowSubGraphExtraction::GetDefaultNewAssetName(), Summary,
			NewAsset, NewSubGraphNode, Error));
		TestNull(TEXT("A stale plan creates no asset"), NewAsset);
		TestNull(TEXT("A stale plan creates no SubGraph node"), NewSubGraphNode);
	});

	It("ExtractionPreservesConcurrentExits", [this]()
	{
		FTestGraph Source;
		UFlowGraphNode* Entry = Source.AddNode(UFlowNode_Reroute::StaticClass());
		UFlowGraphNode* Sequence = Source.AddNode(UFlowNode_ExecutionSequence::StaticClass());
		UFlowGraphNode* Timer = Source.AddNode(UFlowNode_Timer::StaticClass());
		UFlowGraphNode* Log = Source.AddNode(UFlowNode_Log::StaticClass());
		UFlowGraphNode* TimerTarget = Source.AddNode(UFlowNode_Reroute::StaticClass());
		UFlowGraphNode* LogTarget = Source.AddNode(UFlowNode_Reroute::StaticClass());
		Log->NodePosY = 200;

		const UEdGraphSchema* Schema = Source.Graph->GetSchema();
		const auto Connect = [this, Schema](UFlowGraphNode* From, const FName OutputName, UFlowGraphNode* To)
		{
			return TestTrue(TEXT("The source execution connection is valid"), Schema->TryCreateConnection(
				From->FindPin(OutputName, EGPD_Output), To->FindPin(TEXT("In"), EGPD_Input)));
		};
		if (!Connect(Entry, TEXT("Out"), Sequence)
			|| !Connect(Sequence, TEXT("0"), Timer)
			|| !Connect(Sequence, TEXT("1"), Log)
			|| !Connect(Timer, TEXT("Completed"), TimerTarget)
			|| !Connect(Log, TEXT("Out"), LogTarget))
		{
			return;
		}

		const auto CheckExits = [this](const auto& Plan)
		{
			TestTrue(TEXT("A selection containing concurrent timer and immediate branches can be extracted"), Plan.CanApply());
			TestEqual(TEXT("Both branches retain their outgoing interfaces"), Plan.Exits.Num(), 2);
			TSet<FName> ExitNames;
			for (const auto& Exit : Plan.Exits)
			{
				TestFalse(TEXT("An ordinary exit must not finish the child and cancel sibling work"), Exit.bUsesBuiltInPin);
				TestNotEqual(TEXT("An ordinary exit must not use the reserved Finish interface"), Exit.InterfaceName, FName(TEXT("Finish")));
				ExitNames.Add(Exit.InterfaceName);
			}
			TestEqual(TEXT("Concurrent exits use distinct interfaces"), ExitNames.Num(), 2);
		};
		CheckExits(FlowSubGraphExtraction::PlanExtraction({Sequence, Timer, Log}));

		FlowSubGraphExtraction::FExtractionPlanSummary Summary;
		FString Error;
		TestTrue(TEXT("Courier can plan the same selection"), FlowSubGraphExtraction::PlanExtractionByGuid(
			Source.Asset.Get(), {Sequence->NodeGuid, Timer->NodeGuid, Log->NodeGuid},
			FlowSubGraphExtraction::GetDefaultNewAssetName(), Summary, Error));
		TestTrue(TEXT("Courier's extraction plan can be applied"), Summary.bCanApply);
		TestEqual(TEXT("Courier retains both concurrent outputs"), Summary.InterfaceOutputs.Num(), 2);
		TestFalse(TEXT("Courier does not turn an ordinary output into Finish"),
			Summary.InterfaceOutputs.Contains(TEXT("Finish")));
	});

	It("ExtractionByGuidRevalidatesBeforeApply", [this]()
	{
		FTestGraph Source;
		UFlowGraphNode* Entry = Source.AddNode(UFlowNode_Reroute::StaticClass());
		UFlowGraphNode* Selected = Source.AddNode(UFlowNode_Log::StaticClass());
		const UEdGraphSchema* Schema = Source.Graph->GetSchema();
		if (!TestTrue(TEXT("The selection has an incoming connection"), Schema->TryCreateConnection(
			Entry->FindPin(TEXT("Out"), EGPD_Output), Selected->FindPin(TEXT("In"), EGPD_Input))))
		{
			return;
		}

		FlowSubGraphExtraction::FExtractionPlanSummary Summary;
		FString Error;
		const FString AssetName = FlowSubGraphExtraction::GetDefaultNewAssetName();
		TestTrue(TEXT("Planning succeeds"), FlowSubGraphExtraction::PlanExtractionByGuid(
			Source.Asset.Get(), {Selected->NodeGuid}, AssetName, Summary, Error));
		TestTrue(TEXT("The initial plan can be applied"), Summary.bCanApply);
		Schema->BreakNodeLinks(*Entry);

		UFlowAsset* NewAsset = nullptr;
		UFlowGraphNode* SubgraphNode = nullptr;
		TestFalse(TEXT("A stale plan cannot bypass live boundary validation"), FlowSubGraphExtraction::ApplyExtractionByGuidWithoutSave(
			Source.Asset.Get(), {Selected->NodeGuid}, AssetName, Summary, NewAsset, SubgraphNode, Error));
		TestFalse(TEXT("The stale plan reports its error"), Error.IsEmpty());
		TestNull(TEXT("No asset is created for the stale plan"), NewAsset);
		TestEqual(TEXT("Rejected extraction preserves the source"), Source.Graph->Nodes.Num(), 2);
	});

	It("ExtractionByGuidExtractsNodesAndAddOns", [this]()
	{
		FTestGraph Source;
		const FString Folder = TEXT("/Game/FlowExtractionAutomation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		UPackage* SourcePackage = CreatePackage(*(Folder / TEXT("Source")));
		Source.Asset->Rename(TEXT("Source"), SourcePackage, REN_DontCreateRedirectors | REN_NonTransactional);
		UFlowGraphNode* Entry = Source.AddNode(UFlowNode_Reroute::StaticClass());
		UFlowGraphNode* Selected = Source.AddNode(UFlowNode_Branch::StaticClass());
		Source.AddSubNode(Selected, UFlowNodeAddOn_PredicateAND::StaticClass());
		UFlowGraphNode* Target = Source.AddNode(UFlowNode_Reroute::StaticClass());
		const UEdGraphSchema* Schema = Source.Graph->GetSchema();
		if (!TestTrue(TEXT("The entry is connected"), Schema->TryCreateConnection(
			Entry->FindPin(TEXT("Out"), EGPD_Output), Selected->FindPin(TEXT("Evaluate"), EGPD_Input)))
			|| !TestTrue(TEXT("The exit is connected"), Schema->TryCreateConnection(
				Selected->FindPin(TEXT("True"), EGPD_Output), Target->FindPin(TEXT("In"), EGPD_Input))))
		{
			return;
		}

		const FGuid SelectedGuid = Selected->NodeGuid;
		FlowSubGraphExtraction::FExtractionPlanSummary Summary;
		FString Error;
		const FString AssetName = FlowSubGraphExtraction::GetDefaultNewAssetName();
		TestTrue(TEXT("The adapter plans extraction"), FlowSubGraphExtraction::PlanExtractionByGuid(
			Source.Asset.Get(), {SelectedGuid}, AssetName, Summary, Error));
		UFlowAsset* NewAsset = nullptr;
		UFlowGraphNode* SubgraphNode = nullptr;
		bool bApplied = false;
		{
			FScopedTransaction Transaction(NSLOCTEXT("FlowGraphTests", "Extract", "Test subgraph extraction"));
			bApplied = FlowSubGraphExtraction::ApplyExtractionByGuidWithoutSave(
				Source.Asset.Get(), {SelectedGuid}, AssetName, Summary, NewAsset, SubgraphNode, Error);
			Transaction.Cancel();
		}
		TestTrue(*FString::Printf(TEXT("Extraction succeeds: %s"), *Error), bApplied);
		if (bApplied && TestNotNull(TEXT("Extraction creates a replacement node"), SubgraphNode))
		{
			TestNull(TEXT("The selected runtime node leaves the source"), Source.Asset->GetNode(SelectedGuid));
			UFlowNode* ClonedNode = NewAsset->GetNode(SelectedGuid);
			if (TestNotNull(TEXT("The new asset contains the selected runtime node"), ClonedNode))
			{
				const UFlowGraphNode* ClonedGraphNode = CastChecked<UFlowGraphNode>(ClonedNode->GetGraphNode());
				TestEqual(TEXT("The extracted node retains its add-on"), ClonedGraphNode->SubNodes.Num(), 1);
			}
			TestTrue(TEXT("The incoming connection targets the replacement"),
				Entry->FindPin(TEXT("Out"), EGPD_Output)->LinkedTo.Contains(
					SubgraphNode->FindPin(TEXT("Start"), EGPD_Input)));
			TestTrue(TEXT("The outgoing connection comes from the replacement"),
				Target->FindPin(TEXT("In"), EGPD_Input)->LinkedTo.Contains(
					SubgraphNode->FindPin(FName(*Summary.InterfaceOutputs[0]), EGPD_Output)));
		}
		if (NewAsset)
		{
			FAssetRegistryModule::AssetDeleted(NewAsset);
			NewAsset->GetOutermost()->SetDirtyFlag(false);
			NewAsset->ClearFlags(RF_Public | RF_Standalone);
			NewAsset->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}
		SourcePackage->SetDirtyFlag(false);
	});

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
		TGuardValue<FAssetClasses> DeniedClasses(GetBranchDeniedAssetClasses(), {UFlowAsset::StaticClass()});
		UFlowGraphNode* Node = Source.AddNode(UFlowNode_Branch::StaticClass());
		MatchBranchDeniedAssetClasses(*Node);
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
		TGuardValue<FAssetClasses> DeniedClasses(GetBranchDeniedAssetClasses(), {UFlowAsset::StaticClass()});
		UFlowGraphNode* RejectedParent = Source.AddNode(UFlowNode_Branch::StaticClass());
		MatchBranchDeniedAssetClasses(*RejectedParent);
		UFlowGraphNode* Child = Source.AddSubNode(RejectedParent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.AddSubNode(Child, UFlowNodeAddOn_PredicateNOT::StaticClass());
		UFlowGraphNode* ValidNode = Source.AddNode(UFlowNode_Reroute::StaticClass());
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

	It("PasteRejectsAddOnWithMultipleSelectedParents", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Parent = Source.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* Child = Source.AddSubNode(Parent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.Copy({Child});

		// Both targets would accept the add-on on their own.
		UFlowGraphNode* FirstTarget = Target.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* SecondTarget = Target.AddNode(UFlowNode_Branch::StaticClass());
		Target.Editor->SetNodeSelection(FirstTarget, true);
		Target.Editor->SetNodeSelection(SecondTarget, true);
		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);

		TestTrue(TEXT("The first selected node receives no add-ons"), FirstTarget->SubNodes.IsEmpty());
		TestTrue(TEXT("The second selected node receives no add-ons"), SecondTarget->SubNodes.IsEmpty());
		TestEqual(TEXT("An add-on is never left on the canvas"), Target.Graph->Nodes.Num(), 2);
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

	It("PasteKeepsAddOnTreesOfMultipleSelectedNodes", [this]()
	{
		FTestGraph Graph;
		UFlowGraphNode* FirstNode = Graph.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* FirstChild = Graph.AddSubNode(FirstNode, UFlowNodeAddOn_PredicateAND::StaticClass());
		Graph.AddSubNode(FirstChild, UFlowNodeAddOn_PredicateNOT::StaticClass());
		UFlowGraphNode* SecondNode = Graph.AddNode(UFlowNode_Branch::StaticClass());
		Graph.AddSubNode(SecondNode, UFlowNodeAddOn_PredicateAND::StaticClass());

		// Copy leaves both source nodes selected, as with Ctrl+C followed by Ctrl+V.
		Graph.Copy({FirstNode, SecondNode});
		TestTrue(TEXT("Paste stays available with several selected nodes owning add-ons"), Graph.Editor->CanPasteNodes());
		Graph.Editor->PasteNodesHere(FVector2f::ZeroVector);

		TestEqual(TEXT("Both copied nodes are pasted"), Graph.Graph->Nodes.Num(), 4);
		TestEqual(TEXT("The first selected node gets no extra add-ons"), FirstNode->SubNodes.Num(), 1);
		TestEqual(TEXT("The second selected node gets no extra add-ons"), SecondNode->SubNodes.Num(), 1);

		int32 PastedNodeCount = 0;
		int32 NestedAddOnCount = 0;
		for (UEdGraphNode* Node : Graph.Graph->Nodes)
		{
			const UFlowGraphNode* PastedNode = Cast<UFlowGraphNode>(Node);
			if (PastedNode && PastedNode != FirstNode && PastedNode != SecondNode)
			{
				++PastedNodeCount;
				TestEqual(TEXT("Each pasted node keeps its root add-on"), PastedNode->SubNodes.Num(), 1);
				if (PastedNode->SubNodes.Num() == 1)
				{
					NestedAddOnCount += PastedNode->SubNodes[0]->SubNodes.Num();
				}
			}
		}
		TestEqual(TEXT("Two pasted Flow nodes are found"), PastedNodeCount, 2);
		TestEqual(TEXT("The nested add-on is preserved"), NestedAddOnCount, 1);
	});

	It("DuplicateKeepsAddOnTreesOfMultipleSelectedNodes", [this]()
	{
		FTestGraph Graph;
		UFlowGraphNode* FirstNode = Graph.AddNode(UFlowNode_Branch::StaticClass());
		Graph.AddSubNode(FirstNode, UFlowNodeAddOn_PredicateAND::StaticClass());
		UFlowGraphNode* SecondNode = Graph.AddNode(UFlowNode_Branch::StaticClass());
		Graph.AddSubNode(SecondNode, UFlowNodeAddOn_PredicateAND::StaticClass());

		Graph.Editor->SetNodeSelection(FirstNode, true);
		Graph.Editor->SetNodeSelection(SecondNode, true);
		Graph.Editor->DuplicateNodes();

		TestEqual(TEXT("Both selected nodes are duplicated"), Graph.Graph->Nodes.Num(), 4);
		TestEqual(TEXT("The first selected node gets no extra add-ons"), FirstNode->SubNodes.Num(), 1);
		TestEqual(TEXT("The second selected node gets no extra add-ons"), SecondNode->SubNodes.Num(), 1);
		for (UEdGraphNode* Node : Graph.Graph->Nodes)
		{
			if (const UFlowGraphNode* DuplicatedNode = Cast<UFlowGraphNode>(Node))
			{
				TestEqual(TEXT("Every node keeps exactly one add-on"), DuplicatedNode->SubNodes.Num(), 1);
			}
		}
	});

	It("PasteAttachesAddOnSelectedBeforeItsParentToPastedParent", [this]()
	{
		FTestGraph Source;
		FTestGraph Target;
		UFlowGraphNode* Parent = Source.AddNode(UFlowNode_Branch::StaticClass());
		UFlowGraphNode* Child = Source.AddSubNode(Parent, UFlowNodeAddOn_PredicateAND::StaticClass());
		Source.AddSubNode(Child, UFlowNodeAddOn_PredicateNOT::StaticClass());

		// Selection keeps insertion order, so the add-on is visited before its parent during copy.
		Source.Copy({Child, Parent});

		// A single compatible selected node would receive the add-on if it were copied as a root add-on.
		UFlowGraphNode* SelectedTarget = Target.AddNode(UFlowNode_Branch::StaticClass());
		Target.Editor->SetNodeSelection(SelectedTarget, true);
		Target.Editor->PasteNodesHere(FVector2f::ZeroVector);

		TestTrue(TEXT("The selected target receives no add-ons"), SelectedTarget->SubNodes.IsEmpty());
		TestEqual(TEXT("Only the copied parent is added to the canvas"), Target.Graph->Nodes.Num(), 2);

		for (UEdGraphNode* Node : Target.Graph->Nodes)
		{
			const UFlowGraphNode* PastedParent = Cast<UFlowGraphNode>(Node);
			if (PastedParent && PastedParent != SelectedTarget)
			{
				TestEqual(TEXT("The add-on is attached to the pasted parent"), PastedParent->SubNodes.Num(), 1);
				if (PastedParent->SubNodes.Num() == 1)
				{
					TestEqual(TEXT("The nested add-on is preserved"), PastedParent->SubNodes[0]->SubNodes.Num(), 1);
				}
			}
		}
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
