// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"

#include "FlowPinEnums.h"
#include "FlowPinType.h"
#include "FlowDataPinValue.generated.h"

struct FFlowDataPinResult;
class FProperty;
class UObject;
class IPropertyHandle;
class UScriptStruct;

USTRUCT()
struct FFlowDataPinValue
{
	GENERATED_BODY()

	friend class FFlowDataPinValueCustomization;

public:
	// IF a pin was created from this property, this is the cached pin name that was used
	// (which can be used in UFlowDataPinBlueprintLibrary::ResolveAs... functions to lookup the correct pin by name)
	UPROPERTY(VisibleAnywhere, Category = DataPins)
	mutable FName PropertyPinName;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins)
	bool bIsInputPin = false;

	UPROPERTY(EditAnywhere, Category = DataPins)
	EFlowDataMultiType MultiType = EFlowDataMultiType::Single;
#endif

	FFlowDataPinValue() {}
	virtual ~FFlowDataPinValue() {}

#if WITH_EDITOR
	FLOW_API bool IsInputPin() const { return bIsInputPin; }
	FLOW_API bool IsArray() const { FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2); return MultiType == EFlowDataMultiType::Array; }

	// Helper to get the Values property handle (implemented by subclasses or via type system)
	FLOW_API virtual TSharedPtr<IPropertyHandle> GetValuesPropertyHandle() const PURE_VIRTUAL(GetValuesPropertyHandle, return nullptr;);
#endif

	// Pin Type Name (identity)
	FLOW_API virtual const FFlowPinTypeName& GetPinTypeName() const PURE_VIRTUAL(GetPinTypeName, return FFlowPinType::PinTypeNameUnknown;);

	// (optional) Get the field type if one exists (only used for UEnum For Now)
	FLOW_API virtual UField* GetFieldType() const { return nullptr; }

	// (optional)
	FLOW_API virtual bool TryConvertValuesToString(FString& OutString) const { return false; }

	// Resolve the registered data pin type
	FLOW_API const FFlowPinType* LookupPinType() const;

	FLOW_API static const FString StringArraySeparator;
};