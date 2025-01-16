// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowActorOwnerComponentRef.h"

#include "Nodes/FlowNode.h"
#include "Types/FlowInjectComponentsHelper.h"
#include "Types/FlowEnumUtils.h"

#include "FlowNode_ExecuteComponent.generated.h"

// Forward Declarations
class IFlowOwnerInterface;
class UFlowInjectComponentsManager;

UENUM()
enum class EExecuteComponentSource : uint8
{
	Undetermined,

	BindToExisting,
	InjectFromTemplate,
	InjectFromClass,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0,

	UsesInjectManagerFirst = InjectFromTemplate UMETA(Hidden),
	UsesInjectManagerLast = InjectFromClass UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EExecuteComponentSource)

namespace EExecuteComponentSource_Classifiers
{
	FORCEINLINE bool DoesComponentSourceUseInjectManager(EExecuteComponentSource Source) { return FLOW_IS_ENUM_IN_SUBRANGE(Source, EExecuteComponentSource::UsesInjectManager); }
}

/**
 * Execute a UActorComponent on the owning actor as if it was a flow subgraph
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Execute Component"))
class FLOW_API UFlowNode_ExecuteComponent : public UFlowNode
{
	GENERATED_BODY()

public:

	UFlowNode_ExecuteComponent();

	// IFlowCoreExecutableInterface
	virtual void InitializeInstance() override;
	virtual void DeinitializeInstance() override;
	virtual void PreloadContent() override;
	virtual void FlushContent() override;
	virtual void OnActivate() override;
	virtual void Cleanup() override;
	virtual void ForceFinishNode() override;
	virtual void ExecuteInput(const FName& PinName) override;
	// --

	// UFlowNodeBase
	virtual void UpdateNodeConfigText_Implementation() override;
	// --

#if WITH_EDITOR
	// UObject
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

	// UFlowNode
	virtual FText GetNodeTitle() const override;
	virtual EDataValidationResult ValidateNode() override;

	virtual FString GetStatusString() const override;
	// --
#endif // WITH_EDITOR
	
protected:

#if WITH_EDITOR
	void RefreshPins();
	const UActorComponent* TryGetExpectedComponent() const;

	void RefreshComponentSource();
#endif // WITH_EDITOR

	bool TryInjectComponent();

	UActorComponent* TryResolveComponent();
	TSubclassOf<AActor> TryGetExpectedActorOwnerClass() const;

protected:

	// Executable Component (by name) on the expected Flow owning Actor
	//  (the component must implement the IFlowExecutableComponentInterface)
	UPROPERTY(EditAnywhere, Category = "Flow Executable Component", meta = (DisplayName = "Component to Execute", MustImplement = "/Script/Flow.FlowCoreExecutableInterface,/Script/Flow.FlowExternalExecutableInterface", EditConditionHides, EditCondition = "ComponentSource == EExecuteComponentSource::BindToExisting || ComponentSource == EExecuteComponentSource::Undetermined"))
	FFlowActorOwnerComponentRef ComponentRef;

	// Component (template) to inject on the spawned actor, may be configured inline
	UPROPERTY(EditAnywhere, Instanced, Category = Configuration, DisplayName = "Inject & Execute Component (from Template)", meta = (MustImplement = "/Script/Flow.FlowCoreExecutableInterface,/Script/Flow.FlowExternalExecutableInterface", EditConditionHides, EditCondition = "ComponentSource == EExecuteComponentSource::InjectFromTemplate || ComponentSource == EExecuteComponentSource::Undetermined"))
	TObjectPtr<UActorComponent> ComponentTemplate = nullptr;

	// Component (class) to inject on the spawned actor
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "Inject & Execute Component (by Class)", meta = (MustImplement = "/Script/Flow.FlowCoreExecutableInterface,/Script/Flow.FlowExternalExecutableInterface", EditConditionHides, EditCondition = "ComponentSource == EExecuteComponentSource::InjectFromClass || ComponentSource == EExecuteComponentSource::Undetermined"))
	TSubclassOf<UActorComponent> ComponentClass = nullptr;

	// Manager object to inject and remove components from the Flow owning Actor
	UPROPERTY(Transient)
	TObjectPtr<UFlowInjectComponentsManager> InjectComponentsManager = nullptr;

	// Look for the component (by class) on the Actor and re-use it (rather than injecting)
	// if the component already exists.
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "Re-use existing component if found", meta = (EditConditionHides, EditCondition = "ComponentSource == EExecuteComponentSource::InjectFromClass"))
	bool bReuseExistingComponent = true;

	// Allow injecting the component, if it cannot be found on the Actor
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "Allow injecting component", meta = (EditConditionHides, EditCondition = "ComponentSource == EExecuteComponentSource::InjectFromClass && bReuseExistingComponent"))
	bool bAllowInjectComponent = true;

	// Inject component(s) onto the owning Actor
	UPROPERTY()
	EExecuteComponentSource ComponentSource = EExecuteComponentSource::Undetermined;
};
