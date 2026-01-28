// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_ExecuteComponent.h"
#include "Interfaces/FlowCoreExecutableInterface.h"
#include "Interfaces/FlowExternalExecutableInterface.h"
#include "Interfaces/FlowContextPinSupplierInterface.h"
#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "FlowSettings.h"
#include "Types/FlowInjectComponentsHelper.h"
#include "Types/FlowInjectComponentsManager.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

#define LOCTEXT_NAMESPACE "FlowNode"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_ExecuteComponent)

UFlowNode_ExecuteComponent::UFlowNode_ExecuteComponent()
	: Super()
{
#if WITH_EDITOR
	Category = TEXT("Actor");
#endif

	InputPins.Reset();
	OutputPins.Reset();
}

void UFlowNode_ExecuteComponent::InitializeInstance()
{
	Super::InitializeInstance();

	(void) TryInjectComponent();

	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->InitializeInstance();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_InitializeInstance(ResolvedComp);
		}
	}
}

void UFlowNode_ExecuteComponent::DeinitializeInstance()
{
	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->DeinitializeInstance();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_DeinitializeInstance(ResolvedComp);
		}
	}

	if (EExecuteComponentSource_Classifiers::DoesComponentSourceUseInjectManager(ComponentSource))
	{
		if (IsValid(InjectComponentsManager))
		{
			InjectComponentsManager->ShutdownRuntime();
		}
	}

	InjectComponentsManager = nullptr;

	Super::DeinitializeInstance();
}

void UFlowNode_ExecuteComponent::PreloadContent()
{
	Super::PreloadContent();

	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->PreloadContent();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_PreloadContent(ResolvedComp);
		}
	}
}

void UFlowNode_ExecuteComponent::FlushContent()
{
	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->FlushContent();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_FlushContent(ResolvedComp);
		}
	}

	Super::FlushContent();
}

void UFlowNode_ExecuteComponent::OnActivate()
{
	Super::OnActivate();

	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowExternalExecutableInterface* ComponentAsExternalExecutable = Cast<IFlowExternalExecutableInterface>(ResolvedComp))
		{
			// By convention, we must call the PreActivateExternalFlowExecutable() before OnActivate 
			// when we (this node) are acting as the proxy for an IFlowExternalExecutableInterface object
			ComponentAsExternalExecutable->PreActivateExternalFlowExecutable(*this);
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowExternalExecutableInterface::Execute_K2_PreActivateExternalFlowExecutable(ResolvedComp, this);
		}
		else
		{
			UE_LOG(LogFlow, Error, TEXT("Expected a valid UActorComponent that implemented the IFlowExternalExecutableInterface (%s)"), *ResolvedComp->GetClass()->GetName());
		}

		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->OnActivate();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_OnActivate(ResolvedComp);
		}
		else
		{
			UE_LOG(LogFlow, Error, TEXT("Expected a valid UActorComponent that implemented the IFlowCoreExecutableInterface (%s)"), *ResolvedComp->GetClass()->GetName());
		}
	}
}

void UFlowNode_ExecuteComponent::Cleanup()
{
	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->Cleanup();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_Cleanup(ResolvedComp);
		}
	}

	Super::Cleanup();
}

void UFlowNode_ExecuteComponent::ForceFinishNode()
{
	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->ForceFinishNode();
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_ForceFinishNode(ResolvedComp);
		}
	}

	Super::ForceFinishNode();
}

void UFlowNode_ExecuteComponent::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);

	if (UActorComponent* ResolvedComp = TryResolveComponent())
	{
		if (IFlowCoreExecutableInterface* ComponentAsCoreExecutable = Cast<IFlowCoreExecutableInterface>(ResolvedComp))
		{
			ComponentAsCoreExecutable->ExecuteInput(PinName);
		}
		else if (ResolvedComp->Implements<UFlowCoreExecutableInterface>())
		{
			IFlowCoreExecutableInterface::Execute_K2_ExecuteInput(ResolvedComp, PinName);
		}
	}
	else
	{
		LogError(FString::Printf(TEXT("Could not ExecuteInput %s, because the component was missing or could not be resolved."), *PinName.ToString()));
	}
}

#if WITH_EDITOR
TArray<FFlowPin> UFlowNode_ExecuteComponent::GetContextInputs() const
{
	TArray<FFlowPin> ContextInputs;
	const UActorComponent* ResolvedComp = GetResolvedComponent();
	
	if (!IsValid(ResolvedComp))
	{
		// If we don't have a Resolved Component object yet, try to find the expected component object. For
		// injected components this will return the CDO 
		ResolvedComp = TryGetExpectedComponent();
	}

	// NOTE: we have to call GetClass on the Resolved Component; not StaticClass(). This makes it so we can handle classes that only implement the
	// interface in Blueprints. 
	if (ResolvedComp && ResolvedComp->GetClass()->ImplementsInterface(UFlowContextPinSupplierInterface::StaticClass()))
	{
		if (const IFlowContextPinSupplierInterface* CompAsStaticInterface = Cast<IFlowContextPinSupplierInterface>(ResolvedComp))
		{
			// The native (static) class implements the interface, so we call it directly (default implementation provided by the interface will call the K2 BP version of it).
			// Ee assume that the implementor of the interface is responsible for invoking Execute_K2_GetContextInputs in their overrides of GetContextInputs()
			ContextInputs = CompAsStaticInterface->GetContextInputs();
		}
		else
		{
			// Only the BP class implements the interface, so we call it here. 
			ContextInputs = IFlowContextPinSupplierInterface::Execute_K2_GetContextInputs(ResolvedComp);			
		}
	}

	if (ContextInputs.IsEmpty())
	{
		// Add the default input if none are desired by the component
		ContextInputs.Add(UFlowNode::DefaultInputPin);
	}

	ContextInputs.Append(Super::GetContextInputs());

	return ContextInputs;
}

TArray<FFlowPin> UFlowNode_ExecuteComponent::GetContextOutputs() const
{
	TArray<FFlowPin> ContextOutputs;
	const UActorComponent* ResolvedComp = GetResolvedComponent();

	if (!IsValid(ResolvedComp))
	{
		// If we don't have a Resolved Component object yet, try to find the expected component object. For
		// injected components this will return the CDO 
		ResolvedComp = TryGetExpectedComponent();
	}

	// NOTE: we have to call GetClass on the Resolved Component; not StaticClass(). This makes it so we can handle classes that only implement the
	// interface in Blueprints. 
	if (ResolvedComp && ResolvedComp->GetClass()->ImplementsInterface(UFlowContextPinSupplierInterface::StaticClass()))
	{
		if (const IFlowContextPinSupplierInterface* CompAsStaticInterface = Cast<IFlowContextPinSupplierInterface>(ResolvedComp))
		{
			// The native (static) class implements the interface, so we call it directly (default implementation provided by the interface will call the K2 BP version of it.
			// we assume that the implementor of the interface is responsible for invoking K2_GetContextOutputs in their overrides of GetContextOutputs()
			ContextOutputs = CompAsStaticInterface->GetContextOutputs();
		}
		else
		{
			// Only the BP class implements the interface, so we call it here. 
			ContextOutputs = IFlowContextPinSupplierInterface::Execute_K2_GetContextOutputs(ResolvedComp);
		}
	}

	if (ContextOutputs.IsEmpty())
	{
		// Add the default output if none are desired by the component
		ContextOutputs.Add(UFlowNode::DefaultOutputPin);
	}

	ContextOutputs.Append(Super::GetContextOutputs());

	return ContextOutputs;
}

#endif // WITH_EDITOR

void UFlowNode_ExecuteComponent::GatherPotentialPropertyOwnersForDataPins(TArray<const UObject*>& InOutOwners) const
{
	Super::GatherPotentialPropertyOwnersForDataPins(InOutOwners);

	// Can also source properties from the resolved component (runtime) or expected component (in-editor)
	const UActorComponent* ResolvedComp = GetResolvedOrExpectedComponent();
	if (IsValid(ResolvedComp))
	{
		InOutOwners.AddUnique(ResolvedComp);
	}
}

bool UFlowNode_ExecuteComponent::TryInjectComponent()
{
	if (!EExecuteComponentSource_Classifiers::DoesComponentSourceUseInjectManager(ComponentSource))
	{
		return false;
	}

	AActor* ActorOwner = TryGetRootFlowActorOwner();
	if (!IsValid(ActorOwner))
	{
		return false;
	}

	// Create the component instance
	TArray<UActorComponent*> ComponentInstances;
	
	FLOW_ASSERT_ENUM_MAX(EExecuteComponentSource, 4);

	switch (ComponentSource)
	{
	case EExecuteComponentSource::InjectFromTemplate:
		{
			if (IsValid(ComponentTemplate))
			{
				if (UActorComponent* ComponentInstance = FFlowInjectComponentsHelper::TryCreateComponentInstanceForActorFromTemplate(*ActorOwner, *ComponentTemplate))
				{
					ComponentInstances.Add(ComponentInstance);
				}
			}
		}
		break;

	case EExecuteComponentSource::InjectFromClass:
		{
			if (IsValid(ComponentClass))
			{
				if (bReuseExistingComponent)
				{
					// Look for the component class existing already on the actor, for potential re-use

					UActorComponent* ExistingComponent = ActorOwner->FindComponentByClass(ComponentClass);
					if (IsValid(ExistingComponent))
					{
						// Set the ComponentRef directly (for later lookup via TryResolveComponent)
						ComponentRef.SetResolvedComponentDirect(*ExistingComponent);

						return true;
					}

					if (!bAllowInjectComponent)
					{
						return false;
					}
				}

				const FName InstanceBaseName = ComponentClass->GetFName();
				if (UActorComponent* ComponentInstance = FFlowInjectComponentsHelper::TryCreateComponentInstanceForActorFromClass(*ActorOwner, *ComponentClass, InstanceBaseName))
				{
					ComponentInstances.Add(ComponentInstance);
				}
			}
		}
		break;

	default:
		checkNoEntry();
		return false;
	}

	// Create the manager object if we're injecting a component
	InjectComponentsManager = NewObject<UFlowInjectComponentsManager>(this);
	InjectComponentsManager->InitializeRuntime();

	// Inject the desired component
	if (!ComponentInstances.IsEmpty())
	{
		check(ComponentInstances.Num() == 1);

		InjectComponentsManager->InjectComponentsOnActor(*ActorOwner, ComponentInstances);

		// Set the ComponentRef directly (for later lookup via TryResolveComponent)
		ComponentRef.SetResolvedComponentDirect(*ComponentInstances[0]);
	}

	return true;
}

const UActorComponent* UFlowNode_ExecuteComponent::GetResolvedOrExpectedComponent() const
{
	const UActorComponent* ResolvedComp = ComponentRef.GetResolvedComponent();
	if (IsValid(ResolvedComp))
	{
		return ResolvedComp;
	}

#if WITH_EDITOR
	const UActorComponent* ExpectedComp = TryGetExpectedComponent();
	if (IsValid(ExpectedComp))
	{
		return ExpectedComp;
	}
#endif

	return nullptr;
}

UActorComponent* UFlowNode_ExecuteComponent::TryResolveComponent()
{
	UActorComponent* ResolvedComp = ComponentRef.GetResolvedComponent();
	if (IsValid(ResolvedComp))
	{
		return ResolvedComp;
	}

	AActor* ActorOwner = TryGetRootFlowActorOwner();

	if (!IsValid(ActorOwner))
	{
		UE_LOG(LogFlow, Error, TEXT("Expected a valid Actor owner to resolve component reference %s"), *ComponentRef.ComponentName.ToString());

		return nullptr;
	}

	// Injected components are totally optional, if they are not there, we have to assume that's intentional.
	constexpr bool bAllowWarnIfFailed = true;
	ResolvedComp = ComponentRef.TryResolveComponent(*ActorOwner, bAllowWarnIfFailed);

	return ResolvedComp;
}

UActorComponent* UFlowNode_ExecuteComponent::GetResolvedComponent() const
{
	// This version of the function assumes the component has already been resolved previously.
	// (using TryResolveComponent)
	UActorComponent* ResolvedComp = ComponentRef.GetResolvedComponent();
	if (IsValid(ResolvedComp))
	{
		return ResolvedComp;
	}

	return nullptr;
}

#if WITH_EDITOR
const UActorComponent* UFlowNode_ExecuteComponent::TryGetExpectedComponent() const
{
	const TSubclassOf<AActor> ExpectedOwnerClass = TryGetExpectedActorOwnerClass();

	FLOW_ASSERT_ENUM_MAX(EExecuteComponentSource, 4);

	switch (ComponentSource)
	{
	case EExecuteComponentSource::Undetermined:
		{
			return nullptr;
		}
	case EExecuteComponentSource::BindToExisting:
		{
			return AActor::GetActorClassDefaultComponentByName(ExpectedOwnerClass, ComponentRef.ComponentName);
		}
	case EExecuteComponentSource::InjectFromTemplate:
		{
			return ComponentTemplate;
		}
	case EExecuteComponentSource::InjectFromClass:
		{
			return IsValid(ComponentClass) ? ComponentClass->GetDefaultObject<UActorComponent>() : nullptr;
		}

	default:
		return nullptr;
	}
}

void UFlowNode_ExecuteComponent::PostLoad()
{
	Super::PostLoad();

	RefreshComponentSource();
}

void UFlowNode_ExecuteComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowActorOwnerComponentRef, ComponentName) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode_ExecuteComponent, ComponentTemplate) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode_ExecuteComponent, ComponentClass))
	{
		RefreshComponentSource();

		RefreshPins();
	}
}

void UFlowNode_ExecuteComponent::RefreshComponentSource()
{
	if (ComponentRef.IsConfigured())
	{
		ComponentSource = EExecuteComponentSource::BindToExisting;
	}
	else if (ComponentTemplate != nullptr)
	{
		ComponentSource = EExecuteComponentSource::InjectFromTemplate;
	}
	else if (ComponentClass != nullptr)
	{
		ComponentSource = EExecuteComponentSource::InjectFromClass;
	}
	else
	{
		ComponentSource = EExecuteComponentSource::Undetermined;
	}
}

void UFlowNode_ExecuteComponent::RefreshPins()
{
	OnReconstructionRequested.ExecuteIfBound();
}

EDataValidationResult UFlowNode_ExecuteComponent::ValidateNode()
{
	const EDataValidationResult SuperResult = Super::ValidateNode();

	EDataValidationResult FinalResult = CombineDataValidationResults(SuperResult, EDataValidationResult::Valid);
			
	if (IsValid(ComponentTemplate) || IsValid(ComponentClass))
	{
		return FinalResult;
	}
	
	const bool bHasComponent = ComponentRef.IsConfigured();
	if (!bHasComponent)
	{
		ValidationLog.Error<UFlowNode>(TEXT("ExecuteComponent requires a valid Compoennt reference"), this);

		return EDataValidationResult::Invalid;
	}

	const TSubclassOf<AActor> ExpectedActorOwnerClass = TryGetExpectedActorOwnerClass();
	if (!IsValid(ExpectedActorOwnerClass))
	{
		ValidationLog.Error<UFlowNode>(TEXT("Invalid or null Expected Actor Owner Class for this Flow Asset"), this);

		return EDataValidationResult::Invalid;
	}

	{
		// Check if the component can be found on the expected owner
		const UActorComponent* ExpectedComponent = TryGetExpectedComponent();
		if (!IsValid(ExpectedComponent))
		{
			ValidationLog.Error<UFlowNode>(TEXT("Could not resolve component for flow actor owner"), this);

			return EDataValidationResult::Invalid;
		}

		// Check that the component implements the expected interfaces
		if (!Cast<IFlowExternalExecutableInterface>(ExpectedComponent))
		{
			ValidationLog.Error<UFlowNode>(TEXT("Expected component to implement IFlowExternalExecutableInterface"), this);

			return EDataValidationResult::Invalid;
		}

		if (!Cast<IFlowCoreExecutableInterface>(ExpectedComponent))
		{
			ValidationLog.Error<UFlowNode>(TEXT("Expected component to implement IFlowCoreExecutableInterface"), this);

			return EDataValidationResult::Invalid;
		}
	}
		
	return FinalResult;
}

FString UFlowNode_ExecuteComponent::GetStatusString() const
{
	if (ActivationState != EFlowNodeState::NeverActivated)
	{
		return UEnum::GetDisplayValueAsText(ActivationState).ToString();
	}

	return Super::GetStatusString();
}

TSubclassOf<AActor> UFlowNode_ExecuteComponent::TryGetExpectedActorOwnerClass() const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();
	if (IsValid(FlowAsset))
	{
		return FlowAsset->GetExpectedOwnerClass();
	}

	return nullptr;
}

FText UFlowNode_ExecuteComponent::K2_GetNodeTitle_Implementation() const
{
	if (UFlowSettings::Get()->bUseAdaptiveNodeTitles)
	{
		FLOW_ASSERT_ENUM_MAX(EExecuteComponentSource, 4);

		switch (ComponentSource)
		{
		case EExecuteComponentSource::Undetermined:
			break;

		case EExecuteComponentSource::BindToExisting:
			{
				if (!ComponentRef.ComponentName.IsNone())
				{
					const FText ComponentNameText = FText::FromName(ComponentRef.ComponentName);

					return FText::Format(LOCTEXT("ExecuteComponent", "Execute {0}"), {ComponentNameText});
				}
			}
			break;

		case EExecuteComponentSource::InjectFromTemplate:
			{
				if (IsValid(ComponentTemplate))
				{
					FString ComponentNameString = ComponentTemplate->GetName();
					ComponentNameString.RemoveFromEnd(TEXT("_C"));
					const FText ComponentNameText = FText::FromString(ComponentNameString);

					return FText::Format(LOCTEXT("ExecuteComponent", "Execute {0}"), {ComponentNameText});
				}
			}
			break;

		case EExecuteComponentSource::InjectFromClass:
			{
				if (IsValid(ComponentClass))
				{
					FString ComponentClassString = ComponentClass->GetName();
					ComponentClassString.RemoveFromEnd(TEXT("_C"));
					const FText ComponentNameText = FText::FromString(ComponentClassString);

					return FText::Format(LOCTEXT("ExecuteComponent", "Execute {0}"), {ComponentNameText});
				}
			}
			break;

		default: break;
		}
	}

	return Super::K2_GetNodeTitle_Implementation();
}

#endif // WITH_EDITOR

void UFlowNode_ExecuteComponent::UpdateNodeConfigText_Implementation()
{
#if WITH_EDITOR
	FText ComponentNameText;

	const bool bUseAdaptiveNodeTitles = UFlowSettings::Get()->bUseAdaptiveNodeTitles;
	if (!bUseAdaptiveNodeTitles)
	{
		FLOW_ASSERT_ENUM_MAX(EExecuteComponentSource, 4);

		switch (ComponentSource)
		{
		case EExecuteComponentSource::Undetermined:
			break;

		case EExecuteComponentSource::BindToExisting:
			{
				if (!ComponentRef.ComponentName.IsNone())
				{
					ComponentNameText = FText::FromName(ComponentRef.ComponentName);
				}
			}
			break;

		case EExecuteComponentSource::InjectFromTemplate:
			{
				if (IsValid(ComponentTemplate))
				{
					FString ComponentNameString = ComponentTemplate->GetName();
					ComponentNameString.RemoveFromEnd(TEXT("_C"));

					ComponentNameText = FText::FromString(ComponentNameString);
				}
			}
			break;

		case EExecuteComponentSource::InjectFromClass:
			{
				if (IsValid(ComponentClass))
				{
					FString ComponentClassString = ComponentClass->GetName();
					ComponentClassString.RemoveFromEnd(TEXT("_C"));

					ComponentNameText = FText::FromString(ComponentClassString);
				}
			}
			break;

		default: break;
		}
	}

	SetNodeConfigText(ComponentNameText);
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE
