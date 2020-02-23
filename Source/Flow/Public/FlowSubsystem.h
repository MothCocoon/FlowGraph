#pragma once

#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlowSubsystem.generated.h"

class UFlowAsset;
class UFlowComponent;
class UFlowNodeSubGraph;

/**
 * Flow Control System
 */
UCLASS()
class FLOW_API UFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UFlowSubsystem();

private:
	FStreamableManager Streamable;

	// all instanced assets
	TSet<TWeakObjectPtr<UFlowAsset>> InstancedAssets;

	// instanced assets "owned" by Sub Flow nodes
	TMap<UFlowNodeSubGraph*, UFlowAsset*> InstancedSubFlows;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void StartFlow(UFlowAsset* FlowAsset);
	void EndFlow(UFlowAsset* FlowAsset);

	void PreloadSubFlow(UFlowNodeSubGraph* SubFlow);
	void FlushPreload(UFlowNodeSubGraph* SubFlow);

	void StartSubFlow(UFlowNodeSubGraph* SubFlow);

private:
	UFlowAsset* CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset);

public:
	FORCEINLINE UWorld* GetWorld() const;

//////////////////////////////////////////////////////////////////////////
// Flow Components

private:
	// all the Flow Components currently existing in the world
	TMultiMap<FGameplayTag, TWeakObjectPtr<UFlowComponent>> FlowComponents;

public:
	virtual void RegisterComponent(UFlowComponent* Component);
	virtual void UnregisterComponent(UFlowComponent* Component);

	template<class T>
	TArray<TWeakObjectPtr<T>> GetComponents(const FGameplayTag& Tag)
	{
		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FlowComponents.MultiFind(Tag, FoundComponents);

		TArray<TWeakObjectPtr<T>> ResultComponents;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.Get()->GetClass()->IsChildOf(T::StaticClass()))
			{
				ResultComponents.Emplace(Component);
			}
		}

		return ResultComponents;
	}

	template<class T>
	TArray<TWeakObjectPtr<T>> GetActors(const FGameplayTag& Tag)
	{
		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FlowComponents.MultiFind(Tag, FoundComponents);

		TArray<TWeakObjectPtr<T>> ResultActors;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.Get()->GetOwner()->GetClass()->IsChildOf(T::StaticClass()))
			{
				ResultActors.Emplace(Component.Get()->GetOwner());
			}
		}

		return ResultActors;
	}
};