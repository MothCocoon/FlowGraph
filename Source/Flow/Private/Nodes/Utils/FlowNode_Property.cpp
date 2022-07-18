// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_Property.h"
#include "FlowAsset.h"

UFlowNode_Property::UFlowNode_Property(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  Property(nullptr)
{
#if WITH_EDITOR
	Category = TEXT("Variables");
	NodeStyle = EFlowNodeStyle::Default;
#endif
}

void UFlowNode_Property::SetProperty(FProperty* InProperty)
{
	Property = InProperty;
	PropertyName = Property->GetFName();
}

UObject* UFlowNode_Property::GetVariableHolder()
{
	return GetFlowAsset();
}

void UFlowNode_Property::PostLoad()
{
	Super::PostLoad();
	LoadProperty();
}

const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> UFlowNode_Property::GetOutputProperties()
{
	return {};
}

const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> UFlowNode_Property::GetInputProperties()
{
	return {};
}

void UFlowNode_Property::LoadProperty()
{
	if (!Property)
	{
		Property = GetFlowAsset()->GetClass()->FindPropertyByName(PropertyName);
	}
}
