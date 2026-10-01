// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphExporter.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_Reroute.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Misc/AutomationTest.h"
#include "EdGraph/EdGraphPin.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFlowGraphExporterSpec, "FlowGraphCourier.EditorGame.Exporter", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
	UFlowAsset* TestFlowAsset;
END_DEFINE_SPEC(FFlowGraphExporterSpec)

void FFlowGraphExporterSpec::Define()
{
	Describe("FlowGraphExporter", [this]()
	{
		BeforeEach([this]()
		{
			TestFlowAsset = NewObject<UFlowAsset>(GetTransientPackage(), UFlowAsset::StaticClass());
			// bCreateDefaultNodes=false: the 1-arg overload seeds a default Start node, which makes
			// every test's baseline graph non-empty and defeats "should handle empty flow asset"
			// below. Every other test here builds its own nodes explicitly, so nothing relies on it.
			UFlowGraph::CreateGraph(TestFlowAsset, UFlowGraphSchema::StaticClass(), /*bCreateDefaultNodes=*/ false);
		});

		AfterEach([this]()
		{
			if (TestFlowAsset)
			{
				TestFlowAsset->ClearFlags(RF_Standalone);
				TestFlowAsset->MarkAsGarbage();
				TestFlowAsset = nullptr;
			}
		});

		It("should export a simple flow graph with connected nodes", [this]()
		{
			UFlowGraph* FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
			if (!FlowGraph) { return; }

			UFlowGraphNode* StartGraphNode = NewObject<UFlowGraphNode>(FlowGraph);
			StartGraphNode->CreateNewGuid();
			UFlowNode* StartFlowNode = TestFlowAsset->CreateNode(UFlowNode_Reroute::StaticClass(), StartGraphNode);
			StartGraphNode->SetNodeTemplate(StartFlowNode);
			StartGraphNode->AllocateDefaultPins();
			FlowGraph->AddNode(StartGraphNode, false, false);

			UFlowGraphNode* FinishGraphNode = NewObject<UFlowGraphNode>(FlowGraph);
			FinishGraphNode->CreateNewGuid();
			UFlowNode* FinishFlowNode = TestFlowAsset->CreateNode(UFlowNode_Reroute::StaticClass(), FinishGraphNode);
			FinishGraphNode->SetNodeTemplate(FinishFlowNode);
			FinishGraphNode->AllocateDefaultPins();
			FlowGraph->AddNode(FinishGraphNode, false, false);

			if (StartGraphNode->OutputPins.Num() > 0 && FinishGraphNode->InputPins.Num() > 0)
			{
				UEdGraphPin* OutputPin = StartGraphNode->OutputPins[0];
				UEdGraphPin* InputPin = FinishGraphNode->InputPins[0];
				OutputPin->MakeLinkTo(InputPin);
			}

			FlowGraph->NotifyGraphChanged();
			TestFlowAsset->HarvestNodeConnections();

			FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

			TestTrue("Export should not be empty", !ExportedText.IsEmpty());
			TestTrue("Export should declare Courier v2's formatVersion", ExportedText.Contains(TEXT("\"formatVersion\": 2")));
			TestTrue("Export should contain UpsertNode ops", ExportedText.Contains(TEXT("\"kind\": \"UpsertNode\"")));
			TestTrue("Export should contain an AddConnection op", ExportedText.Contains(TEXT("\"kind\": \"AddConnection\"")));
		});

		It("should export asset metadata correctly", [this]()
		{
			TestFlowAsset->AssetGuid = FGuid::NewGuid();
			TestFlowAsset->bWorldBound = true;

			FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

			TestTrue("Export should contain bWorldBound", ExportedText.Contains(TEXT("\"bWorldBound\": true")));
			TestTrue("Export should contain expectedOwnerClass", ExportedText.Contains(TEXT("\"expectedOwnerClass\"")));
		});

		It("should export node properties", [this]()
		{
			UFlowGraph* FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
			if (!FlowGraph) { return; }

			UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(FlowGraph);
			GraphNode->CreateNewGuid();
			UFlowNode* FlowNode = TestFlowAsset->CreateNode(UFlowNode_Reroute::StaticClass(), GraphNode);
			GraphNode->SetNodeTemplate(FlowNode);
			GraphNode->AllocateDefaultPins();
			FlowGraph->AddNode(GraphNode, false, false);
			FlowGraph->NotifyGraphChanged();

			FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

			TestTrue("Export should contain an UpsertNode op", ExportedText.Contains(TEXT("\"kind\": \"UpsertNode\"")));
			TestTrue("Export should contain a type field", ExportedText.Contains(TEXT("\"type\":")));
			TestTrue("Export should contain a properties field", ExportedText.Contains(TEXT("\"properties\":")));
			TestTrue("Export should contain an inputPins field", ExportedText.Contains(TEXT("\"inputPins\":")));
			TestTrue("Export should contain an outputPins field", ExportedText.Contains(TEXT("\"outputPins\":")));
		});

		It("should handle empty flow asset", [this]()
		{
			FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

			TestTrue("Export should not be empty", !ExportedText.IsEmpty());
			TestTrue("Export should declare Courier v2's formatVersion", ExportedText.Contains(TEXT("\"formatVersion\": 2")));
			TestTrue("Export should contain an empty ops array", ExportedText.Contains(TEXT("\"ops\": []")));
		});

		It("should handle null flow asset gracefully", [this]()
		{
			FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(nullptr);

			TestTrue("Export should be empty for null asset", ExportedText.IsEmpty());
		});

		It("should export multiple connected nodes in sequence", [this]()
		{
			UFlowGraph* FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
			if (!FlowGraph) { return; }

			UFlowGraphNode* GraphNode1 = NewObject<UFlowGraphNode>(FlowGraph);
			GraphNode1->CreateNewGuid();
			UFlowNode* FlowNode1 = TestFlowAsset->CreateNode(UFlowNode_Reroute::StaticClass(), GraphNode1);
			GraphNode1->SetNodeTemplate(FlowNode1);
			GraphNode1->AllocateDefaultPins();
			FlowGraph->AddNode(GraphNode1, false, false);

			UFlowGraphNode* GraphNode2 = NewObject<UFlowGraphNode>(FlowGraph);
			GraphNode2->CreateNewGuid();
			UFlowNode* FlowNode2 = TestFlowAsset->CreateNode(UFlowNode_Reroute::StaticClass(), GraphNode2);
			GraphNode2->SetNodeTemplate(FlowNode2);
			GraphNode2->AllocateDefaultPins();
			FlowGraph->AddNode(GraphNode2, false, false);

			UFlowGraphNode* GraphNode3 = NewObject<UFlowGraphNode>(FlowGraph);
			GraphNode3->CreateNewGuid();
			UFlowNode* FlowNode3 = TestFlowAsset->CreateNode(UFlowNode_Reroute::StaticClass(), GraphNode3);
			GraphNode3->SetNodeTemplate(FlowNode3);
			GraphNode3->AllocateDefaultPins();
			FlowGraph->AddNode(GraphNode3, false, false);

			if (GraphNode1->OutputPins.Num() > 0 && GraphNode2->InputPins.Num() > 0)
			{
				GraphNode1->OutputPins[0]->MakeLinkTo(GraphNode2->InputPins[0]);
			}

			if (GraphNode2->OutputPins.Num() > 0 && GraphNode3->InputPins.Num() > 0)
			{
				GraphNode2->OutputPins[0]->MakeLinkTo(GraphNode3->InputPins[0]);
			}

			FlowGraph->NotifyGraphChanged();
			TestFlowAsset->HarvestNodeConnections();

			FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

			int32 ConnectionCount = 0;
			int32 SearchPos = 0;
			const FString AddConnectionMarker = TEXT("\"kind\": \"AddConnection\"");
			while ((SearchPos = ExportedText.Find(AddConnectionMarker, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchPos)) != INDEX_NONE)
			{
				ConnectionCount++;
				SearchPos += AddConnectionMarker.Len();
			}

			TestEqual("Export should contain two connections", ConnectionCount, 2);
		});
	});
}

#endif
