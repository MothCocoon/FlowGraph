// Copyright Acinex Games 2020


#include "Nodes/Utils/FlowNode_PropertyGetter.h"

#include "FlowAsset.h"

UFlowNode_PropertyGetter::UFlowNode_PropertyGetter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  Property(nullptr)
{
#if WITH_EDITOR
	Category = TEXT("Variables");
	NodeStyle = EFlowNodeStyle::Default;
#endif

	InputPins = {};
	OutputPins = {};
}

void UFlowNode_PropertyGetter::SetProperty(FProperty* InProperty)
{
	Property = InProperty;
	PropertyName = Property->GetFName();
}

FText UFlowNode_PropertyGetter::GetNodeTitle() const
{
	return FText::FromString(Property ? Property->GetAuthoredName() : TEXT("Invalid Property"));
}

void UFlowNode_PropertyGetter::PostLoad()
{
	Super::PostLoad();
	LoadProperty();
}

const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> UFlowNode_PropertyGetter::GetOutputProperties()
{
	LoadProperty();
	
	TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> Properties;
	if (!Property)
	{
		return Properties;
	}

	FFlowInputOutputPin PropertyToAdd = FFlowInputOutputPin(nullptr, Property);
	PropertyToAdd.OutputPinName = Property->GetFName();
	PropertyToAdd.PinTooltip = Property->GetToolTipText(true).ToString();
	Properties.Add(GetFlowAsset(), PropertyToAdd);
	return Properties;
}

const TMultiMap<TWeakObjectPtr<UObject>, FFlowInputOutputPin> UFlowNode_PropertyGetter::GetInputProperties()
{
	return {};
}

UObject* UFlowNode_PropertyGetter::GetVariableHolder()
{
	return GetFlowAsset();
}

void UFlowNode_PropertyGetter::LoadProperty()
{
	if (!Property)
	{
		Property = GetFlowAsset()->GetClass()->FindPropertyByName(PropertyName);
	}
}
