// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDataPinTypeNamesStandard.h"

#include "UObject/NameTypes.h"
#include "Math/Color.h"
#if WITH_EDITOR
#include "GraphEditorSettings.h"
#endif

#include "FlowDataPinType.generated.h"

class FFormatArgumentValue;
class IPropertyHandle;
class UFlowNodeBase;
struct FFlowDataPinResult;

USTRUCT(BlueprintType)
struct FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual ~FFlowDataPinType() {}

	// Lookup a registered type by name
	FLOW_API static const FFlowDataPinType* LookupDataPinType(const FFlowPinTypeName& DataPinTypeName);

	// Identity
	FLOW_API virtual const FFlowPinTypeName& GetPinTypeName() const PURE_VIRTUAL(GetPinTypeName, return FFlowDataPinTypeNamesStandard::UnknownPinTypeName;)

	// Category / Subcategory
	FLOW_API virtual FName GetPinCategory() const PURE_VIRTUAL(GetPinCategory, return NAME_None;)
	FLOW_API virtual UObject* GetSubCategoryObject() const { return nullptr; }

	// Value resolution
	FLOW_API virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const;
	FLOW_API virtual bool PopulateResult(const UFlowNodeBase& Node, const FName& PinName, FFlowDataPinResult& OutResult) const;

#if WITH_EDITOR
	// Editor visualization
	FLOW_API virtual FLinearColor GetPinColor() const { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
	FLOW_API virtual TSharedPtr<IPropertyHandle> GetValuesHandle(const TSharedRef<IPropertyHandle>& FlowDataPinValuePropertyHandle) const;
#endif
};