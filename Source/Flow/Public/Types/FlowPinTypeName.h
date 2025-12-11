// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/NameTypes.h"

#include "FlowPinTypeName.generated.h"

USTRUCT(BlueprintType)
struct FFlowPinTypeName
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = FlowPin)
	FName Name = NAME_None;

	FFlowPinTypeName() = default;
	explicit FFlowPinTypeName(const TCHAR* InPinName) : Name(FName(InPinName)) {}
	explicit FFlowPinTypeName(const FName& InName) : Name(InName) {}
	explicit FFlowPinTypeName(const FString& InString) : Name(FName(InString)) {}

	friend inline uint32 GetTypeHash(const FFlowPinTypeName& PinTypeName)
	{
		return GetTypeHash(PinTypeName.Name);
	}

	FORCEINLINE bool operator==(const FFlowPinTypeName& Other) const { return Name == Other.Name; }
	FORCEINLINE bool operator==(const FName& OtherName) const { return Name == OtherName; }

	FString ToString() const { return Name.ToString(); }
	bool IsNone() const { return Name.IsNone(); }
};