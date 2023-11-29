// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Pins/SFlowInputPinHandle.h"
#include "Nodes/FlowNode.h"
#include "Pins/SFlowPinHandle.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"

void SFlowInputPinHandle::RefreshNameList()
{
	PinNames.Empty();

	if (Blueprint && Blueprint->GeneratedClass)
	{
		if (UFlowNode* FlowNode = Blueprint->GeneratedClass->GetDefaultObject<UFlowNode>())
		{
			for (const FFlowPin& InputPin : FlowNode->InputPins)
			{
				PinNames.Add(MakeShareable(new FName(InputPin.PinName)));
			}
		}
	}
}

TSharedPtr<SGraphPin> FFlowInputPinHandleFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (InPin->PinType.PinCategory == GetDefault<UEdGraphSchema_K2>()->PC_Struct && InPin->PinType.PinSubCategoryObject == FFlowInputPinHandle::StaticStruct() && InPin->LinkedTo.Num() == 0)
	{
		if (const UEdGraphNode* GraphNode = InPin->GetOuter())
		{
			if (const UBlueprint* Blueprint = GraphNode->GetGraph()->GetTypedOuter<UBlueprint>())
			{
				return SNew(SFlowInputPinHandle, InPin, Blueprint);
			}
		}
	}

	return nullptr;
}
