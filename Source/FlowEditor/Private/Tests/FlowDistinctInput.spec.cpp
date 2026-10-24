// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Tests/FlowDistinctInputTestTypes.h"

#include "EdGraphUtilities.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Types/FlowPinTypesStandard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDistinctInputTestTypes)

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace FlowDistinctInputTests
{
	struct FFixture
	{
		TStrongObjectPtr<UFlowAsset> Asset{NewObject<UFlowAsset>()};
		UFlowGraph* Graph = nullptr;
		UFlowGraphNode* SinkGraphNode = nullptr;
		UFlowDistinctInputTestNode* Sink = nullptr;
		UFlowDistinctInputTestAddOn* First = nullptr;
		UFlowDistinctInputTestAddOn* Second = nullptr;

		UFlowGraphNode* AddNode(int32 Role)
		{
			UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(Graph);
			GraphNode->CreateNewGuid();
			Graph->AddNode(GraphNode, false, false);
			UFlowDistinctInputTestNode* Node = NewObject<UFlowDistinctInputTestNode>(Asset.Get());
			Node->Role = Role;
			Node->Value.Values = {Role};
			Asset->RegisterNode(GraphNode->NodeGuid, Node);
			GraphNode->SetNodeTemplate(Node);
			GraphNode->MarkNeedsFullReconstruction();
			GraphNode->ReconstructNode();
			return GraphNode;
		}

		FFixture()
		{
			Graph = NewObject<UFlowGraph>(Asset.Get());
			Graph->Schema = UFlowGraphSchema::StaticClass();
			SinkGraphNode = AddNode(0);
			Sink = CastChecked<UFlowDistinctInputTestNode>(SinkGraphNode->GetFlowNodeBase());
			First = NewObject<UFlowDistinctInputTestAddOn>(Sink);
			Second = NewObject<UFlowDistinctInputTestAddOn>(Sink);
			Sink->GetFlowNodeAddOnChildrenByEditor() = {First, Second};
			SinkGraphNode->ReconstructNode();
			UFlowGraphNode* FirstSupplier = AddNode(11);
			UFlowGraphNode* SecondSupplier = AddNode(22);
			FirstSupplier->FindPin(TEXT("Value"), EGPD_Output)->MakeLinkTo(
				SinkGraphNode->FindPin(First->Value.PropertyPinName, EGPD_Input));
			SecondSupplier->FindPin(TEXT("Value"), EGPD_Output)->MakeLinkTo(
				SinkGraphNode->FindPin(Second->Value.PropertyPinName, EGPD_Input));
			Asset->HarvestNodeConnections();
		}

		static bool ResolvesValue(UFlowNode& Node, const FName& PinName, int32 ExpectedValue)
		{
			int32 Value = 0;
			return Node.TryResolveDataPinValue<FFlowPinType_Int>(PinName, Value)
				== EFlowDataPinResolveResult::Success && Value == ExpectedValue;
		}
	};
}

BEGIN_DEFINE_SPEC(FFlowDistinctInputSpec, "Flow.Editor.DistinctInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FFlowDistinctInputSpec)

void FFlowDistinctInputSpec::Define()
{
	using namespace FlowDistinctInputTests;

	It("keeps each connection and resolved value attached to its provider after reorder", [this]
	{
		FFixture Fixture;
		const FName FirstName = Fixture.First->Value.PropertyPinName;
		const FName SecondName = Fixture.Second->Value.PropertyPinName;
		TestTrue(TEXT("First supplier is connected before reorder"),
			FFixture::ResolvesValue(*Fixture.Sink, FirstName, 11));
		TestTrue(TEXT("Second supplier is connected before reorder"),
			FFixture::ResolvesValue(*Fixture.Sink, SecondName, 22));
		Fixture.Sink->GetFlowNodeAddOnChildrenByEditor().Swap(0, 1);
		Fixture.SinkGraphNode->ReconstructNode();
		Fixture.Asset->HarvestNodeConnections();
		TestEqual(TEXT("First identity is stable"), Fixture.First->Value.PropertyPinName, FirstName);
		TestEqual(TEXT("Second identity is stable"), Fixture.Second->Value.PropertyPinName, SecondName);
		TestTrue(TEXT("First still reads its original supplier"),
			FFixture::ResolvesValue(*Fixture.Sink, FirstName, 11));
		TestTrue(TEXT("Second still reads its original supplier"),
			FFixture::ResolvesValue(*Fixture.Sink, SecondName, 22));
	});

	It("preserves existing connections when a same-property provider is inserted and removed", [this]
	{
		FFixture Fixture;
		UFlowDistinctInputTestAddOn* Inserted = NewObject<UFlowDistinctInputTestAddOn>(Fixture.Sink);
		Fixture.Sink->GetFlowNodeAddOnChildrenByEditor().Insert(Inserted, 0);
		Fixture.SinkGraphNode->ReconstructNode();
		Fixture.Asset->HarvestNodeConnections();
		TestTrue(TEXT("First supplier survives insertion"),
			FFixture::ResolvesValue(*Fixture.Sink, Fixture.First->Value.PropertyPinName, 11));
		TestTrue(TEXT("Second supplier survives insertion"),
			FFixture::ResolvesValue(*Fixture.Sink, Fixture.Second->Value.PropertyPinName, 22));
		Fixture.Sink->GetFlowNodeAddOnChildrenByEditor().Remove(Inserted);
		Fixture.SinkGraphNode->ReconstructNode();
		Fixture.Asset->HarvestNodeConnections();
		TestTrue(TEXT("First supplier survives removal"),
			FFixture::ResolvesValue(*Fixture.Sink, Fixture.First->Value.PropertyPinName, 11));
		TestTrue(TEXT("Second supplier survives removal"),
			FFixture::ResolvesValue(*Fixture.Sink, Fixture.Second->Value.PropertyPinName, 22));
	});

	It("remaps imported provider identities while retaining each copied connection", [this]
	{
		FFixture Fixture;
		TSet<UObject*> NodesToCopy;
		for (UEdGraphNode* GraphNode : Fixture.Graph->Nodes)
		{
			GraphNode->PrepareForCopying();
			NodesToCopy.Add(GraphNode);
		}
		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(NodesToCopy, ExportedText);
		for (UObject* Object : NodesToCopy)
		{
			CastChecked<UFlowGraphNode>(Object)->PostCopyNode();
		}

		TSet<UEdGraphNode*> ImportedNodes;
		FEdGraphUtilities::ImportNodesFromText(Fixture.Graph, ExportedText, ImportedNodes);
		UFlowDistinctInputTestNode* CopiedSink = nullptr;
		for (UEdGraphNode* ImportedNode : ImportedNodes)
		{
			UFlowGraphNode* GraphNode = CastChecked<UFlowGraphNode>(ImportedNode);
			GraphNode->CreateNewGuid();
			UFlowDistinctInputTestNode* Node = CastChecked<UFlowDistinctInputTestNode>(GraphNode->GetFlowNodeBase());
			if (Node->Role == 0)
			{
				CopiedSink = Node;
			}
			Fixture.Asset->RegisterNode(GraphNode->NodeGuid, Node);
			GraphNode->MarkNeedsFullReconstruction();
			GraphNode->ReconstructNode();
		}
		Fixture.Asset->HarvestNodeConnections();
		if (!TestNotNull(TEXT("Copied sink was imported"), CopiedSink)
			|| !TestEqual(TEXT("Both copied providers retained"), CopiedSink->GetFlowNodeAddOnChildren().Num(), 2))
		{
			return;
		}
		UFlowDistinctInputTestAddOn* CopiedFirst =
			CastChecked<UFlowDistinctInputTestAddOn>(CopiedSink->GetFlowNodeAddOnChildren()[0]);
		UFlowDistinctInputTestAddOn* CopiedSecond =
			CastChecked<UFlowDistinctInputTestAddOn>(CopiedSink->GetFlowNodeAddOnChildren()[1]);
		TestTrue(TEXT("Copied provider gets its own identity"), CopiedFirst->GetGuid() != Fixture.First->GetGuid());
		TestTrue(TEXT("First copied connection reads the first supplier"),
			FFixture::ResolvesValue(*CopiedSink, CopiedFirst->Value.PropertyPinName, 11));
		TestTrue(TEXT("Second copied connection reads the second supplier"),
			FFixture::ResolvesValue(*CopiedSink, CopiedSecond->Value.PropertyPinName, 22));
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
