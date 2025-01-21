// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowPinEnums.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"

#include "FlowPin.generated.h"

class UEnum;
class UClass;
class UObject;
class IPropertyHandle;

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStruct", HasNativeBreak = "/Script/Flow.FlowDataPinBlueprintLibrary.BreakStruct"))
struct FLOW_API FFlowPin
{
	GENERATED_BODY()

	// A logical name, used during execution of pin
	UPROPERTY(EditDefaultsOnly, Category = FlowPin)
	FName PinName;

	// An optional Display Name, you can use it to override PinName without the need to update graph connections
	UPROPERTY(EditDefaultsOnly, Category = FlowPin)
	FText PinFriendlyName;

	UPROPERTY(EditDefaultsOnly, Category = FlowPin)
	FString PinToolTip;

protected:
	// PinType (implies PinCategory)
	UPROPERTY(EditAnywhere, Category = FlowPin)
	EFlowPinType PinType = EFlowPinType::Exec;

	// Sub-category object
	// (used to identify the struct or class type for some PinCategories, see IsSubtypeSupportedPinCategory)
	UPROPERTY()
	TWeakObjectPtr<UObject> PinSubCategoryObject;

#if WITH_EDITORONLY_DATA
	// Filter for limiting the compatible classes for this data pin.
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for runtime use).
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Class", EditConditionHides))
	TSubclassOf<UClass> SubCategoryClassFilter = UClass::StaticClass();

	// Filter for limiting the compatible object types for this data pin.
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for runtime use).
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Object", EditConditionHides))
	TSubclassOf<UObject> SubCategoryObjectFilter = UObject::StaticClass();

	// Configuration option for setting the EnumClass to a Blueprint Enum 
	// (C++ enums must bind by name using SubCategoryEnumName, due to a limitation with UE's UEnum discovery).
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for runtime use).
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Enum", EditConditionHides))
	TObjectPtr<UEnum> SubCategoryEnumClass = nullptr;

	// name of enum defined in c++ code, will take priority over asset from EnumType property
	//  (this is a work-around because EnumClass cannot find C++ Enums, 
	//   so you need to type the name of the enum in here, manually)
	// See also: FFlowPin::PostEditChangedEnumName()
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Enum", EditConditionHides))
	FString SubCategoryEnumName;
#endif // WITH_EDITORONLY_DATA

public:

	// PinCategory aliases for (a subset of) those defined in UEdGraphSchema_K2
	static inline FName PC_Exec = TEXT("exec");
	static inline FName PC_Boolean = TEXT("bool");
	static inline FName PC_Byte = TEXT("byte");
	static inline FName PC_Class = TEXT("class");
	static inline FName PC_Int = TEXT("int");
	static inline FName PC_Int64 = TEXT("int64");
	static inline FName PC_Float = TEXT("float");
	static inline FName PC_Double = TEXT("double");
	static inline FName PC_Name = TEXT("name");
	static inline FName PC_Object = TEXT("object");
	static inline FName PC_String = TEXT("string");
	static inline FName PC_Text = TEXT("text");
	static inline FName PC_Struct = TEXT("struct");
	static inline FName PC_Enum = TEXT("enum");

	static inline FName AnyPinName = TEXT("AnyPinName");

	FFlowPin()
		: PinName(NAME_None)
	{
	}

	FFlowPin(const FName& InPinName)
		: PinName(InPinName)
	{
	}

	FFlowPin(const FString& InPinName)
		: PinName(*InPinName)
	{
	}

	FFlowPin(const FText& InPinName)
		: PinName(*InPinName.ToString())
	{
	}

	FFlowPin(const TCHAR* InPinName)
		: PinName(FName(InPinName))
	{
	}

	FFlowPin(const uint8& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FFlowPin(const int32& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FFlowPin(const FStringView InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	FFlowPin(const FStringView InPinName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FStringView InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FName& InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	FFlowPin(const FName& InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FName& InPinName, const FText& InPinFriendlyName, EFlowPinType InFlowPinType, UObject* SubCategoryObject = nullptr)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
		SetPinType(InFlowPinType, SubCategoryObject);
	}

	FFlowPin(const FName& InPinName, EFlowPinType InFlowPinType, UObject* SubCategoryObject = nullptr)
		: PinName(InPinName)
	{
		SetPinType(InFlowPinType, SubCategoryObject);
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

	void SetPinType(EFlowPinType InFlowPinType, UObject* SubCategoryObject = nullptr);
	EFlowPinType GetPinType() const { return PinType; }
	static const FName& GetPinCategoryFromPinType(EFlowPinType FlowPinType);
	static const TArray<FName>& GetFlowPinTypeEnumValuesWithoutSpaces();

	const TWeakObjectPtr<UObject>& GetPinSubCategoryObject() const { return PinSubCategoryObject; }

	static bool ArePinArraysMatchingNamesAndTypes(const TArray<FFlowPin>& Left, const TArray<FFlowPin>& Right);
	static bool DoPinsMatchNamesAndTypes(const FFlowPin& LeftPin, const FFlowPin& RightPin)
	{
		return (LeftPin.PinName == RightPin.PinName && LeftPin.PinType == RightPin.PinType && LeftPin.PinSubCategoryObject == RightPin.PinSubCategoryObject);
	}

	// FFlowPin instance signatures for "trait" functions
	FORCEINLINE bool IsExecPin() const { return PinType == EFlowPinType::Exec; }
	FORCEINLINE bool IsDataPin() const { return PinType != EFlowPinType::Exec; }
	// --

	// PinCategory "trait" functions:
	FORCEINLINE static bool IsExecPinCategory(const FName& PC) { return PC == PC_Exec; }
	FORCEINLINE static bool IsDataPinCategory(const FName& PC) { return PC != PC_Exec; }
	FORCEINLINE static bool IsBoolPinCategory(const FName& PC) { return PC == PC_Boolean; }
	FORCEINLINE static bool IsIntPinCategory(const FName& PC) { return PC == PC_Byte || PC == PC_Int || PC == PC_Int64; }
	FORCEINLINE static bool IsFloatPinCategory(const FName& PC) { return PC == PC_Double || PC == PC_Float; }
	FORCEINLINE static bool IsEnumPinCategory(const FName& PC) { return PC == PC_Enum; }
	FORCEINLINE static bool IsTextPinCategory(const FName& PC) { return PC == PC_Name || PC == PC_String || PC == PC_Text; }
	FORCEINLINE static bool IsObjectPinCategory(const FName& PC) { return PC == PC_Object; }
	FORCEINLINE static bool IsClassPinCategory(const FName& PC) { return PC == PC_Class; }	
	FORCEINLINE static bool IsStructPinCategory(const FName& PC) { return PC == PC_Struct; }
	// --

	// IsConvertable trait functions:
	FORCEINLINE static bool IsConvertableToBoolPinCategory(const FName& PC) { return IsBoolPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToIntPinCategory(const FName& PC) { return IsIntPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToFloatPinCategory(const FName& PC) { return IsFloatPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToEnumPinCategory(const FName& PC) { return IsEnumPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToTextPinCategory(const FName& PC) { return IsTextPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToObjectPinCategory(const FName& PC) { return IsObjectPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToClassPinCategory(const FName& PC) { return IsClassPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToStructPinCategory(const FName& PC) { return IsStructPinCategory(PC); }
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

private:

	// Cached EFlowPinType values as FName, de-spaced, so they can be compared with FlowPinType metadata strings
	static TArray<FName> FlowPinTypeEnumValuesWithoutSpaces;
};

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

// It can represent any trait added on the specific node instance, i.e. breakpoint
USTRUCT()
struct FLOW_API FFlowPinTrait
{
	GENERATED_USTRUCT_BODY()

protected:	
	UPROPERTY()
	uint8 bTraitAllowed : 1;

	uint8 bEnabled : 1;
	uint8 bHit : 1;

public:
	FFlowPinTrait()
		: bTraitAllowed(false)
		, bEnabled(false)
		, bHit(false)
	{
	};

	explicit FFlowPinTrait(const bool bInitialState)
		: bTraitAllowed(bInitialState)
		, bEnabled(bInitialState)
		, bHit(false)
	{
	};

	void AllowTrait();
	void DisallowTrait();
	bool IsAllowed() const;

	void EnableTrait();
	void DisableTrait();
	void ToggleTrait();

	bool CanEnable() const;
	bool IsEnabled() const;

	void MarkAsHit();
	void ResetHit();
	bool IsHit() const;
};
