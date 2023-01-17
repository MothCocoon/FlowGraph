// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "FlowComponent.h"
#include "FlowSubsystem.generated.h"

class UFlowAsset;
class UFlowNode_SubGraph;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimpleFlowEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimpleFlowComponentEvent, UFlowComponent*, Component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTaggedFlowComponentEvent, UFlowComponent*, Component, const FGameplayTagContainer&, Tags);

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

public:
	UFlowSubsystem();

private:
	friend class UFlowAsset;
	friend class UFlowComponent;
	friend class UFlowNode_SubGraph;

	/* All asset templates with active instances */
	UPROPERTY()
	TArray<UFlowAsset*> InstancedTemplates;

	/* Assets instanced by object from another system, i.e. World Settings or Player Controller */
	UPROPERTY()
	TMap<UFlowAsset*, TWeakObjectPtr<UObject>> RootInstances;

	/* Assets instanced by Sub Graph nodes */
	UPROPERTY()
	TMap<UFlowNode_SubGraph*, UFlowAsset*> InstancedSubFlows;

	FStreamableManager Streamable;

protected:
	UPROPERTY()
	UFlowSaveGame* LoadedSaveGame;

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void AbortActiveFlows();

	/* Start the root Flow, graph that will eventually instantiate next Flow Graphs through the SubGraph node */
	UFUNCTION(BlueprintCallable, Category = "FlowSubsystem", meta = (DefaultToSelf = "Owner"))
	virtual void StartRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const bool bAllowMultipleInstances = true);

	virtual UFlowAsset* CreateRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const bool bAllowMultipleInstances = true);

	/* Finish Policy value is read by Flow Node
	 * Nodes have opportunity to terminate themselves differently if Flow Graph has been aborted
	 * Example: Spawn node might despawn all actors if Flow Graph is aborted, not completed */
	UFUNCTION(BlueprintCallable, Category = "FlowSubsystem", meta = (DefaultToSelf = "Owner"))
	virtual void FinishRootFlow(UObject* Owner, UFlowAsset* TemplateAsset, const EFlowFinishPolicy FinishPolicy);

	/* Finish Policy value is read by Flow Node
	 * Nodes have opportunity to terminate themselves differently if Flow Graph has been aborted
	 * Example: Spawn node might despawn all actors if Flow Graph is aborted, not completed */
	UFUNCTION(BlueprintCallable, Category = "FlowSubsystem", meta = (DefaultToSelf = "Owner"))
	virtual void FinishAllRootFlows(UObject* Owner, const EFlowFinishPolicy FinishPolicy);

protected:
	UFlowAsset* CreateSubFlow(UFlowNode_SubGraph* SubGraphNode, const FString SavedInstanceName = FString(), const bool bPreloading = false);
	void RemoveSubFlow(UFlowNode_SubGraph* SubGraphNode, const EFlowFinishPolicy FinishPolicy);

	UFlowAsset* CreateFlowInstance(const TWeakObjectPtr<UObject> Owner, TSoftObjectPtr<UFlowAsset> FlowAsset, FString NewInstanceName = FString());

	virtual void AddInstancedTemplate(UFlowAsset* Template);
	virtual void RemoveInstancedTemplate(UFlowAsset* Template);

public:
	/* Returns all assets instanced by object from another system like World Settings */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TMap<UObject*, UFlowAsset*> GetRootInstances() const;
	
	/* Returns asset instanced by specific object */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TSet<UFlowAsset*> GetRootInstancesByOwner(const UObject* Owner) const;

	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeprecatedFunction, DeprecationMessage="Use GetRootInstancesByOwner() instead."))
	UFlowAsset* GetRootFlow(const UObject* Owner) const;

	/* Returns assets instanced by Sub Graph nodes */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TMap<UFlowNode_SubGraph*, UFlowAsset*> GetInstancedSubFlows() const { return InstancedSubFlows; }

	virtual UWorld* GetWorld() const override;

//////////////////////////////////////////////////////////////////////////
// SaveGame

	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FSimpleFlowEvent OnSaveGame;

	UFUNCTION(BlueprintCallable, Category = "FlowSubsystem")
	virtual void OnGameSaved(UFlowSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "FlowSubsystem")
	virtual void OnGameLoaded(UFlowSaveGame* SaveGame);

	virtual void LoadRootFlow(UObject* Owner, UFlowAsset* FlowAsset, const FString& SavedAssetInstanceName);
	virtual void LoadSubFlow(UFlowNode_SubGraph* SubGraphNode, const FString& SavedAssetInstanceName);

	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	UFlowSaveGame* GetLoadedSaveGame() const { return LoadedSaveGame; }

//////////////////////////////////////////////////////////////////////////
// Component Registry

protected:
	/* All the Flow Components currently existing in the world */
	TMultiMap<FGameplayTag, TWeakObjectPtr<UFlowComponent>> FlowComponentRegistry;

protected:
	virtual void RegisterComponent(UFlowComponent* Component);
	virtual void OnIdentityTagAdded(UFlowComponent* Component, const FGameplayTag& AddedTag);
	virtual void OnIdentityTagsAdded(UFlowComponent* Component, const FGameplayTagContainer& AddedTags);

	virtual void UnregisterComponent(UFlowComponent* Component);
	virtual void OnIdentityTagRemoved(UFlowComponent* Component, const FGameplayTag& RemovedTag);
	virtual void OnIdentityTagsRemoved(UFlowComponent* Component, const FGameplayTagContainer& RemovedTags);

public:
	/* Called when actor with Flow Component appears in the world */
	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FSimpleFlowComponentEvent OnComponentRegistered;

	/* Called after adding Identity Tags to already registered Flow Component
	 * This can happen only after Begin Play occured in the component */
	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FTaggedFlowComponentEvent OnComponentTagAdded;

	/* Called when actor with Flow Component disappears from the world */
	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FSimpleFlowComponentEvent OnComponentUnregistered;

	/* Called after removing Identity Tags from the Flow Component, if component still has some Identity Tags
	 * This can happen only after Begin Play occured in the component */
	UPROPERTY(BlueprintAssignable, Category = "FlowSubsystem")
	FTaggedFlowComponentEvent OnComponentTagRemoved;

	/**
	 * Returns all registered Flow Components identified by given tag
	 * 
	 * @param Tag Tag to check if it matches Identity Tags of registered Flow Components
	 * @param ComponentClass Only components matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeterminesOutputType = "ComponentClass"))
	TSet<UFlowComponent*> GetFlowComponentsByTag(const FGameplayTag Tag, const TSubclassOf<UFlowComponent> ComponentClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered Flow Components identified by Any or All provided tags
	 * 
	 * @param Tags Container to check if it matches Identity Tags of registered Flow Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param ComponentClass Only components matching this class we'll be returned
	* @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeterminesOutputType = "ComponentClass"))
	TSet<UFlowComponent*> GetFlowComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<UFlowComponent> ComponentClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors with Flow Component identified by given tag
	 * 
	 * @param Tag Tag to check if it matches Identity Tags of registered Flow Components
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TSet<AActor*> GetFlowActorsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors with Flow Component identified by Any or All provided tags
	 * 
	 * @param Tags Container to check if it matches Identity Tags of registered Flow Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TSet<AActor*> GetFlowActorsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors as pairs: Actor as key, its Flow Component as value
	 * 
	 * @param Tag Tag to check if it matches Identity Tags of registered Flow Components
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TMap<AActor*, UFlowComponent*> GetFlowActorsAndComponentsByTag(const FGameplayTag Tag, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered actors as pairs: Actor as key, its Flow Component as value
	 * 
	 * @param Tags Container to check if it matches Identity Tags of registered Flow Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param ActorClass Only actors matching this class we'll be returned
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	UFUNCTION(BlueprintPure, Category = "FlowSubsystem", meta = (DeterminesOutputType = "ActorClass"))
	TMap<AActor*, UFlowComponent*> GetFlowActorsAndComponentsByTags(const FGameplayTagContainer Tags, const EGameplayContainerMatchType MatchType, const TSubclassOf<AActor> ActorClass, const bool bExactMatch = true) const;

	/**
	 * Returns all registered Flow Components identified by given tag
	 * 
	 * @tparam T Only components matching this class we'll be returned
	 * @param Tag Tag to check if it matches Identity Tags of registered Flow Components
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetComponents(const FGameplayTag& Tag, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");

		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tag, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ComponentOfClass = Cast<T>(Component))
				{
					Result.Emplace(ComponentOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered Flow Components identified by Any or All provided tags
	 * 
	 * @tparam T Only components matching this class we'll be returned
	 * @param Tags Container to check if it matches Identity Tags of registered Flow Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to GetComponents must be derived from UActorComponent");

		TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ComponentOfClass = Cast<T>(Component))
				{
					Result.Emplace(ComponentOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered Flow Components identified by given tag
	 * 
	 * @tparam T Only components matching this class we'll be returned
	 * @param Tag Tag to check if it matches Identity Tags of registered Flow Components
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetActors(const FGameplayTag& Tag, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetActors must be derived from AActor");

		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tag, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ActorOfClass = Cast<T>(Component->GetOwner()))
				{
					Result.Emplace(ActorOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered Flow Components identified by Any or All provided tags
	 * 
	 * @tparam T Only actors matching this class we'll be returned
	 * @param Tags Container to check if it matches Identity Tags of registered Flow Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class T>
	TSet<TWeakObjectPtr<T>> GetActors(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const AActor>::Value, "'T' template parameter to GetActors must be derived from AActor");

		TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

		TSet<TWeakObjectPtr<T>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				if (T* ActorOfClass = Cast<T>(Component->GetOwner()))
				{
					Result.Emplace(ActorOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered actors with Flow Component identified by given tag
	 * 
	 * @tparam ActorT Only actors matching this class we'll be returned
	 * @tparam ComponentT Only components matching this class we'll be returned
	 * @param Tag Tag to check if it matches Identity Tags of registered Flow Components
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class ActorT, class ComponentT>
	TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> GetActorsAndComponents(const FGameplayTag& Tag, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<ActorT, const AActor>::Value, "'ActorT' template parameter to GetActorsAndComponents must be derived from AActor");
		static_assert(TPointerIsConvertibleFromTo<ComponentT, const UActorComponent>::Value, "'ComponentT' template parameter to GetActorsAndComponents must be derived from UActorComponent");

		TArray<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tag, bExactMatch, FoundComponents);

		TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				ComponentT* ComponentOfClass = Cast<ComponentT>(Component);
				ActorT* ActorOfClass = Cast<ActorT>(Component->GetOwner());
				if (ComponentOfClass && ActorOfClass)
				{
					Result.Emplace(ActorOfClass, ComponentOfClass);
				}
			}
		}

		return Result;
	}

	/**
	 * Returns all registered actors with Flow Component identified by Any or All provided tags
	 * 
	 * @tparam ActorT Only actors matching this class we'll be returned
	 * @tparam ComponentT Only components matching this class we'll be returned
	 * @param Tags Container to check if it matches Identity Tags of registered Flow Components
	 * @param MatchType If Any, returned component needs to have only one of given tags. If All, component needs to have all given Identity Tags
	 * @param bExactMatch If true, the tag has to be exactly present, if false then TagContainer will include it's parent tags while matching. Be careful, using latter option may be very expensive, as the search cost is proportional to the number of registered Gameplay Tags!
	 */
	template <class ActorT, class ComponentT>
	TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> GetActorsAndComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch = true) const
	{
		static_assert(TPointerIsConvertibleFromTo<ActorT, const AActor>::Value, "'ActorT' template parameter to GetActorsAndComponents must be derived from AActor");
		static_assert(TPointerIsConvertibleFromTo<ComponentT, const UActorComponent>::Value, "'ComponentT' template parameter to GetActorsAndComponents must be derived from UActorComponent");

		TSet<TWeakObjectPtr<UFlowComponent>> FoundComponents;
		FindComponents(Tags, MatchType, bExactMatch, FoundComponents);

		TMap<TWeakObjectPtr<ActorT>, TWeakObjectPtr<ComponentT>> Result;
		for (const TWeakObjectPtr<UFlowComponent>& Component : FoundComponents)
		{
			if (Component.IsValid())
			{
				ComponentT* ComponentOfClass = Cast<ComponentT>(Component);
				ActorT* ActorOfClass = Cast<ActorT>(Component->GetOwner());
				if (ComponentOfClass && ActorOfClass)
				{
					Result.Emplace(ActorOfClass, ComponentOfClass);
				}
			}
		}

		return Result;
	}

private:
	void FindComponents(const FGameplayTag& Tag, const bool bExactMatch, TArray<TWeakObjectPtr<UFlowComponent>>& OutComponents) const;
	void FindComponents(const FGameplayTagContainer& Tags, const EGameplayContainerMatchType MatchType, const bool bExactMatch, TSet<TWeakObjectPtr<UFlowComponent>>& OutComponents) const;
};
