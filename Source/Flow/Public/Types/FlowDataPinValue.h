// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowPinEnums.h"
#include "FlowDataPinTypeNamesStandard.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"

#include "FlowDataPinValue.generated.h"

struct FFlowDataPinResult;
struct FFlowDataPinType;
class FProperty;
class UObject;
class IPropertyHandle;

USTRUCT()
struct FFlowDataPinValue
{
	GENERATED_BODY()

	friend class FFlowDataPinValueCustomization;

	typedef void FValueType;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins)
	bool bIsInputPin = false;

	UPROPERTY(EditAnywhere, Category = DataPins)
	EFlowDataMultiType MultiType = EFlowDataMultiType::Single;
#endif

public:
	FFlowDataPinValue() {}
	virtual ~FFlowDataPinValue() {}

#if WITH_EDITOR
	FLOW_API bool IsInputPin() const { return bIsInputPin; }
	FLOW_API bool IsArray() const { FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2); return MultiType == EFlowDataMultiType::Array; }

	// Helper to get the Values property handle (implemented by subclasses or via type system)
	FLOW_API virtual TSharedPtr<IPropertyHandle> GetValuesPropertyHandle() const { return nullptr; }

	// Optional SubCategory object source (now moved off the Type class).
	// Implementations can return e.g. Enum asset, struct UScriptStruct, etc. Default: nullptr.
	FLOW_API virtual UObject* GetSubCategoryObject() const { return nullptr; }
#endif

	// Pin Type Name (identity)
	FLOW_API virtual const FFlowPinTypeName& GetPinTypeName() const PURE_VIRTUAL(GetPinTypeName, return FFlowDataPinTypeNamesStandard::UnknownPinTypeName;)

	// Resolve the registered data pin type
	FLOW_API const FFlowDataPinType* LookupDataPinType() const;

	// Populate a result object from this value (property + container context)
	FLOW_API bool PopulateResult(const FProperty* Property, const UObject* Container, FFlowDataPinResult& OutResult) const;
};