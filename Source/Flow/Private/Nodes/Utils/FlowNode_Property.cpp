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
	return const_cast<UScriptStruct*>(GetFlowAsset()->Properties.GetScriptStruct());
}

uint8* UFlowNode_Property::GetVariableContainer()
{
	return GetFlowAsset()->Properties.GetMutableMemory();
}

void UFlowNode_Property::PostLoad()
{
	Super::PostLoad();
	LoadProperty();
}

const TArray<FFlowPropertyPin> UFlowNode_Property::GetOutputProperties()
{
	return {};
}

const TArray<FFlowPropertyPin> UFlowNode_Property::GetInputProperties()
{
	return {};
}

void UFlowNode_Property::LoadProperty()
{
	if (!Property)
	{
		if (const UScriptStruct* ScriptStruct = GetFlowAsset()->Properties.GetScriptStruct())
		{
			Property = ScriptStruct->FindPropertyByName(PropertyName);
		}
	}
}
