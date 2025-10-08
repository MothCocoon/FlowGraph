// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinType.h"
#include "Types/FlowDataPinTypeNamesStandard.h"
#include "Nodes/FlowPin.h"

#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"
#if WITH_EDITOR
#include "GraphEditorSettings.h"
#endif

#include "FlowDataPinTypesStandard.generated.h"

/*
 * Flow Data Pin Type Declarations (State-less)
 *
 * NOTE:
 * - Per latest design update: Pin subcategory objects (enum assets, class filters, etc.)
 *   are NO LONGER stored on the FFlowDataPinType subclasses.
 *   Those objects now belong exclusively to the FFlowDataPinProperty subclasses (or pin instance metadata).
 * - These type classes now only describe the stable identity:
 *     * PinTypeName
 *     * PinCategory
 *     * (Optional) Color override matching editor schema expectations
 * - GetSubCategoryObject() is left as the base implementation (returns nullptr) for all types.
 *   The pin/property layer is responsible for assigning PinSubCategoryObject (e.g. enum asset, class filter, struct type).
 * - Specialized struct coloring (Vector/Rotator/Transform) relies on the pin property assigning the correct
 *   PinSubCategoryObject (TBaseStructure<...>::Get()) during pin setup
 *   (see FFlowPin::TrySetStructSubCategoryObjectFromPinType()).
 */

USTRUCT(BlueprintType)
struct FFlowDataPinType_Exec : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameExec; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Exec; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ExecutionPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Bool : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameBool; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Boolean; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->BooleanPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Int : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInt; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Int; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->IntPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Int64 : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInt64; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Int64; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->Int64PinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Float : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameFloat; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Float; }
#if WITH_EDITOR
	// Using default color; adjust if a distinct Float color is later exposed.
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Double : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	// Shares Float category (no separate Double category in schema)
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameDouble; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Float; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Name : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameName; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Name; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->NamePinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_String : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameString; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_String; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StringPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Text : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameText; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Text; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->TextPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Enum : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	// EnumClass removed per new design (belongs on property / pin instance)
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameEnum; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Enum; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Vector : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	// SubCategoryObject (FVector struct) supplied by property/pin logic now
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameVector; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Struct; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->VectorPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Rotator : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameRotator; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Struct; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->RotatorPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Transform : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameTransform; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Struct; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->TransformPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_GameplayTag : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameGameplayTag; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Struct; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_GameplayTagContainer : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameGameplayTagContainer; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Struct; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_InstancedStruct : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInstancedStruct; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Struct; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Object : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	// ClassFilter removed; belongs on property
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameObject; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Object; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ObjectPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_InstancedObject : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	// Still uses Object category
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInstancedObject; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Object; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ObjectPinTypeColor; }
#endif
};

USTRUCT(BlueprintType)
struct FFlowDataPinType_Class : public FFlowDataPinType
{
	GENERATED_BODY()

public:
	// MetaClassFilter removed; belongs on property
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameClass; }
	virtual FName GetPinCategory() const override { return FFlowPin::PC_Class; }
#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ClassPinTypeColor; }
#endif
};