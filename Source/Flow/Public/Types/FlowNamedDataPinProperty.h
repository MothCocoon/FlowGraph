// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"
#include "Nodes/FlowPin.h"
#include "Types/FlowDataPinValue.h"

#include "FlowNamedDataPinProperty.generated.h"

struct FFlowDataPinProperty;
struct FFlowDataPinValue;

// Wrapper for FFlowDataPinProperty that is used for flow nodes that add 
// dynamic properties, with associated data pins, on the flow node instance
// (as opposed to C++ or blueprint compile-time).
USTRUCT(BlueprintType, DisplayName = "Flow Named DataPin Property")
struct FFlowNamedDataPinProperty
{
	GENERATED_BODY()

public:
	// Name of this instanced property
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins, meta = (EditCondition = "bMayChangeNameAndType", HideEditConditionToggle))
	FName Name = NAME_None;

private:
	// DataPinProperty payload
	UPROPERTY(VisibleAnywhere, Category = DataPins, meta = (DeprecatedProperty))
	TInstancedStruct<FFlowDataPinProperty> DataPinProperty;

public:
	// DataPinProperty payload
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (ExcludeBaseStruct, NoClear))
	TInstancedStruct<FFlowDataPinValue> DataPinValue;

#if WITH_EDITORONLY_DATA
	// Unique identifier for property tracking
	UPROPERTY()
	FGuid Guid = FGuid::NewGuid();

	// Tracks if this property overrides its super (auto-clears if matches super)
	UPROPERTY()
	bool bIsOverride = false;

	// TODO (gtaylor) Does not currently police the type, 
	// because that prevents the instanced struct contents being edited as well, 
	// which is not what we want from this feature.  
	// Will try to fix next pass on the details customization.
	UPROPERTY()
	bool bMayChangeNameAndType = true;
#endif

public:
	FFlowNamedDataPinProperty() = default;

	bool IsValid() const { return Name != NAME_None && DataPinValue.GetPtr() != nullptr; }

	// #FlowDataPinLegacy
	bool FixupDataPinProperty();
	// --

#if WITH_EDITOR
	FLOW_API FFlowPin CreateFlowPin() const;

	FLOW_API FText BuildHeaderText() const;

	void ConfigureForFlowAssetParams() 
	{
		bIsOverride = false;
		bMayChangeNameAndType = false;
	}

	void ConfigureForFlowAssetStartNode()
	{
		bIsOverride = false;
		bMayChangeNameAndType = true;
	}

	static void ConfigurePropertiesForFlowAssetParams(TArray<FFlowNamedDataPinProperty>& MutableProperties)
	{
		for (FFlowNamedDataPinProperty& Property : MutableProperties)
		{
			Property.ConfigureForFlowAssetParams();
		}
	}
	static void ConfigurePropertiesForFlowAssetStartNode(TArray<FFlowNamedDataPinProperty>& MutableProperties)
	{
		for (FFlowNamedDataPinProperty& Property : MutableProperties)
		{
			Property.ConfigureForFlowAssetStartNode();
		}
	}

#endif
};

