// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphPinFactory.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Widgets/SFlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"

#include "NodeFactory.h"
#include "SGraphPin.h"

//////////////////////////////////////////////////////////////////////////
// FFlowGraphPinFactory

TSharedPtr<SGraphPin> FFlowGraphPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (!InPin->GetSchema()->IsA<UFlowGraphSchema>())
	{
		// Limit pin widget creation to FlowGraph schemas 
		return nullptr;
	}

	const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(InPin->GetOwningNode());

	// Create the widget for a Flow 'Exec'-style pin
	if (FlowGraphNode && FFlowPin::IsExecPinCategory(InPin->PinType.PinCategory))
	{
		const TSharedPtr<SGraphPin> NewPinWidget = SNew(SFlowGraphPinExec, InPin);

		const UFlowNode* FlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase());

		if (!UFlowGraphSettings::Get()->bShowDefaultPinNames && IsValid(FlowNode))
		{
			if (InPin->Direction == EGPD_Input)
			{
				// Pin array can have pins with name None, which will not be created. We need to check if array have only one valid pin
				if (GatherValidPinsCount(FlowNode->GetInputPins()) == 1 && InPin->PinName == UFlowNode::DefaultInputPin.PinName)
				{
					NewPinWidget->SetShowLabel(false);
				}
			}
			else
			{
				// Pin array can have pins with name None, which will not be created. We need to check if array have only one valid pin
				if (GatherValidPinsCount(FlowNode->GetOutputPins()) == 1 && InPin->PinName == UFlowNode::DefaultOutputPin.PinName)
				{
					NewPinWidget->SetShowLabel(false);
				}
			}
		}

		return NewPinWidget;
	}

	// For data pins, give the K2 (blueprint) node factory an opportunity to create the widget
	TSharedPtr<SGraphPin> K2PinWidget = FNodeFactory::CreateK2PinWidget(InPin);
	if (K2PinWidget.IsValid())
	{
		return K2PinWidget;
	}

	return nullptr;
}

int32 FFlowGraphPinFactory::GatherValidPinsCount(const TArray<FFlowPin>& Pins)
{
	int32 Count = 0;
	for (const FFlowPin& Pin : Pins)
	{
		if (Pin.IsValid())
		{
			++Count;
		}
	}

	return Count;
}
