// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Details/Actor/FlowActorOwnerComponentRefCustomization.h"

#include "AddOns/FlowNodeAddOn.h"
#include "FlowAsset.h"
#include "FlowActorOwnerComponentFilters.h"
#include "Nodes/FlowNode.h"

#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"

void FFlowActorOwnerComponentRefCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Do not include children properties (the header is all we need to show for this struct)
}

TSharedPtr<IPropertyHandle> FFlowActorOwnerComponentRefCustomization::GetCuratedNamePropertyHandle() const
{
	check(StructPropertyHandle->IsValidHandle());

	TSharedPtr<IPropertyHandle> FoundHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowActorOwnerComponentRef, ComponentName));
	check(FoundHandle);

	return FoundHandle;
}

void FFlowActorOwnerComponentRefCustomization::SetCuratedName(const FName& NewComponentName)
{
	TSharedPtr<IPropertyHandle> ComponentNameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowActorOwnerComponentRef, ComponentName));

	check(ComponentNameHandle);

	ComponentNameHandle->SetPerObjectValue(0, NewComponentName.ToString());
}

bool FFlowActorOwnerComponentRefCustomization::TryGetCuratedName(FName& OutName) const
{
	const FFlowActorOwnerComponentRef* ComponentRef = GetFlowActorOwnerComponentRef();
	if (ComponentRef)
	{
		OutName = ComponentRef->ComponentName;

		return true;
	}
	else
	{
		return false;
	}
}

TArray<FName> FFlowActorOwnerComponentRefCustomization::GetCuratedNameOptions() const
{
	TArray<FName> Results;

	UClass* ExpectedOwnerClass = TryGetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass) || !ExpectedOwnerClass->IsChildOf<AActor>())
	{
		return Results;
	}

	Results = GetFlowActorOwnerComponents(ExpectedOwnerClass);

	return Results;
}

UClass* FFlowActorOwnerComponentRefCustomization::TryGetExpectedOwnerClass() const
{
	const UFlowNode* NodeOwner = TryGetFlowNodeOuter();
	if (!IsValid(NodeOwner))
	{
		return nullptr;
	}

	const UFlowAsset* FlowAsset = NodeOwner->GetFlowAsset();
	if (!IsValid(FlowAsset))
	{
		return nullptr;
	}

	UClass* ExpectedOwnerClass = FlowAsset->GetExpectedOwnerClass().LoadSynchronous();
	return ExpectedOwnerClass;
}

UFlowNode* FFlowActorOwnerComponentRefCustomization::TryGetFlowNodeOuter() const
{
	check(StructPropertyHandle->IsValidHandle());

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	for (UObject* OuterObject : OuterObjects)
	{
		UFlowNode* FlowNodeOuter = Cast<UFlowNode>(OuterObject);
		if (IsValid(FlowNodeOuter))
		{
			return FlowNodeOuter;
		}

		UFlowNodeAddOn* FlowNodeAddOnOuter = Cast<UFlowNodeAddOn>(OuterObject);
		if (IsValid(FlowNodeAddOnOuter))
		{
			return FlowNodeAddOnOuter->GetFlowNode();
		}
	}

	return nullptr;
}

TArray<FName> FFlowActorOwnerComponentRefCustomization::GetFlowActorOwnerComponents(TSubclassOf<AActor> ExpectedActorOwnerClass) const
{
	TArray<const UActorComponent*> AllComponents;

	AActor::GetActorClassDefaultComponents<UActorComponent>(ExpectedActorOwnerClass, AllComponents);

	// Array for components that pass the metadata filter
	TArray<FName> PassedComponentNames;
	PassedComponentNames.Reserve(AllComponents.Num());

	const FProperty* MetadataProperty = StructPropertyHandle->GetMetaDataProperty();
	if (ensure(MetadataProperty))
	{
		// Pull the metadata from the struct property, setting up the AllowedClass filters, etc.
		FFlowActorOwnerComponentFilters Filters;
		Filters.BuildFiltersFromMetadata(*MetadataProperty);

		for (const UActorComponent* ActorComponent : AllComponents)
		{
			if (Filters.IsFilteredComponent(*ActorComponent))
			{
				FString ComponentCleanedName = ActorComponent->GetFName().ToString();

				// Some components end with _GEN_VARIABLE, remove that suffix so we can match component names
				ComponentCleanedName.RemoveFromEnd(UActorComponent::ComponentTemplateNameSuffix);

				PassedComponentNames.Add(FName(ComponentCleanedName));
			}
		}
	}

	return PassedComponentNames;
}
