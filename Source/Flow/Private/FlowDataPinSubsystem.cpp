// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowDataPinSubsystem.h"
#include "FlowLogChannels.h"
#include "Types/FlowDataPinTypesStandard.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinSubsystem)

UFlowDataPinSubsystem* UFlowDataPinSubsystem::Get()
{
	return GEngine->GetEngineSubsystem<UFlowDataPinSubsystem>();
}

void UFlowDataPinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register standard types
	TInstancedStruct<FFlowDataPinType> Type;

	Type.InitializeAs<FFlowDataPinType_Bool>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Int>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Int64>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Float>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Double>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Name>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_String>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Text>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Enum>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Vector>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Rotator>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Transform>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_GameplayTag>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_GameplayTagContainer>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_InstancedStruct>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Object>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_InstancedObject>();
	RegisterDataPinType(Type);

	Type.InitializeAs<FFlowDataPinType_Class>();
	RegisterDataPinType(Type);
}

void UFlowDataPinSubsystem::Deinitialize()
{
	UnregisterAllDataPinTypes();

	Super::Deinitialize();
}

void UFlowDataPinSubsystem::RegisterDataPinType(const TInstancedStruct<FFlowDataPinType>& DataPinType)
{
	const FFlowDataPinType& BaseType = DataPinType.Get<FFlowDataPinType>();
	const FFlowPinTypeName& TypeName = BaseType.GetPinTypeName();

	DataPinTypes.Add(TypeName, DataPinType);
}

void UFlowDataPinSubsystem::UnregisterDataPinType(const FFlowPinTypeName& TypeName)
{
	DataPinTypes.Remove(TypeName);
}

TArray<FFlowPinTypeName> UFlowDataPinSubsystem::GetDataPinTypeNames() const
{
	TArray<FFlowPinTypeName> TypeNames;
	DataPinTypes.GetKeys(TypeNames);

	return TypeNames;
}