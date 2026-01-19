// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetParams.h"
#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "Asset/FlowAssetParamsUtils.h"
#include "Types/FlowDataPinValuesStandard.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAssetParams)

#if WITH_EDITOR
void UFlowAssetParams::PostLoad()
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject))
	{
		// Migrate the named properties over to the new structs

		bool bMadeAnyChanges = false;
		for (FFlowNamedDataPinProperty& NamedProperty : Properties)
		{
			bMadeAnyChanges |= NamedProperty.FixupDataPinProperty();
		}
		
		if (bMadeAnyChanges)
		{
			ModifyAndRebuildPropertiesMap();
		}
	}

	const EFlowReconcilePropertiesResult ReconcileResult = ReconcilePropertiesWithParentParams();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ReconcileResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Failed to reconcile ParentParams during PostLoad() for %s: %s"),
			*GetPathName(), *UEnum::GetDisplayValueAsText(ReconcileResult).ToString());
	}
}

void UFlowAssetParams::PreSaveRoot(FObjectPreSaveRootContext ObjectSaveContext)
{
	Super::PreSaveRoot(ObjectSaveContext);

	const EFlowReconcilePropertiesResult ReconcileResult = ReconcilePropertiesWithParentParams();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ReconcileResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Failed to reconcile ParentParams during PreSaveRoot() for %s: %s"),
			*GetPathName(), *UEnum::GetDisplayValueAsText(ReconcileResult).ToString());
	}
}
#endif

void UFlowAssetParams::Serialize(FArchive& Ar)
{
#if WITH_EDITOR
	if (Ar.IsCooking())
	{
		const EFlowReconcilePropertiesResult ReconcileResult = ReconcilePropertiesWithParentParams();
		if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ReconcileResult))
		{
			UE_LOG(LogFlow, Error, TEXT("Failed to reconcile ParentParams during cooking for %s: %s"),
				*GetPathName(), *UEnum::GetDisplayValueAsText(ReconcileResult).ToString());
		}
	}

#endif
	Super::Serialize(Ar);
}

UFlowAsset* UFlowAssetParams::ProvideFlowAsset() const
{
#if WITH_EDITOR
	return OwnerFlowAsset.LoadSynchronous();
#else
	// We don't have knowledge of the OwnerFlowAsset in non-editor builds
	checkNoEntry();
	return nullptr;
#endif
}

#if WITH_EDITOR
EDataValidationResult UFlowAssetParams::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (OwnerFlowAsset.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("OwnerFlowAsset is null")));
		Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
	}
	else if (!OwnerFlowAsset.IsValid() && !OwnerFlowAsset.LoadSynchronous())
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("Failed to load OwnerFlowAsset: %s"), *OwnerFlowAsset.ToString())));
		Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
	}

	const EFlowReconcilePropertiesResult CycleResult = CheckForParentCycle();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(CycleResult))
	{
		Context.AddError(FText::FromString(TEXT("Cyclic inheritance detected")));
		Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
	}

	TSet<FGuid> SeenGuids;
	for (int32 Index = 0; Index < Properties.Num(); ++Index)
	{
		const FFlowNamedDataPinProperty& Property = Properties[Index];
		if (Property.Name == NAME_None)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Property at index %d has invalid name"), Index)));
			Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
		}

		if (!Property.DataPinValue.IsValid())
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Property at index %d has invalid DataPinValue"), Index)));
			Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
		}

		if (!Property.Guid.IsValid())
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Property at index %d has invalid Guid"), Index)));
			Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
		}
		else if (SeenGuids.Contains(Property.Guid))
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate Guid found for property at index %d"), Index)));
			Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
		}
		else
		{
			SeenGuids.Add(Property.Guid);
		}

		if (Property.bMayChangeNameAndType)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Property at index %d has bMayChangeNameAndType = true in UFlowAssetParams"), Index)));
			Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
		}
	}

	return Result;
}

EFlowReconcilePropertiesResult UFlowAssetParams::ReconcilePropertiesWithStartNode(
	const FDateTime& FlowAssetLastSaveTimeStamp,
	const TSoftObjectPtr<UFlowAsset>& InOwnerFlowAsset,
	TArray<FFlowNamedDataPinProperty>& MutablePropertiesFromStartNode)
{
	OwnerFlowAsset = InOwnerFlowAsset;

	if (OwnerFlowAsset.IsNull())
	{
		return EFlowReconcilePropertiesResult::Error_InvalidAsset;
	}

	const EFlowReconcilePropertiesResult PropertiesMatchResult = FFlowAssetParamsUtils::CheckPropertiesMatch(Properties, MutablePropertiesFromStartNode);
	const FDateTime ParamsTimestamp = FFlowAssetParamsUtils::GetLastSavedTimestampForObject(this);

	if (FlowAssetLastSaveTimeStamp >= ParamsTimestamp ||
		EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(PropertiesMatchResult))
	{
		ConfigureFlowAssetParams(InOwnerFlowAsset, nullptr, MutablePropertiesFromStartNode);

		return EFlowReconcilePropertiesResult::ParamsPropertiesUpdated;
	}

	MutablePropertiesFromStartNode = Properties;

	FFlowNamedDataPinProperty::ConfigurePropertiesForFlowAssetParams(MutablePropertiesFromStartNode);

	return EFlowReconcilePropertiesResult::AssetPropertyValuesUpdated;
}

EFlowReconcilePropertiesResult UFlowAssetParams::ReconcilePropertiesWithParentParams()
{
	const EFlowReconcilePropertiesResult CycleResult = CheckForParentCycle();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(CycleResult))
	{
		return CycleResult;
	}

	if (ParentParams.AssetPtr.IsNull())
	{
		return EFlowReconcilePropertiesResult::NoChanges;
	}

	UFlowAssetParams* Parent = ParentParams.AssetPtr.LoadSynchronous();
	if (!Parent)
	{
		UE_LOG(LogFlow, Warning, TEXT("Failed to load ParentParams: %s"), *ParentParams.AssetPtr.ToString());

		return EFlowReconcilePropertiesResult::Error_UnloadableParent;
	}

	const EFlowReconcilePropertiesResult ParentResult = Parent->ReconcilePropertiesWithParentParams();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ParentResult))
	{
		return ParentResult;
	}

	const TArray<FFlowNamedDataPinProperty>& ParentProps = Parent->Properties;
	TArray<FFlowNamedDataPinProperty> NewProperties;

	for (const FFlowNamedDataPinProperty& ParentProp : ParentProps)
	{
		FFlowNamedDataPinProperty* LocalProp = FFlowAssetParamsUtils::FindPropertyByGuid(Properties, ParentProp.Guid);
		if (LocalProp != nullptr)
		{
			// We have a version of ParentProp locally.
			// Determine if our local property has been modified since our last reconcile.
			// A local property is considered modified if we've never added it to PropertyMap or is different from what currently exists in PropertyMap.
			bool bLocalPropHasChanged = true;
			if (PropertyMap.Contains(LocalProp->Name))
			{
				FFlowNamedDataPinProperty PreviousLocalProp = *LocalProp;
				PreviousLocalProp.DataPinValue = PropertyMap[LocalProp->Name];
				bLocalPropHasChanged = !FFlowAssetParamsUtils::ArePropertiesEqual(*LocalProp, PreviousLocalProp);
			}

			if (bLocalPropHasChanged)
			{
				// If the local property has been changed then compare it to the parent value to determine if it is an override or not.
				if (FFlowAssetParamsUtils::ArePropertiesEqual(*LocalProp, ParentProp))
				{
					FFlowNamedDataPinProperty& NewProp = NewProperties.Add_GetRef(ParentProp);
					NewProp.bIsOverride = false;
				}
				else
				{
					FFlowNamedDataPinProperty& NewProp = NewProperties.Add_GetRef(*LocalProp);
					NewProp.Name = ParentProp.Name;
					NewProp.bIsOverride = true;
				}
			}
			else
			{
				// If the local property has not been changed then check whether it is an override.
				// Overrides will get copied over while non-overrides will be updated to match the parent.
				if (LocalProp->bIsOverride)
				{
					FFlowNamedDataPinProperty& NewProp = NewProperties.Add_GetRef(*LocalProp);
					NewProp.Name = ParentProp.Name;
					NewProp.bIsOverride = true;
				}
				else
				{
					FFlowNamedDataPinProperty& NewProp = NewProperties.Add_GetRef(ParentProp);
					NewProp.bIsOverride = false;
				}
			}
		}
		else
		{
			// We do not have a version of ParentProp. Just make a non-override copy.
			FFlowNamedDataPinProperty& NewProp = NewProperties.Add_GetRef(ParentProp);
			NewProp.bIsOverride = false;
		}
	}

	for (FFlowNamedDataPinProperty& LocalProp : Properties)
	{
		if (!FFlowAssetParamsUtils::FindPropertyByGuid(ParentProps, LocalProp.Guid))
		{
			LocalProp.bIsOverride = true;

			NewProperties.Add(LocalProp);
		}
	}

	Properties = NewProperties;

	ModifyAndRebuildPropertiesMap();

	return EFlowReconcilePropertiesResult::ParamsPropertiesUpdated;
}

void UFlowAssetParams::ConfigureFlowAssetParams(TSoftObjectPtr<UFlowAsset> OwnerAsset, TSoftObjectPtr<UFlowAssetParams> InParentParams, const TArray<FFlowNamedDataPinProperty>& InProperties)
{
	ParentParams.AssetPtr = InParentParams;
	OwnerFlowAsset = OwnerAsset;
	Properties = InProperties;
	FFlowNamedDataPinProperty::ConfigurePropertiesForFlowAssetParams(Properties);

	ModifyAndRebuildPropertiesMap();
}

bool UFlowAssetParams::CanModifyFlowDataPinType() const
{
	// These are set by the Flow asset, which is authoritative
	return false;
}

bool UFlowAssetParams::ShowFlowDataPinValueInputPinCheckbox() const
{
	// These are set by the Flow asset, which is authoritative
	return false;
}

bool UFlowAssetParams::ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const
{
	return true;
}

bool UFlowAssetParams::CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const
{
	// These are set by the Flow asset, which is authoritative
	return false;
}

EFlowReconcilePropertiesResult UFlowAssetParams::CheckForParentCycle() const
{
	TSet<TSoftObjectPtr<UFlowAssetParams>> Visited;
	TSoftObjectPtr<UFlowAssetParams> Current = ParentParams.AssetPtr;

	while (!Current.IsNull())
	{
		if (Visited.Contains(Current))
		{
			UE_LOG(LogFlow, Error, TEXT("Cyclic inheritance detected at: %s"), *Current.ToString());
			return EFlowReconcilePropertiesResult::Error_CyclicInheritance;
		}

		Visited.Add(Current);
		const UFlowAssetParams* CurrentParams = Current.LoadSynchronous();
		if (!CurrentParams)
		{
			UE_LOG(LogFlow, Warning, TEXT("Failed to load ParentParams: %s"), *Current.ToString());
			return EFlowReconcilePropertiesResult::Error_UnloadableParent;
		}

		Current = CurrentParams->ParentParams.AssetPtr;
	}

	return EFlowReconcilePropertiesResult::NoChanges;
}

void UFlowAssetParams::ModifyAndRebuildPropertiesMap()
{
	Modify();

	RebuildPropertiesMap();

	MarkPackageDirty();
}

void UFlowAssetParams::RebuildPropertiesMap()
{
	PropertyMap.Reset();

	for (const FFlowNamedDataPinProperty& Prop : Properties)
	{
		if (Prop.IsValid())
		{
			PropertyMap.Add(Prop.Name, Prop.DataPinValue);
		}
		else
		{
			UE_LOG(LogFlow, Warning, TEXT("Skipping invalid property %s during rebuild for %s"), *Prop.Name.ToString(), *GetPathName());
		}
	}
}
#endif

bool UFlowAssetParams::CanSupplyDataPinValues_Implementation() const
{
	return !PropertyMap.IsEmpty();
}

FFlowDataPinResult UFlowAssetParams::TrySupplyDataPin_Implementation(FName PinName) const
{
	if (const TInstancedStruct<FFlowDataPinValue>* Found = PropertyMap.Find(PinName))
	{
		FFlowDataPinResult DataPinResult(EFlowDataPinResolveResult::Success);
		DataPinResult.ResultValue = (*Found);

		return DataPinResult;
	}

	return FFlowDataPinResult(EFlowDataPinResolveResult::FailedUnknownPin);
}
