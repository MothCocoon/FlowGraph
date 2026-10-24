// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Find/FlowSearch.h"

#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/Developer/FlowNode_Log.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "UObject/UnrealType.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFlowSearchSpec, "FlowEditor.Search", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TArray<UFlowAsset*> TestAssets;

	UFlowAsset* CreateAsset()
	{
		UFlowAsset* Asset = NewObject<UFlowAsset>(GetTransientPackage());
		UFlowGraph::CreateGraph(Asset);
		TestAssets.Add(Asset);
		return Asset;
	}

	UFlowGraphNode* AddNode(UFlowAsset& Asset, const UClass* NodeClass, const FString& Comment = FString())
	{
		UFlowGraph* Graph = Cast<UFlowGraph>(Asset.GetGraph());
		if (!Graph)
		{
			return nullptr;
		}

		UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(Graph);
		GraphNode->CreateNewGuid();
		GraphNode->SetNodeTemplate(Asset.CreateNode(NodeClass, GraphNode));
		GraphNode->NodeComment = Comment;
		Graph->AddNode(GraphNode, false, false);
		GraphNode->AllocateDefaultPins();
		return GraphNode;
	}
END_DEFINE_SPEC(FFlowSearchSpec)

void FFlowSearchSpec::Define()
{
	Describe("FlowSearch", [this]()
	{
		AfterEach([this]()
		{
			for (UFlowAsset* Asset : TestAssets)
			{
				if (IsValid(Asset))
				{
					Asset->ClearFlags(RF_Standalone);
					Asset->MarkAsGarbage();
				}
			}
			TestAssets.Empty();
		});

		It("requires every search token to match", [this]()
		{
			UFlowAsset* Asset = CreateAsset();
			if (!TestNotNull("Asset should be created", Asset)) { return; }
			AddNode(*Asset, UFlowNode_Log::StaticClass(), TEXT("Hello World Marker"));

			FFlowSearchQuery Query;
			Query.SearchText = TEXT("Hello World");
			Query.Flags = EFlowSearchFlags::Comments;
			Query.Scope = EFlowSearchScope::ThisAssetOnly;
			Query.ContextAsset = Asset;

			TArray<FFlowSearchResultItem> Results;
			TestTrue("Both comment tokens should match", FFlowSearch::Search(Query, Results));
			TestEqual("One node should match", Results.Num(), 1);

			Query.SearchText = TEXT("Hello Missing");
			Results.Reset();
			TestFalse("A missing token should fail the AND match", FFlowSearch::Search(Query, Results));
			TestTrue("A failed match should return no results", Results.IsEmpty());
		});

		It("searches property values when enabled", [this]()
		{
			UFlowAsset* Asset = CreateAsset();
			if (!TestNotNull("Asset should be created", Asset)) { return; }
			UFlowGraphNode* GraphNode = AddNode(*Asset, UFlowNode_Log::StaticClass());
			if (!TestNotNull("Graph node should be created", GraphNode)) { return; }

			FStrProperty* MessageProperty = FindFProperty<FStrProperty>(UFlowNode_Log::StaticClass(), TEXT("Message"));
			if (!TestNotNull("Log node should expose its Message property", MessageProperty)) { return; }
			MessageProperty->SetPropertyValue_InContainer(GraphNode->GetFlowNodeBase(), TEXT("PropertySearchMarker"));

			FFlowSearchQuery Query;
			Query.SearchText = TEXT("PropertySearchMarker");
			Query.Flags = EFlowSearchFlags::PropertyValues;
			Query.Scope = EFlowSearchScope::ThisAssetOnly;
			Query.ContextAsset = Asset;

			TArray<FFlowSearchResultItem> Results;
			TestTrue("Search should find an editable property value", FFlowSearch::Search(Query, Results));
		});

		It("recurses into subgraphs and terminates on cycles", [this]()
		{
			UFlowAsset* Parent = CreateAsset();
			UFlowAsset* Child = CreateAsset();
			if (!TestNotNull("Parent should be created", Parent) || !TestNotNull("Child should be created", Child)) { return; }

			UFlowGraphNode* ParentNode = AddNode(*Parent, UFlowNode_SubGraph::StaticClass());
			UFlowGraphNode* ChildNode = AddNode(*Child, UFlowNode_SubGraph::StaticClass(), TEXT("CycleMarker"));
			if (!TestNotNull("Parent subgraph node should be created", ParentNode) || !TestNotNull("Child subgraph node should be created", ChildNode)) { return; }
			CastChecked<UFlowNode_SubGraph>(ParentNode->GetFlowNodeBase())->SetAsset(Child);
			CastChecked<UFlowNode_SubGraph>(ChildNode->GetFlowNodeBase())->SetAsset(Parent);

			FFlowSearchQuery Query;
			Query.SearchText = TEXT("CycleMarker");
			Query.Flags = EFlowSearchFlags::Comments | EFlowSearchFlags::Subgraphs;
			Query.Scope = EFlowSearchScope::ThisAssetOnly;
			Query.ContextAsset = Parent;

			TArray<FFlowSearchResultItem> Results;
			TestTrue("A child result should be found despite the cycle", FFlowSearch::Search(Query, Results));
			TestEqual("The cycle should not duplicate the result", Results.Num(), 1);
		});

		It("applies pin direction and connection constraints", [this]()
		{
			UFlowAsset* Asset = CreateAsset();
			if (!TestNotNull("Asset should be created", Asset)) { return; }
			UFlowGraphNode* Source = AddNode(*Asset, UFlowNode_Log::StaticClass());
			UFlowGraphNode* Target = AddNode(*Asset, UFlowNode_Log::StaticClass());
			if (!TestNotNull("Source node should be created", Source) || !TestNotNull("Target node should be created", Target)) { return; }

			UEdGraphPin* Output = Source->FindPin(FName(TEXT("Out")), EGPD_Output);
			UEdGraphPin* Input = Target->FindPin(FName(TEXT("In")), EGPD_Input);
			if (!TestNotNull("Output pin should exist", Output) || !TestNotNull("Input pin should exist", Input)) { return; }
			Output->MakeLinkTo(Input);

			FFlowSearchQuery Query;
			Query.SearchText = TEXT("Out");
			Query.Flags = EFlowSearchFlags::PinNames;
			Query.Scope = EFlowSearchScope::ThisAssetOnly;
			Query.ContextAsset = Asset;
			Query.PinFilter.Direction = EFlowSearchPinDirection::Output;
			Query.PinFilter.ConnectionState = EFlowSearchPinConnectionState::Connected;

			TArray<FFlowSearchResultItem> Results;
			TestTrue("The connected output should be found", FFlowSearch::Search(Query, Results));
			TestEqual("Only the source node should match", Results.Num(), 1);
			if (Results.Num() == 1)
			{
				TestEqual("The matched result should belong to the source", Results[0].NodeGuid, Source->NodeGuid);
				TestEqual("Matched pin metadata should report output", Results[0].MatchedPins[0].Direction, EFlowSearchPinDirection::Output);
			}
		});
	});
}

#endif
