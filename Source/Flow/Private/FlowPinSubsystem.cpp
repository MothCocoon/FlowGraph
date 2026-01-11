// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowPinSubsystem.h"
#include "Types/FlowPinTypesStandard.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPinSubsystem)

UFlowPinSubsystem* UFlowPinSubsystem::Get()
{
	return GEngine->GetEngineSubsystem<UFlowPinSubsystem>();
}

bool UFlowPinSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create an instance if there is no override implementation defined elsewhere
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);
	return (ChildClasses.Num() == 0);
}

void UFlowPinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	check(PinTypes.IsEmpty());

	// Register standard types
	RegisterPinType<FFlowPinType_Bool>();
	RegisterPinType<FFlowPinType_Int>();
	RegisterPinType<FFlowPinType_Int64>();
	RegisterPinType<FFlowPinType_Float>();
	RegisterPinType<FFlowPinType_Double>();
	RegisterPinType<FFlowPinType_Name>();
	RegisterPinType<FFlowPinType_String>();
	RegisterPinType<FFlowPinType_Text>();
	RegisterPinType<FFlowPinType_Enum>();
	RegisterPinType<FFlowPinType_Vector>();
	RegisterPinType<FFlowPinType_Rotator>();
	RegisterPinType<FFlowPinType_Transform>();
	RegisterPinType<FFlowPinType_GameplayTag>();
	RegisterPinType<FFlowPinType_GameplayTagContainer>();
	RegisterPinType<FFlowPinType_InstancedStruct>();
	RegisterPinType<FFlowPinType_Object>();
	RegisterPinType<FFlowPinType_Class>();
}

void UFlowPinSubsystem::Deinitialize()
{
	UnregisterAllPinTypes();

	Super::Deinitialize();
}

void UFlowPinSubsystem::UnregisterAllPinTypes()
{
	const TArray<FFlowPinTypeName> PinTypeNames = GetPinTypeNames();
	for (const FFlowPinTypeName& PinTypeName : PinTypeNames)
	{
		UnregisterPinType(PinTypeName);
	}

	check(PinTypes.IsEmpty());
}

void UFlowPinSubsystem::RegisterPinType(const FFlowPinTypeName& TypeName, const TInstancedStruct<FFlowPinType>& PinType)
{
	PinTypes.Add(TypeName, PinType);
}

void UFlowPinSubsystem::UnregisterPinType(const FFlowPinTypeName& TypeName)
{
	PinTypes.Remove(TypeName);
}

TArray<FFlowPinTypeName> UFlowPinSubsystem::GetPinTypeNames() const
{
	TArray<FFlowPinTypeName> TypeNames;
	PinTypes.GetKeys(TypeNames);

	return TypeNames;
}
