#pragma once

#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "FlowComponent.h"
#include "FlowSubsystem.generated.h"

class UFlowAsset;
class UFlowNode_SubGraph;

DECLARE_MULTICAST_DELEGATE_OneParam(FSimpleFlowComponentnEvent, UFlowComponent* /*Component*/);

/**
 * Flow Control System
 */
UCLASS()
class FLOW_API UFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UFlowSubsystem();

	FStreamableManager Streamable;

	// all instanced assets
	UPROPERTY()
	TSet<UFlowAsset*> InstancedAssets;

	// instanced assets "owned" by Sub Flow nodes
	UPROPERTY()
	TMap<UFlowNode_SubGraph*, UFlowAsset*> InstancedSubFlows;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void StartFlow(UFlowAsset* FlowAsset);
	void EndFlow(UFlowAsset* FlowAsset);

	void PreloadSubFlow(UFlowNode_SubGraph* SubFlow);
	void FlushPreload(UFlowNode_SubGraph* SubFlow);

	void StartSubFlow(UFlowNode_SubGraph* SubFlow);

private:
	UFlowAsset* CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset);

public:
	virtual UWorld* GetWorld() const override;

//////////////////////////////////////////////////////////////////////////
// Component Registry

private:
	// all the Flow Components currently existing in the world
	TMultiMap<FGameplayTag, TWeakObjectPtr<UFlowComponent>> FlowComponents;

public:
	virtual void RegisterComponent(UFlowComponent* Component);
	virtual void UnregisterComponent(UFlowComponent* Component);

	FSimpleFlowComponentnEvent OnComponentRegistered;
	FSimpleFlowComponentnEvent OnComponentUnregistered;

	template<class T>
	TArray<TWeakObjectPtr<T>> GetComponents(const FGameplayTag& Tag) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");
		
		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FlowComponents.MultiFind(Tag, FoundComponents);

		TArray<TWeakObjectPtr<T>> ResultComponents;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component->GetClass()->IsChildOf(T::StaticClass()))
			{
				ResultComponents.Emplace(Cast<T>(Component));
			}
		}

		return ResultComponents;
	}

	template<class T>
	TMap<TWeakObjectPtr<T>, TWeakObjectPtr<UFlowComponent>> GetActors(const FGameplayTag& Tag) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetComponents must be derived from AActor");
		
		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FlowComponents.MultiFind(Tag, FoundComponents);

		TMap<TWeakObjectPtr<T>, TWeakObjectPtr<UFlowComponent>> ResultActors;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component->GetOwner()->GetClass()->IsChildOf(T::StaticClass()))
			{
				ResultActors.Emplace(Cast<T>(Component->GetOwner()), Component);
			}
		}

		return ResultActors;
	}
};
