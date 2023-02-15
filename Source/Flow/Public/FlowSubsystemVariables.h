// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"
#include "Nodes/FlowVariable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlowSubsystemVariables.generated.h"

UENUM(BlueprintType)
enum class EFlowVariableSetResult : uint8
{
	Success,			// Operation succeeded.
	TypeMismatch,		// Tried to access mismatching type (e.g. setting a struct to bool)
	OutOfBounds,		// Tried to access an array property out of bounds.
	PropertyNotFound,	// Could not find property of specified name.
	MemberNameNone		// Member name was none.
};

/**
 * Flow Subsystem Variables
 * - manages variables for Flow Graphs
 */
UCLASS()
class FLOW_API UFlowSubsystemVariables : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

//////////////////////////////////////////////////////////////////////////
// Variables

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Bool"))
	EFlowVariableSetResult SetBoolVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const bool Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Byte"))
	EFlowVariableSetResult SetByteVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const uint8 Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Int32"))
	EFlowVariableSetResult SetInt32Variable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const int32 Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Int64"))
	EFlowVariableSetResult SetInt64Variable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const int64 Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Float"))
	EFlowVariableSetResult SetFloatVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const float Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Double"))
	EFlowVariableSetResult SetDoubleVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const double Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Name"))
	EFlowVariableSetResult SetNameVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FName Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set String"))
	EFlowVariableSetResult SetStringVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FString Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Text"))
	EFlowVariableSetResult SetTextVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FText Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Object"))
	EFlowVariableSetResult SetObjectVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, UObject* Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Class"))
	EFlowVariableSetResult SetClassVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, UClass* Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Vector"))
	EFlowVariableSetResult SetVectorVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FVector Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Vector (2D)"))
	EFlowVariableSetResult SetVector2DVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FVector2D Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Vector4"))
	EFlowVariableSetResult SetVector4Variable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FVector4 Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Int Vector"))
	EFlowVariableSetResult SetIntVectorVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FIntVector Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Int Vector (2D)"))
	EFlowVariableSetResult SetIntVector2DVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FIntPoint Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Rotator"))
	EFlowVariableSetResult SetRotatorVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FRotator Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Transform"))
	EFlowVariableSetResult SetTransformVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FTransform Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set GameplayTag"))
	EFlowVariableSetResult SetGameplayTagVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FGameplayTag Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set GameplayTag Container"))
	EFlowVariableSetResult SetGameplayTagContainerVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FGameplayTagContainer Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Date Time"))
	EFlowVariableSetResult SetDateTimeVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FDateTime Value);

	UFUNCTION(BlueprintCallable, Category = FlowSubsystemVariables, meta = (DisplayName = "Set Quat"))
	EFlowVariableSetResult SetQuatVariable(UPARAM(ref) FFlowPropertyBag& Property, const FName MemberName, const FQuat Value);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Bool"))
	bool GetVariableBool(const FFlowPropertyBag& PropertyReference, const FName MemberName, bool& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Byte"))
	bool GetVariableByte(const FFlowPropertyBag& PropertyReference, const FName MemberName, uint8& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Int32"))
	bool GetVariableInteger(const FFlowPropertyBag& PropertyReference, const FName MemberName, int32& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Int64"))
	bool GetVariableInteger64(const FFlowPropertyBag& PropertyReference, const FName MemberName, int64& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Float"))
	bool GetVariableFloat(const FFlowPropertyBag& PropertyReference, const FName MemberName, float& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Double"))
	bool GetVariableDouble(const FFlowPropertyBag& PropertyReference, const FName MemberName, double& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Name"))
	bool GetVariableName(const FFlowPropertyBag& PropertyReference, const FName MemberName, FName& OutValue);
	
	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get String"))
	bool GetVariableString(const FFlowPropertyBag& PropertyReference, const FName MemberName, FString& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Text"))
	bool GetVariableText(const FFlowPropertyBag& PropertyReference, const FName MemberName, FText& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Object"))
	bool GetVariableObject(const FFlowPropertyBag& PropertyReference, const FName MemberName, UObject*& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Class"))
	bool GetVariableClass(const FFlowPropertyBag& PropertyReference, const FName MemberName, UClass*& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Vector"))
	bool GetVariableVector(const FFlowPropertyBag& PropertyReference, const FName MemberName, FVector& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Vector (2D)"))
	bool GetVariableVector2D(const FFlowPropertyBag& PropertyReference, const FName MemberName, FVector2D& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Vector4"))
	bool GetVariableVector4(const FFlowPropertyBag& PropertyReference, const FName MemberName, FVector4& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Int Vector"))
	bool GetVariableIntVector(const FFlowPropertyBag& PropertyReference, const FName MemberName, FIntVector& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Int Vector (2D)"))
	bool GetVariableIntVector2(const FFlowPropertyBag& PropertyReference, const FName MemberName, FIntPoint& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Rotator"))
	bool GetVariableRotator(const FFlowPropertyBag& PropertyReference, const FName MemberName, FRotator& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Transform"))
	bool GetVariableTransform(const FFlowPropertyBag& PropertyReference, const FName MemberName, FTransform& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get GameplayTag"))
	bool GetVariableGameplayTag(const FFlowPropertyBag& PropertyReference, const FName MemberName, FGameplayTag& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get GameplayTag Container"))
	bool GetVariableGameplayTagContainer(const FFlowPropertyBag& PropertyReference, const FName MemberName, FGameplayTagContainer& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Date Time"))
	bool GetVariableDateTime(const FFlowPropertyBag& PropertyReference, const FName MemberName, FDateTime& OutValue);

	UFUNCTION(BlueprintPure, Category = FlowSubsystemVariables, meta = (DisplayName = "Get Quat"))
	bool GetVariableQuat(const FFlowPropertyBag& PropertyReference, const FName MemberName, FQuat& OutValue);
};
