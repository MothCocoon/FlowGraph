// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "AddOns/FlowNodeAddOn_PredicateNOT.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/Route/FlowNode_Branch.h"

BEGIN_DEFINE_SPEC(FFlowGraphOrphanedNodesSpec, "FlowEditor.RemoveOrphanedNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FFlowGraphOrphanedNodesSpec)

void FFlowGraphOrphanedNodesSpec::Define()
{
	It("KeepsInstancesOfTheWholeAddOnTreeAndRemovesRealOrphans", [this]()
	{
		TStrongObjectPtr<UFlowAsset> Asset(NewObject<UFlowAsset>(GetTransientPackage(), NAME_None, RF_Transactional));
		UFlowGraph::CreateGraph(Asset.Get(), UFlowGraphSchema::StaticClass(), /*bCreateDefaultNodes=*/false);
		UFlowGraph* Graph = CastChecked<UFlowGraph>(Asset->GetGraph());

		UFlowGraphNode* Node = NewObject<UFlowGraphNode>(Graph, NAME_None, RF_Transactional);
		Node->CreateNewGuid();
		Node->SetNodeTemplate(Asset->CreateNode(UFlowNode_Branch::StaticClass(), Node));
		Graph->AddNode(Node, false, false);
		Node->AllocateDefaultPins();

		UFlowGraphNode* AddOn = NewObject<UFlowGraphNode>(Graph, NAME_None, RF_Transactional);
		AddOn->SetNodeTemplate(NewObject<UFlowNodeAddOn>(Asset.Get(), UFlowNodeAddOn_PredicateAND::StaticClass(), NAME_None, RF_Transactional));
		Node->AddSubNode(AddOn);

		UFlowGraphNode* NestedAddOn = NewObject<UFlowGraphNode>(Graph, NAME_None, RF_Transactional);
		NestedAddOn->SetNodeTemplate(NewObject<UFlowNodeAddOn>(Asset.Get(), UFlowNodeAddOn_PredicateNOT::StaticClass(), NAME_None, RF_Transactional));
		AddOn->AddSubNode(NestedAddOn);

		// Outered to the asset but referenced by no graph node.
		const UFlowNodeAddOn* Orphan = NewObject<UFlowNodeAddOn>(Asset.Get(), UFlowNodeAddOn_PredicateAND::StaticClass(), NAME_None, RF_Transactional);

		Graph->RemoveOrphanedNodes();

		TestTrue(TEXT("Node instance stays in the asset"), Node->GetFlowNodeBase()->GetOuter() == Asset.Get());
		TestTrue(TEXT("AddOn instance stays in the asset"), AddOn->GetFlowNodeBase()->GetOuter() == Asset.Get());
		TestTrue(TEXT("Nested AddOn instance stays in the asset"), NestedAddOn->GetFlowNodeBase()->GetOuter() == Asset.Get());
		TestTrue(TEXT("Unreferenced instance leaves the asset"), Orphan->GetOuter() != Asset.Get());
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
