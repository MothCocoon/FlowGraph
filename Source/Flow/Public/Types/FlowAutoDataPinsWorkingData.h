// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowPin.h"

#include "FlowAutoDataPinsWorkingData.generated.h"

class IFlowDataPinValueOwnerInterface;
class UFlowNode;
class UObject;
struct FFlowDataPinValue;

/**
 * Entry in MapDataPinNameToPropertySource for how to source a non-trivial pin mapping in TryGatherPropertyOwnersAndPopulateResult.
 */
USTRUCT()
struct FFlowPinPropertySource
{
	GENERATED_BODY()

	FFlowPinPropertySource() = default;
	FFlowPinPropertySource(const FName& InPropertyName, int32 InValueOwnerIndex)
		: PropertyName(InPropertyName)
		, ValueOwnerIndex(InValueOwnerIndex)
	{ }

	UPROPERTY()
	FName PropertyName;

	UPROPERTY()
	int32 ValueOwnerIndex = INDEX_NONE;
};

/**
 * This is a record of a data pin 'value owner' (which must implement the IFlowDataPinValueOwnerInterface)
 * It includes the owner pointer itself and the index of the value owner in the FFlowDataPinValueOwnerCollection
 */
struct FFlowDataPinValueOwner
{
	explicit FFlowDataPinValueOwner(
		IFlowDataPinValueOwnerInterface& InOwnerInterface,
		int32 InValueOwnerIndex)
		: OwnerInterface(&InOwnerInterface)
		, ValueOwnerIndex(InValueOwnerIndex)
	{
		check(IsValid());
	}

	FName GetValueOwnerName() const
	{
		if (const UObject* ValueOwnerAsObject = GetValueOwnerAsObject())
		{
			return ValueOwnerAsObject->GetFName();
		}

		return NAME_None;
	}

	UObject* GetValueOwnerAsObject();
	const UObject* GetValueOwnerAsObject() const;

	bool IsValid() const
	{
		return OwnerInterface != nullptr && ValueOwnerIndex != INDEX_NONE;
	}

	// The 0th ValueOwnerIndex is the default value owner
	bool IsDefaultValueOwner() const { return ValueOwnerIndex == 0; }

	IFlowDataPinValueOwnerInterface* OwnerInterface = nullptr;

	int32 ValueOwnerIndex = INDEX_NONE;
};

/**
 * A collection of ValueOwner structs that is gathered for use in auto-pin generation and also
 * in runtime pin lookup.
 */
struct FFlowDataPinValueOwnerCollection
{
public:
	void AddValueOwner(IFlowDataPinValueOwnerInterface& ValueOwnerInterface);

	TArray<FFlowDataPinValueOwner>& GetValueOwners() { return ValueOwners; }

	bool IsEmpty() const { return ValueOwners.IsEmpty(); }

protected:

	TArray<FFlowDataPinValueOwner> ValueOwners;
};

#if WITH_EDITOR

/**
 * Container for pin data collected during automatic pin generation.
 */
struct FFlowPinSourceData
{
	FFlowPinSourceData(const FFlowPin& InFlowPin, const FFlowDataPinValueOwner& InValueOwner, const FFlowDataPinValue* InDataPinValue = nullptr)
		: FlowPin(InFlowPin)
		, ValueOwner(InValueOwner)
		, DataPinValue(InDataPinValue)
	{
	}

	FFlowPin FlowPin;
	FFlowDataPinValueOwner ValueOwner;
	const FFlowDataPinValue* DataPinValue = nullptr;
};

/**
 * Transient working data used during auto-generation of data pins.
 */
struct FFlowAutoDataPinsWorkingData
{
public:
	struct FDeferredValuePinNamePatch
	{
		const FFlowDataPinValue* DataPinValue = nullptr;
		FName NewPinName = NAME_None;
	};

	struct FBuildResult
	{
		TArray<FFlowPin> AutoInputPins;
		TArray<FFlowPin> AutoOutputPins;
		TMap<FName, FFlowPinPropertySource> MapDataPinNameToPropertySource;
		TArray<FDeferredValuePinNamePatch> DeferredValuePatches;

		void Reset()
		{
			AutoInputPins.Reset();
			AutoOutputPins.Reset();
			MapDataPinNameToPropertySource.Reset();
			DeferredValuePatches.Reset();
		}
	};

public:
	FFlowAutoDataPinsWorkingData(const TArray<FFlowPin>& InputPinsPrev, const TArray<FFlowPin>& OutputPinsPrev)
		: AutoInputDataPinsPrev(InputPinsPrev)
		, AutoOutputDataPinsPrev(OutputPinsPrev)
	{
	}

	/* Builds the proposed next pins, property source map, and staged wrapper patches. */
	FLOW_API void Build(UFlowNode& FlowNode, FBuildResult& OutBuildResult) const;

	FLOW_API void AddFlowDataPinsForClassProperties(FFlowDataPinValueOwner& ValueOwner);

	static void BuildNextFlowPinArray(const TArray<FFlowPinSourceData>& PinSourceDatas, TArray<FFlowPin>& OutFlowPins);

	static bool CheckIfProposedPinsMatchPreviousPins(const TArray<FFlowPin>& PrevPins, const TArray<FFlowPin>& ProposedPins);

	static bool CheckIfProposedMapMatchesPreviousMap(
		const TMap<FName, FFlowPinPropertySource>& PrevMap,
		const TMap<FName, FFlowPinPropertySource>& ProposedMap);

protected:
	void AddFlowDataPinForProperty(FProperty* Property, FFlowDataPinValueOwner& ValueOwner);

	static bool ArePropertySourcesEqual(const FFlowPinPropertySource& A, const FFlowPinPropertySource& B);

	static void AddPinMappingToMap(
		TMap<FName, FFlowPinPropertySource>& InOutMap,
		const FName& FinalPinName,
		const FName& OriginalPinName,
		const FFlowDataPinValueOwner& ValueOwner);

	static void AppendDeferredPatchIfNeeded(
		TArray<FDeferredValuePinNamePatch>& InOutPatches,
		const FFlowPinSourceData& PinSourceData);

	/* Input pins: map + disambiguate only when same-name pins have mismatched type signatures. */
	static void AddInputDataPinsToMapAndDisambiguate(
		TArray<FFlowPinSourceData>& InOutAutoInputPinsNext,
		TMap<FName, FFlowPinPropertySource>& InOutMap,
		TArray<FDeferredValuePinNamePatch>& InOutDeferredPatches);

	static bool AreFlowPinTypeSignaturesEquivalent(const FFlowPin& A, const FFlowPin& B);

	/* Output pins: existing behavior (always disambiguate duplicates). */
	static void AddOutputDataPinsToMapAndDisambiguate(
		TArray<FFlowPinSourceData>& InOutAutoOutputPinsNext,
		TMap<FName, FFlowPinPropertySource>& InOutMap,
		TArray<FDeferredValuePinNamePatch>& InOutDeferredPatches);

	static void DisambiguateDuplicatePin(
		FFlowPinSourceData& PinSourceData,
		TSet<FName>& InOutUsedNames,
		uint32 LogicalDuplicateIndex,
		TArray<FDeferredValuePinNamePatch>& InOutDeferredPatches);

	static void ApplyDuplicatePresentation(
		FFlowPinSourceData& PinSourceData,
		uint32 LogicalDuplicateIndex);

	static void AppendPinSourceToTooltip(FFlowPinSourceData& PinSourceData);

public:
	const TArray<FFlowPin>& AutoInputDataPinsPrev;
	const TArray<FFlowPin>& AutoOutputDataPinsPrev;

	/* Collected proposals (in provider-defined order) */
	TArray<FFlowPinSourceData> AutoInputDataPinsNext;
	TArray<FFlowPinSourceData> AutoOutputDataPinsNext;
};

#endif