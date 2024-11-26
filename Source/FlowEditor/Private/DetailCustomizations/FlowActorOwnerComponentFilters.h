// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/ObjectPtr.h"

#include "FlowActorOwnerComponentFilters.generated.h"

// Forward Declarations
class UActorComponent;
class IPropertyHandle;

// Metadata-derived filters to describe qualifying UActorComponents 
// for a given FFlowActorOwnerComponentRef
USTRUCT()
struct FFlowActorOwnerComponentFilters
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	void BuildFiltersFromMetadata(const FProperty& ComponentNameProperty);

	// Returns true if the Component passes the filters (built in BuildFiltersFromMetadata)
	bool IsFilteredComponent(const UActorComponent& Component) const;

protected:
	void BuildClassFilters(const FProperty& ComponentNameProperty);
	void BuildInterfaceFilters(const FProperty& ComponentNameProperty);

#endif // WITH_EDITOR

protected:

#if WITH_EDITORONLY_DATA
	// Classes that can be used with this property
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UClass>> AllowedComponentClassFilters;

	// Classes that can NOT be used with this property
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UClass>> DisallowedComponentClassFilters;

	// Must implement (all) interface(s)
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UClass>> RequiredInterfaceFilters;

	// Has BuildClassFiltersFromMetadata been called?
	UPROPERTY(Transient)
	bool bHasBuiltFilters = false;

	// Meta-data keys
	static const FName NAME_AllowedClasses;
	static const FName NAME_DisallowedClasses;
	static const FName NAME_MustImplement;
#endif // WITH_EDITORONLY_DATA
};
