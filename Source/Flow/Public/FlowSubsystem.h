#pragma once

#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "FlowComponent.h"
#include "FlowSubsystem.generated.h"

class UFlowAsset;
class UFlowNode_SubGraph;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimpleFlowComponentEvent, UFlowComponent*, Component);

/**
 * Flow Subsystem
 * - manages lifetime of Flow Graphs
 * - connects Flow Graphs with actors containing the Flow Component
 * - convenient base for project-specific systems
 */
UCLASS()
class FLOW_API UFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UFlowSubsystem();

	FStreamableManager Streamable;

	// All asset templates with active instances
	UPROPERTY()
	TArray<UFlowAsset*> InstancedTemplates;

	// Assets instanced by object from another system like World Settings, not SubGraph node
	UPROPERTY()
	TMap<UObject*, UFlowAsset*> RootInstances;

	// Assets instanced by Sub Graph nodes
	UPROPERTY()
	TMap<UFlowNode_SubGraph*, UFlowAsset*> InstancedSubFlows;

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Start the root Flow, graph that will eventually instantiate next Flow Graphs through the SubGraph node
	void StartRootFlow(UObject* Owner, UFlowAsset* FlowAsset);

	// Finish the root Flow, typically when closing world that created this flow
	void FinishRootFlow(UObject* Owner, UFlowAsset* FlowAsset);

	void PreloadSubFlow(UFlowNode_SubGraph* SubFlow);
	void StartSubFlow(UFlowNode_SubGraph* SubFlow);
	void FinishSubFlow(UFlowNode_SubGraph* SubFlow);

	void RemoveInstancedTemplate(UFlowAsset* Template);

private:
	UFlowAsset* CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset);

public:
	// Returns assets instanced by object from another system like World Settings
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TMap<UObject*, UFlowAsset*> GetRootInstances() const { return RootInstances; }

	// Returns assets instanced by Sub Graph nodes
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
    TMap<UFlowNode_SubGraph*, UFlowAsset*> GetInstancedSubFlows() const { return InstancedSubFlows; }

	virtual UWorld* GetWorld() const override;

//////////////////////////////////////////////////////////////////////////
// Component Registry

private:
	// All the Flow Components currently existing in the world
	TMultiMap<FGameplayTag, TWeakObjectPtr<UFlowComponent>> FlowComponents;

public:
	virtual void RegisterComponent(UFlowComponent* Component);
	virtual void UnregisterComponent(UFlowComponent* Component);

	// Called when actor with Flow Component appears in the world
	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FSimpleFlowComponentEvent OnComponentRegistered;

	// Called when actor with Flow Component disappears from the world
	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FSimpleFlowComponentEvent OnComponentUnregistered;

	// Returns all registered Flow Components identified by given tag
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TSet<UFlowComponent*> GetFlowComponentsByTag(const FGameplayTag Tag) const;

	// Returns all registered Flow Components identified by at least one of given tags
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TSet<UFlowComponent*> GetFlowComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType) const;

	// Returns all registered actors with Flow Component identified by given tag
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TSet<AActor*> GetFlowActorsByTag(const FGameplayTag Tag) const;

	// Returns all registered actors with Flow Component identified by at least one of given tags
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TSet<AActor*> GetFlowActorsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType) const;

	// Returns all registered actors as pairs: Actor as key, its Flow Component as value
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TMap<AActor*, UFlowComponent*> GetFlowActorsAndComponentsByTag(const FGameplayTag Tag) const;

	// Returns all registered actors as pairs: Actor as key, its Flow Component as value
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TMap<AActor*, UFlowComponent*> GetFlowActorsAndComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType) const;

	// Returns all registered Flow Components identified by given tag
	template <class T>
	TSet<TWeakObjectPtr<T>> GetComponents(const FGameplayTag& Tag) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");

		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FlowComponents.MultiFind(Tag, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid() && Component->GetClass()->IsChildOf(T::StaticClass()))
			{
				Result.Emplace(Cast<T>(Component));
			}
		}

		return Result;
	}

	// Returns all registered Flow Components identified by at least one of given tags
	template <class T>
	TSet<TWeakObjectPtr<T>> GetComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");

		TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tags, FoundComponents, MatchType);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid() && Component->GetClass()->IsChildOf(T::StaticClass()))
			{
				Result.Emplace(Cast<T>(Component));
			}
		}

		return Result;
	}

	// Returns all registered actors with Flow Component identified by given tag
	template <class T>
	TMap<TWeakObjectPtr<T>, TWeakObjectPtr<UFlowComponent>> GetActors(const FGameplayTag& Tag) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetComponents must be derived from AActor");

		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FlowComponents.MultiFind(Tag, FoundComponents);

		TMap<TWeakObjectPtr<T>, TWeakObjectPtr<UFlowComponent>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(T::StaticClass()))
			{
				Result.Emplace(Cast<T>(Component->GetOwner()), Component);
			}
		}

		return Result;
	}

	// Returns all registered actors with Flow Component identified by at least one of given tags
	template <class T>
	TMap<TWeakObjectPtr<T>, TWeakObjectPtr<UFlowComponent>> GetActors(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetComponents must be derived from AActor");

		TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tags, FoundComponents, MatchType);

		TMap<TWeakObjectPtr<T>, TWeakObjectPtr<UFlowComponent>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid() && Component->GetOwner()->GetClass()->IsChildOf(T::StaticClass()))
			{
				Result.Emplace(Cast<T>(Component->GetOwner()), Component);
			}
		}

		return Result;
	}

private:
	void FindComponents(const FGameplayTagContainer& Tags, TSet<TWeakObjectPtr<UFlowComponent>>& OutComponents, const EGameplayContainerMatchType MatchType) const;
};
