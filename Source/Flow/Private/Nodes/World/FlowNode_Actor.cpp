// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_Actor.h"

UFlowNode_Actor::UFlowNode_Actor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Condition;
	Category = TEXT("World");
#endif

	PropertyResolveMode = EFlowNodePropertyResolveMode::OnInputTrigger;
}

void UFlowNode_Actor::InitializeInstance()
{
	if (PropertyResolveMode == EFlowNodePropertyResolveMode::OnInitialize)
	{
		ResolveSoftActorPtrs();
	}

	Super::InitializeInstance();
}

void UFlowNode_Actor::ExecuteInput(const FName& PinName)
{
	if (PropertyResolveMode == EFlowNodePropertyResolveMode::OnInputTrigger)
	{
		ResolveSoftActorPtrs();
	}
	Super::ExecuteInput(PinName);
}



void UFlowNode_Actor::ResolveSoftActorPtrs()
{
	if (bHasResolvedSoftObjectPointers)
	{
		return;
	}

	// Stop resolve on template node
	UFlowAsset* Asset = GetFlowAsset();
	if (Asset == nullptr || Asset->GetTemplateAsset() == nullptr)
	{
		return;
	}

	bHasResolvedSoftObjectPointers = true;


	ULevel* Level = nullptr;
	{
		UObject* AssetOwner = Asset->GetOwner();
		if (AActor* ActorOwner = Cast<AActor>(AssetOwner))
		{
			Level = ActorOwner->GetLevel();
		}
		else if (UActorComponent* CompOwner = Cast<UActorComponent>(AssetOwner))
		{
			Level = CompOwner->GetComponentLevel();
		}
	}

	if (!Level)
	{
		return;
	}

	int32 NumResolved = 0;
	for (TPropertyValueIterator<FSoftObjectProperty> It(this->GetClass(), this); It; ++It)
	{
		FSoftObjectProperty* ObjectProp = CastField<FSoftObjectProperty>(It.Key());

		if (ObjectProp &&
			//ObjectProp->HasAllPropertyFlags(CPF_Edit) && 
			!ObjectProp->HasAnyPropertyFlags(CPF_DisableEditOnInstance | CPF_Transient | CPF_Protected) &&
			ObjectProp->PropertyClass && ObjectProp->PropertyClass->IsChildOf<AActor>())
		{
			NumResolved++;

			// Is modifying safe during iteration? But writing giant boilerplate from TPropertyValueIterator to handle all containers is suboptimal
			FSoftObjectPtr* Value = const_cast<FSoftObjectPtr*>(ObjectProp->GetPropertyValuePtr(It.Value()));

			FSoftObjectPath Path = Value->ToSoftObjectPath();
			if (Path.IsValid() && !Path.GetSubPathString().IsEmpty())
			{
				UObject* ResolvedObject = FindObject<UObject>(Level->GetOuter(), *Path.GetSubPathString());
				Path = FSoftObjectPath(ResolvedObject);
			}
			else
			{
				Path.Reset(); // Clear path to prevent modifying template 
			}

			*Value = Path;
		}
	}
}