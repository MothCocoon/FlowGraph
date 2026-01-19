// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowExecutableActorComponent.h"
#include "Nodes/FlowNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowExecutableActorComponent)

#if WITH_EDITOR
bool UFlowExecutableActorComponent::CanModifyFlowDataPinType() const
{
	return IsDefaultObject();
}

bool UFlowExecutableActorComponent::ShowFlowDataPinValueInputPinCheckbox() const
{
	return IsDefaultObject();
}

bool UFlowExecutableActorComponent::ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const
{
	return IsDefaultObject();
}

bool UFlowExecutableActorComponent::CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const
{
	return IsDefaultObject();
}

void UFlowExecutableActorComponent::SetFlowDataPinValuesRebuildDelegate(FSimpleDelegate InDelegate)
{
	FlowDataPinValuesRebuildDelegate = InDelegate;
}

void UFlowExecutableActorComponent::RequestFlowDataPinValuesDetailsRebuild()
{
	if (FlowDataPinValuesRebuildDelegate.IsBound())
	{
		FlowDataPinValuesRebuildDelegate.Execute();
	}
}
#endif

void UFlowExecutableActorComponent::PreActivateExternalFlowExecutable(UFlowNodeBase& FlowNodeBase)
{
	IFlowExternalExecutableInterface::PreActivateExternalFlowExecutable(FlowNodeBase);

	FlowNodeProxy = &FlowNodeBase;
}
