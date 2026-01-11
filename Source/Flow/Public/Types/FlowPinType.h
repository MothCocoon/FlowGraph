// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowPinEnums.h"

#include "Math/Color.h"
#include "UObject/NameTypes.h"

#if WITH_EDITOR
#include "GraphEditorSettings.h"
#endif

#include "FlowPinType.generated.h"

class FFormatArgumentValue;
class IPropertyHandle;
class UFlowNodeBase;
class UFlowNode;
struct FFlowDataPinResult;
struct FFlowPin;
struct FFlowDataPinValue;

USTRUCT(BlueprintType)
struct FFlowPinType
{
	GENERATED_BODY()

public:
	virtual ~FFlowPinType() {}

	// Lookup a registered type by name
	FLOW_API static const FFlowPinType* LookupPinType(const FFlowPinTypeName& FlowPinTypeName);

	// Identity
	FLOW_API virtual const FFlowPinTypeName& GetPinTypeName() const PURE_VIRTUAL(GetPinTypeName, return PinTypeNameUnknown;);

	// Value resolution
	FLOW_API virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const;
	FLOW_API virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const;

#if WITH_EDITOR
	// Editor visualization
	FLOW_API virtual FLinearColor GetPinColor() const { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
	FLOW_API virtual TSharedPtr<IPropertyHandle> GetValuesHandle(const TSharedRef<IPropertyHandle>& FlowDataPinValuePropertyHandle) const;
	FLOW_API virtual bool SupportsMultiType(EFlowDataMultiType Mode) const { return true; }
	FLOW_API virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const { return nullptr; }

	// Pin creation
	FLOW_API FFlowPin CreateFlowPinFromProperty(const FProperty& Property, void const* InContainer) const;
	FLOW_API FFlowPin CreateFlowPinFromValueWrapper(const FName& PinName, const FFlowDataPinValue& Wrapper) const;
#endif

	static const FFlowPinTypeName PinTypeNameUnknown;
};