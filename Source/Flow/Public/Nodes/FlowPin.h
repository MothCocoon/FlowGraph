// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowPinEnums.h"
#include "Types/FlowPinTypeName.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"
#include "Types/FlowPinTypeNamesStandard.h"
#include "EdGraph/EdGraphPin.h"

#include "FlowPin.generated.h"

class UEnum;
class UClass;
class UObject;
class IPropertyHandle;
struct FFlowPinType;

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStruct", HasNativeBreak = "/Script/Flow.FlowDataPinBlueprintLibrary.BreakStruct"))
struct FLOW_API FFlowPin
{
	GENERATED_BODY()

	// A logical name, used during execution of pin
	UPROPERTY(EditDefaultsOnly, Category = DataPins)
	FName PinName;

	// An optional Display Name, you can use it to override PinName without the need to update graph connections
	UPROPERTY(EditDefaultsOnly, Category = DataPins)
	FText PinFriendlyName;

	UPROPERTY(EditDefaultsOnly, Category = DataPins)
	FString PinToolTip;

	// PinType (implies PinCategory)
	UPROPERTY(Meta = (DeprecatedProperty, DeprecationMessage = "Use PinTypeName instead"))
	EFlowPinType PinType = EFlowPinType::Invalid;

	// Only supporting None (Single) or Array for now(tm) for data pins via EFlowMultiType
	UPROPERTY()
	EPinContainerType ContainerType = EPinContainerType::None;

protected:
	UPROPERTY(EditDefaultsOnly, Category = DataPins)
	FFlowPinTypeName PinTypeName = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameExec);

	// Sub-category object
	// (used to identify the struct or class type for some PinCategories)
	UPROPERTY(VisibleAnywhere, Category = DataPins)
	TWeakObjectPtr<UObject> PinSubCategoryObject;

#if WITH_EDITORONLY_DATA
	// Filter for limiting the compatible classes for this data pin.
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinTypeName matches (for runtime use).
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (EditCondition = "PinTypeName == Class", EditConditionHides))
	TSubclassOf<UClass> SubCategoryClassFilter = UClass::StaticClass();

	// Filter for limiting the compatible object types for this data pin.
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinTypeName matches (for runtime use).
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (EditCondition = "PinTypeName == Object", EditConditionHides))
	TSubclassOf<UObject> SubCategoryObjectFilter = UObject::StaticClass();

	// Configuration option for setting the EnumClass to a Blueprint Enum
	// (C++ enums must bind by name using SubCategoryEnumName, due to a limitation with UE's UEnum discovery).
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for runtime use).
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (EditCondition = "PinTypeName == Enum", EditConditionHides))
	TObjectPtr<UEnum> SubCategoryEnumClass = nullptr;

	// name of enum defined in c++ code, will take priority over asset from EnumType property
	// (this is a work-around because EnumClass cannot find C++ Enums,
	// so you need to type the name of the enum in here, manually)
	// See also: FFlowPin::PostEditChangedEnumName()
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (EditCondition = "PinTypeName == Enum", EditConditionHides))
	FString SubCategoryEnumName;
#endif

public:

	FFlowPin()
		: PinName(NAME_None)
	{
	}

	explicit FFlowPin(const FName& InPinName)
		: PinName(InPinName)
	{
	}

	explicit FFlowPin(const FString& InPinName)
		: PinName(*InPinName)
	{
	}

	explicit FFlowPin(const FText& InPinName)
		: PinName(*InPinName.ToString())
	{
	}

	explicit FFlowPin(const TCHAR* InPinName)
		: PinName(FName(InPinName))
	{
	}

	explicit FFlowPin(const uint8& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	explicit FFlowPin(const int32& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	explicit FFlowPin(const FStringView InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	explicit FFlowPin(const FStringView InPinName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinToolTip(InPinTooltip)
	{
	}

	explicit FFlowPin(const FStringView InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	explicit FFlowPin(const FName& InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	explicit FFlowPin(const FName& InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	explicit FFlowPin(const FName& InPinName, const FText& InPinFriendlyName, const FFlowPinTypeName& InTypeName, UObject* OptionalSubCategoryObject = nullptr)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
		SetPinTypeName(InTypeName);
		SetPinSubCategoryObject(OptionalSubCategoryObject);
	}

	explicit FFlowPin(const FName& InPinName, const FFlowPinTypeName& InTypeName, UObject* OptionalSubCategoryObject = nullptr)
		: PinName(InPinName)
	{
		SetPinTypeName(InTypeName);
		SetPinSubCategoryObject(OptionalSubCategoryObject);
	}

	FORCEINLINE bool IsValid() const
	{
		return !PinName.IsNone();
	}

	FORCEINLINE bool operator==(const FFlowPin& Other) const
	{
		return PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FFlowPin& Other) const
	{
		return PinName != Other.PinName;
	}

	FORCEINLINE bool operator==(const FName& Other) const
	{
		return PinName == Other;
	}

	FORCEINLINE bool operator!=(const FName& Other) const
	{
		return PinName != Other;
	}

	bool DeepIsEqual(const FFlowPin& Other) const
	{
		// Do a deep pin match (not a simple name-only match), to check if the pins are exactly equal
		return 
			PinName == Other.PinName &&
			PinFriendlyName.EqualTo(Other.PinFriendlyName) &&
			PinToolTip == Other.PinToolTip &&
			ContainerType == Other.ContainerType &&
			PinTypeName == Other.PinTypeName &&
			PinSubCategoryObject == Other.PinSubCategoryObject;
	}

	friend uint32 GetTypeHash(const FFlowPin& FlowPin)
	{
		return GetTypeHash(FlowPin.PinName);
	}

public:

#if WITH_EDITOR
	// Must be called from PostEditChangeProperty() by an owning UObject <sigh>
	// whenever PinType, 
	void PostEditChangedPinTypeOrSubCategorySource();
	FText BuildHeaderText() const;

	static bool ValidateEnum(const UEnum& EnumType);
#endif // WITH_EDITOR

	void SetPinTypeName(const FFlowPinTypeName& InTypeName);
	const FFlowPinTypeName& GetPinTypeName() const { return PinTypeName; }
	const FFlowPinType* ResolveFlowPinType() const;
	void SetPinSubCategoryObject(UObject* Object) { PinSubCategoryObject = Object; }
	static FFlowPinTypeName GetPinTypeNameForLegacyPinType(EFlowPinType PinType);

#if WITH_EDITOR
	FEdGraphPinType BuildEdGraphPinType() const;
#endif

	const TWeakObjectPtr<UObject>& GetPinSubCategoryObject() const { return PinSubCategoryObject; }

	FORCEINLINE_DEBUGGABLE static bool DeepArePinArraysMatching(const TArray<FFlowPin>& Left, const TArray<FFlowPin>& Right);

	// FFlowPin instance signatures for "trait" functions
	bool IsExecPin() const;
	static bool IsExecPinCategory(const FName& PC);
	// --

	// Metadata keys for properties that bind and auto-generate Data Pins:

	// SourceForOutputFlowPin
	//   May be used on a non-FFlowDataPinProperty within a UFlowNode to bind the
	//   output data pin to use the property as its source.
	//
	//   If a string value is given, it is interpreted as the Data Pin's name,
	//   otherwise, the property's DisplayName (or lacking that, its authored name)
	//   will be assumed to also be the Pin's name.
	static const FName MetadataKey_SourceForOutputFlowPin;

	// DefaultForInputFlowPin
	//   May be used on a non-FFlowDataPinProperty within a UFlowNode to bind the
	//   input data pin to use the property as its default value.
	//
	//   If the input pin IS NOT connected to another node, then the bound property
	//   value will be supplied as a default.
	// 
	//   If the input pin IS connected to another node, then the connected node's supplied
	//   value will be used instead of the default from the bound property.
	// 
	//   If a string value is given, it is interpreted as the Data Pin's name,
	//   otherwise, the property's DisplayName (or lacking that, its authored name)
	//   will be assumed to also be the Pin's name.
	static const FName MetadataKey_DefaultForInputFlowPin;

	// FlowPinType
	//   May be used on either a property (within a UFlowNode) or a USTRUCT declaration for
	//   a FFlowDataPinProperty subclass.
	//   
	//   If used on a property, then it indicates that a data pin of the given type should be auto-generated,
	//   and bound to the property.  May be used in conjunction with SourceForOutputFlowPin or DefaultForInputFlowPin
	//   (but not both) to determine how the property binding is to be applied (as input default or output supply source)
	//
	//   If used on a FFlowDataPinProperty struct declaration, then it defines the type of pin
	//   that should be auto-generated when the struct is used as a property in a UFlowNode.
	//
	//   The string value of the metadata should exactly match a value in EFlowPinType
	static const FName MetadataKey_FlowPinType;
	// --

protected:

	void TrySetStructSubCategoryObjectFromPinType();
};

// Inline implementations
bool FFlowPin::DeepArePinArraysMatching(const TArray<FFlowPin>& Left, const TArray<FFlowPin>& Right)
{
	if (Left.Num() != Right.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Left.Num(); ++Index)
	{
		if (!Left[Index].DeepIsEqual(Right[Index]))
		{
			return false;
		}
	}

	return true;
}

USTRUCT()
struct FLOW_API FFlowPinHandle
{
	GENERATED_BODY()

	// Update SFlowPinHandleBase code if this property name would be ever changed
	UPROPERTY()
	FName PinName;

	FFlowPinHandle()
		: PinName(NAME_None)
	{
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowInputPinHandle : public FFlowPinHandle
{
	GENERATED_BODY()

	FFlowInputPinHandle()
	{
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowOutputPinHandle : public FFlowPinHandle
{
	GENERATED_BODY()

	FFlowOutputPinHandle()
	{
	}
};

// Processing Flow Nodes creates map of connected pins
USTRUCT()
struct FLOW_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;

	FConnectedPin()
		: NodeGuid(FGuid())
		, PinName(NAME_None)
	{
	}

	FConnectedPin(const FGuid InNodeId, const FName& InPinName)
		: NodeGuid(InNodeId)
		, PinName(InPinName)
	{
	}

	FORCEINLINE bool operator==(const FConnectedPin& Other) const
	{
		return NodeGuid == Other.NodeGuid && PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FConnectedPin& Other) const
	{
		return NodeGuid != Other.NodeGuid || PinName != Other.PinName;
	}

	friend uint32 GetTypeHash(const FConnectedPin& ConnectedPin)
	{
		return GetTypeHash(ConnectedPin.NodeGuid) + GetTypeHash(ConnectedPin.PinName);
	}
};

UENUM(BlueprintType)
enum class EFlowPinActivationType : uint8
{
	Default,
	Forced,
	PassThrough
};

// Every time pin is activated, we record it and display this data while user hovers mouse over pin
#if !UE_BUILD_SHIPPING
struct FLOW_API FPinRecord
{
	double Time;
	FString HumanReadableTime;
	EFlowPinActivationType ActivationType;

	static FString NoActivations;
	static FString PinActivations;
	static FString ForcedActivation;
	static FString PassThroughActivation;

	FPinRecord();
	FPinRecord(const double InTime, const EFlowPinActivationType InActivationType);

private:
	FORCEINLINE static FString DoubleDigit(const int32 Number);
};
#endif
