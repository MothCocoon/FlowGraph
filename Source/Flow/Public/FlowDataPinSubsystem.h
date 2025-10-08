// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "Types/FlowDataPinType.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/UnrealTypeTraits.h"

#include "FlowDataPinSubsystem.generated.h"

UCLASS(MinimalApi)
class UFlowDataPinSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TMap<FFlowPinTypeName, TInstancedStruct<FFlowDataPinType>> DataPinTypes;

public:
	FLOW_API static UFlowDataPinSubsystem* Get();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FLOW_API void RegisterDataPinType(const TInstancedStruct<FFlowDataPinType>& DataPinType);
	FLOW_API void UnregisterDataPinType(const FFlowPinTypeName& TypeName);

	template <typename T = FFlowDataPinType>
	const T* FindDataPinType(const FFlowPinTypeName& TypeName) const
	{
		static_assert(TIsDerivedFrom<T, FFlowDataPinType>::IsDerived, "T must be derived from FFlowDataPinType");

		if (const TInstancedStruct<FFlowDataPinType>* Found = DataPinTypes.Find(TypeName))
		{
			return Found->GetPtr<T>();
		}

		return nullptr;
	}

	FLOW_API TArray<FFlowPinTypeName> GetDataPinTypeNames() const;

protected:
	FORCEINLINE void UnregisterAllDataPinTypes()
	{
		DataPinTypes.Empty();
	}
};