// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EdGraph/EdGraph.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Tests/FlowNodePinNameValidationTestTypes.h"
#include "Types/FlowPinTypesStandard.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(
	FFlowNodePinNameValidationSpec,
	"Flow.Editor.PinNameValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FFlowNodePinNameValidationSpec)

void FFlowNodePinNameValidationSpec::Define()
{
	It("allows same-named exec pins in opposite directions and preserves their graph directions", [this]
	{
		TStrongObjectPtr<UFlowAsset> Asset{NewObject<UFlowAsset>()};
		UFlowGraph* Graph = NewObject<UFlowGraph>(Asset.Get());
		Graph->Schema = UFlowGraphSchema::StaticClass();

		UFlowNodePinNameValidationTestNode* FlowNode =
			NewObject<UFlowNodePinNameValidationTestNode>(Asset.Get());
		UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(Graph);
		GraphNode->CreateNewGuid();
		Graph->AddNode(GraphNode, false, false);
		Asset->RegisterNode(GraphNode->NodeGuid, FlowNode);
		GraphNode->SetNodeTemplate(FlowNode);
		GraphNode->MarkNeedsFullReconstruction();
		GraphNode->ReconstructNode();

		TestTrue(
			TEXT("Same-named exec pins in opposite directions pass node validation"),
			FlowNode->ValidatePinNamesForTest() != EDataValidationResult::Invalid);

		UEdGraphPin* InputPin = GraphNode->FindPin(TEXT("StartLeft"), EGPD_Input);
		UEdGraphPin* OutputPin = GraphNode->FindPin(TEXT("StartLeft"), EGPD_Output);
		if (!TestNotNull(TEXT("Input exec pin is present"), InputPin)
			|| !TestNotNull(TEXT("Output exec pin is present"), OutputPin))
		{
			return;
		}

		InputPin->Direction = EGPD_Output;
		GraphNode->ReconstructNode();

		TestNotNull(TEXT("Reconstruction restores the input pin direction"),
			GraphNode->FindPin(TEXT("StartLeft"), EGPD_Input));
		TestNotNull(TEXT("Output pin remains present after reconstruction"),
			GraphNode->FindPin(TEXT("StartLeft"), EGPD_Output));
	});

	It("continues rejecting duplicate names within one direction and data pin collisions", [this]
	{
		UFlowNodePinNameValidationTestNode* DuplicateInputNode =
			NewObject<UFlowNodePinNameValidationTestNode>();
		DuplicateInputNode->SetPinsForTest(
			{
				FFlowPin(TEXT("Shared")),
				FFlowPin(TEXT("Shared"), FFlowPinType_Bool::GetPinTypeNameStatic())
			},
			{FFlowPin(TEXT("Output"))});
		TestEqual(
			TEXT("Duplicate input names remain invalid"),
			DuplicateInputNode->ValidatePinNamesForTest(),
			EDataValidationResult::Invalid);

		UFlowNodePinNameValidationTestNode* DataCollisionNode =
			NewObject<UFlowNodePinNameValidationTestNode>();
		DataCollisionNode->SetPinsForTest(
			{FFlowPin(TEXT("Shared"), FFlowPinType_Bool::GetPinTypeNameStatic())},
			{FFlowPin(TEXT("Shared"))});
		TestEqual(
			TEXT("A same-named data and exec pin remains invalid"),
			DataCollisionNode->ValidatePinNamesForTest(),
			EDataValidationResult::Invalid);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
