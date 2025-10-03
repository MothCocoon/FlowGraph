// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetParams.h"
#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "Asset/FlowAssetParamsUtils.h"
#if WITH_EDITOR
#include "SourceControlHelpers.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAssetParams)

void UFlowAssetParams::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	const EFlowReconcilePropertiesResult ReconcileResult = ReconcilePropertiesWithParentParams();
	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ReconcileResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Failed to reconcile ParentParams for %s: %s"),
			*GetPathName(), *UEnum::GetDisplayValueAsText(ReconcileResult).ToString());
	}
#endif
}

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

		if (!Property.DataPinProperty.IsValid())
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Property at index %d has invalid DataPinProperty"), Index)));
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
		if (LocalProp && LocalProp->bIsOverride)
		{
			FFlowNamedDataPinProperty UpdatedProp = *LocalProp;

			// Enforce Parent's name
			UpdatedProp.Name = ParentProp.Name;

			NewProperties.Add(UpdatedProp);

			continue;
		}

		if (LocalProp && FFlowAssetParamsUtils::ArePropertiesEqual(*LocalProp, ParentProp))
		{
			LocalProp->bIsOverride = false;

			// Enforce Parent's name
			LocalProp->Name = ParentProp.Name;

			NewProperties.Add(*LocalProp);

			continue;
		}

		NewProperties.Add(ParentProp);
	}

	for (FFlowNamedDataPinProperty& LocalProp : Properties)
	{
		if (!FFlowAssetParamsUtils::FindPropertyByGuid(ParentProps, LocalProp.Guid))
		{
			LocalProp.bIsOverride = true;

			NewProperties.Add(LocalProp);
		}
	}

	if (FFlowAssetParamsUtils::ArePropertyArraysEqual(NewProperties, Properties))
	{
		return EFlowReconcilePropertiesResult::NoChanges;
	}

	Properties = NewProperties;

	(void) TryCheckOutFromSourceControl();

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

bool UFlowAssetParams::TryCheckOutFromSourceControl() const
{
	if (!USourceControlHelpers::IsAvailable())
	{
		return true;
	}

	const FString FileName = USourceControlHelpers::PackageFilename(GetPathName());
	if (!USourceControlHelpers::CheckOutOrAddFile(FileName))
	{
		UE_LOG(LogFlow, Warning, TEXT("%s is not checked out; properties updated in-memory only"), *GetPathName());
		return false;
	}

	return true;
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
			PropertyMap.Add(Prop.Name, Prop.DataPinProperty);
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

FFlowDataPinResult_Bool UFlowAssetParams::TrySupplyDataPinAsBool_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Bool::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Bool& BoolProp = Found->Get<FFlowDataPinOutputProperty_Bool>();
			return FFlowDataPinResult_Bool(BoolProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Bool pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Bool(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Int UFlowAssetParams::TrySupplyDataPinAsInt_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		const UScriptStruct* Struct = Found->GetScriptStruct();
		if (Struct->IsChildOf(FFlowDataPinOutputProperty_Int64::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Int64& IntProp = Found->Get<FFlowDataPinOutputProperty_Int64>();
			return FFlowDataPinResult_Int(IntProp.Value);
		}
		else if (Struct->IsChildOf(FFlowDataPinOutputProperty_Int32::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Int32& IntProp = Found->Get<FFlowDataPinOutputProperty_Int32>();
			return FFlowDataPinResult_Int(static_cast<int64>(IntProp.Value));
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Int pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Int(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Float UFlowAssetParams::TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		const UScriptStruct* Struct = Found->GetScriptStruct();
		if (Struct->IsChildOf(FFlowDataPinOutputProperty_Double::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Double& FloatProp = Found->Get<FFlowDataPinOutputProperty_Double>();
			return FFlowDataPinResult_Float(FloatProp.Value);
		}
		else if (Struct->IsChildOf(FFlowDataPinOutputProperty_Float::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Float& FloatProp = Found->Get<FFlowDataPinOutputProperty_Float>();
			return FFlowDataPinResult_Float(static_cast<double>(FloatProp.Value));
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Float pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Float(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Name UFlowAssetParams::TrySupplyDataPinAsName_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Name::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Name& NameProp = Found->Get<FFlowDataPinOutputProperty_Name>();
			return FFlowDataPinResult_Name(NameProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Name pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Name(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_String UFlowAssetParams::TrySupplyDataPinAsString_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_String::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_String& StringProp = Found->Get<FFlowDataPinOutputProperty_String>();
			return FFlowDataPinResult_String(StringProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for String pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_String(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Text UFlowAssetParams::TrySupplyDataPinAsText_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Text::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Text& TextProp = Found->Get<FFlowDataPinOutputProperty_Text>();
			return FFlowDataPinResult_Text(TextProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Text pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Text(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Enum UFlowAssetParams::TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Enum::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Enum& EnumProp = Found->Get<FFlowDataPinOutputProperty_Enum>();
			return FFlowDataPinResult_Enum(EnumProp.Value, EnumProp.EnumClass);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Enum pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Enum(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Vector UFlowAssetParams::TrySupplyDataPinAsVector_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Vector::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Vector& VectorProp = Found->Get<FFlowDataPinOutputProperty_Vector>();
			return FFlowDataPinResult_Vector(VectorProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Vector pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Vector(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Rotator UFlowAssetParams::TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Rotator::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Rotator& RotatorProp = Found->Get<FFlowDataPinOutputProperty_Rotator>();
			return FFlowDataPinResult_Rotator(RotatorProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Rotator pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Rotator(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Transform UFlowAssetParams::TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Transform::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Transform& TransformProp = Found->Get<FFlowDataPinOutputProperty_Transform>();
			return FFlowDataPinResult_Transform(TransformProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Transform pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Transform(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_GameplayTag UFlowAssetParams::TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_GameplayTag::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_GameplayTag& TagProp = Found->Get<FFlowDataPinOutputProperty_GameplayTag>();
			return FFlowDataPinResult_GameplayTag(TagProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for GameplayTag pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_GameplayTag(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_GameplayTagContainer UFlowAssetParams::TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_GameplayTagContainer::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_GameplayTagContainer& ContainerProp = Found->Get<FFlowDataPinOutputProperty_GameplayTagContainer>();
			return FFlowDataPinResult_GameplayTagContainer(ContainerProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for GameplayTagContainer pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_GameplayTagContainer(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_InstancedStruct UFlowAssetParams::TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_InstancedStruct::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_InstancedStruct& StructProp = Found->Get<FFlowDataPinOutputProperty_InstancedStruct>();
			return FFlowDataPinResult_InstancedStruct(StructProp.Value);
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for InstancedStruct pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_InstancedStruct(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Object UFlowAssetParams::TrySupplyDataPinAsObject_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Object::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Object& ObjectProp = Found->Get<FFlowDataPinOutputProperty_Object>();
			return FFlowDataPinResult_Object(ObjectProp.GetObjectValue());
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Object pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Object(EFlowDataPinResolveResult::FailedUnknownPin);
}

FFlowDataPinResult_Class UFlowAssetParams::TrySupplyDataPinAsClass_Implementation(const FName& PinName) const
{
	if (const TInstancedStruct<FFlowDataPinProperty>* Found = PropertyMap.Find(PinName))
	{
		if (Found->GetScriptStruct()->IsChildOf(FFlowDataPinOutputProperty_Class::StaticStruct()))
		{
			const FFlowDataPinOutputProperty_Class& ClassProp = Found->Get<FFlowDataPinOutputProperty_Class>();
			return FFlowDataPinResult_Class(ClassProp.GetResolvedClass());
		}

		UE_LOG(LogFlow, Warning, TEXT("Type mismatch for Class pin %s in %s"), *PinName.ToString(), *GetPathName());
	}

	return FFlowDataPinResult_Class(EFlowDataPinResolveResult::FailedUnknownPin);
}