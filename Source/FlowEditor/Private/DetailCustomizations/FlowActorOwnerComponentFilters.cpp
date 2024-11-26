 // Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowActorOwnerComponentFilters.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR
#include "EditorClassUtils.h"
#endif // WITH_EDITOR

#if WITH_EDITOR

const FName FFlowActorOwnerComponentFilters::NAME_AllowedClasses = "AllowedClasses";
const FName FFlowActorOwnerComponentFilters::NAME_DisallowedClasses = "DisallowedClasses";
const FName FFlowActorOwnerComponentFilters::NAME_MustImplement = "MustImplement";

void FFlowActorOwnerComponentFilters::BuildFiltersFromMetadata(const FProperty& ComponentNameProperty)
{
	if (bHasBuiltFilters)
	{
		return;
	}

	BuildClassFilters(ComponentNameProperty);
	BuildInterfaceFilters(ComponentNameProperty);

	bHasBuiltFilters = true;
}

void FFlowActorOwnerComponentFilters::BuildClassFilters(const FProperty& ComponentNameProperty)
{
	// NOTE (gtaylor) Adapted from FComponentReferenceCustomization::BuildClassFilters()

	auto AddToClassFilters = [this](const UClass* Class, TArray<const UClass*>& ComponentList)
	{
		if (Class->IsChildOf(UActorComponent::StaticClass()))
		{
			ComponentList.Add(Class);
		}
	};

	auto ParseClassFilters = [this, AddToClassFilters](const FString& MetaDataString, TArray<const UClass*>& ComponentList)
	{
		if (!MetaDataString.IsEmpty())
		{
			TArray<FString> ClassFilterNames;
			MetaDataString.ParseIntoArrayWS(ClassFilterNames, TEXT(","), true);

			for (const FString& ClassName : ClassFilterNames)
			{
				UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);
				if (!Class)
				{
					Class = LoadObject<UClass>(nullptr, *ClassName);
				}

				if (Class)
				{
					// If the class is an interface, expand it to be all classes in memory that implement the class.
					if (Class->HasAnyClassFlags(CLASS_Interface))
					{
						for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
						{
							UClass* const ClassWithInterface = (*ClassIt);
							if (ClassWithInterface->ImplementsInterface(Class))
							{
								AddToClassFilters(ClassWithInterface, ComponentList);
							}
						}
					}
					else
					{
						AddToClassFilters(Class, ComponentList);
					}
				}
			}
		}
	};

	// Account for the allowed classes specified in the property metadata
	const FString& AllowedClassesFilterString = ComponentNameProperty.GetMetaData(NAME_AllowedClasses);
	ParseClassFilters(AllowedClassesFilterString, MutableView(AllowedComponentClassFilters));

	// Account for disallowed classes specified in the property metadata
	const FString& DisallowedClassesFilterString = ComponentNameProperty.GetMetaData(NAME_DisallowedClasses);
	ParseClassFilters(DisallowedClassesFilterString, MutableView(DisallowedComponentClassFilters));
}

void FFlowActorOwnerComponentFilters::BuildInterfaceFilters(const FProperty& ComponentNameProperty)
{
	auto ParseInterfaceFilters = [this](const FString& MetaDataString, TArray<const UClass*>& RequiredInterfaces)
	{
		if (!MetaDataString.IsEmpty())
		{
			TArray<FString> InterfaceFilterNames;
			MetaDataString.ParseIntoArrayWS(InterfaceFilterNames, TEXT(","), true);

			for (const FString& InterfaceName : InterfaceFilterNames)
			{
				if (const UClass* RequiredInterface = FEditorClassUtils::GetClassFromString(InterfaceName))
				{
					RequiredInterfaces.Add(RequiredInterface);
				}
			}
		}
	};

	// MustImplement interface(s)
	const FString& MustImplementInterfacesFilterString = ComponentNameProperty.GetMetaData(NAME_MustImplement);
	ParseInterfaceFilters(MustImplementInterfacesFilterString, MutableView(RequiredInterfaceFilters));
}

bool FFlowActorOwnerComponentFilters::IsFilteredComponent(const UActorComponent& Component) const
{
	check(bHasBuiltFilters);

	// For Now(tm) at least, hard coding excluding Transient components
	//  (could make this configurable, but that doesn't make any sense for FRGIPeerComponentReference)
	constexpr bool bAllowTransient = false;
	if constexpr (!bAllowTransient)
	{
		const EObjectFlags Flags = Component.GetFlags();
		const bool bIsTransient = (Flags & RF_Transient) != 0;
		if (bIsTransient)
		{
			// The component is allowed to be transient if the owning actor is also marked as transient.
			// This happens with level instance actors placed in a level.
			if (const AActor* CompOwnerActor = Component.GetOwner())
			{
				const EObjectFlags OuterFlags = CompOwnerActor->GetFlags();
				const bool bIsOuterTransient = (OuterFlags & RF_Transient) != 0;
				if(!bIsOuterTransient)
				{
					return false;
				}
			}
		}
	}

	// Check for required interface(s)
	for (const UClass* RequiredInterface : RequiredInterfaceFilters)
	{
		if (IsValid(RequiredInterface) && !Component.GetClass()->ImplementsInterface(RequiredInterface))
		{
			return false;
		}
	}

	// NOTE (gtaylor) Adapted from FComponentReferenceCustomization::IsFilteredObject

	bool bAllowedToSetBasedOnFilter = true;

	const UClass* ObjectClass = Component.GetClass();
	if (AllowedComponentClassFilters.Num() > 0)
	{
		bAllowedToSetBasedOnFilter = false;
		for (const UClass* AllowedClass : AllowedComponentClassFilters)
		{
			const bool bAllowedClassIsInterface = AllowedClass->HasAnyClassFlags(CLASS_Interface);
			if (ObjectClass->IsChildOf(AllowedClass) || (bAllowedClassIsInterface && ObjectClass->ImplementsInterface(AllowedClass)))
			{
				bAllowedToSetBasedOnFilter = true;
				break;
			}
		}
	}

	if (DisallowedComponentClassFilters.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		for (const UClass* DisallowedClass : DisallowedComponentClassFilters)
		{
			const bool bDisallowedClassIsInterface = DisallowedClass->HasAnyClassFlags(CLASS_Interface);
			if (ObjectClass->IsChildOf(DisallowedClass) || (bDisallowedClassIsInterface && ObjectClass->ImplementsInterface(DisallowedClass)))
			{
				bAllowedToSetBasedOnFilter = false;
				break;
			}
		}
	}

	return bAllowedToSetBasedOnFilter;
}

#endif // WITH_EDITOR
