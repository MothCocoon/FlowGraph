// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "Types/FlowPinType.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/UnrealTypeTraits.h"

#include "FlowPinSubsystem.generated.h"

struct FFlowPinTypeName;

UCLASS(MinimalApi)
class UFlowPinSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TMap<FFlowPinTypeName, TInstancedStruct<FFlowPinType>> PinTypes;
	
public:
	FLOW_API static UFlowPinSubsystem* Get();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	template <typename TPinType>
	void RegisterPinType()
	{
		TInstancedStruct<FFlowPinType> PinType;
		PinType.InitializeAs<TPinType>();
		RegisterPinType(TPinType::GetPinTypeNameStatic(), PinType);
	}
	FLOW_API void RegisterPinType(const FFlowPinTypeName& TypeName, const TInstancedStruct<FFlowPinType>& PinType);

	template <typename TPinType>
	void UnregisterPinType()
	{
		UnregisterPinType(TPinType::GetPinTypeNameStatic());
	}
	FLOW_API void UnregisterPinType(const FFlowPinTypeName& TypeName);

	template <typename TPinType = FFlowPinType>
	const TPinType* FindPinType(const FFlowPinTypeName& TypeName) const
	{
		static_assert(TIsDerivedFrom<TPinType, FFlowPinType>::IsDerived, "TPinType must be derived from FFlowPinType");

		if (const TInstancedStruct<FFlowPinType>* Found = PinTypes.Find(TypeName))
		{
			return Found->GetPtr<TPinType>();
		}

		return nullptr;
	}

	FLOW_API TArray<FFlowPinTypeName> GetPinTypeNames() const;

protected:
	void UnregisterAllPinTypes();
};