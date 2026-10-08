// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_Timer.h"
#include "Types/FlowPinTypesStandard.h"

#include "FlowNodeClassReplacementTestTypes.generated.h"

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestTimer : public UFlowNode_Timer
{
	GENERATED_BODY()
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestAmbiguousTimer : public UFlowNode_Timer
{
	GENERATED_BODY()

public:
	UFlowNodeClassReplacementTestAmbiguousTimer()
	{
		OutputPins.Add(FFlowPin(TEXT("In")));
	}
};

UCLASS(Hidden, NotBlueprintable, EditInlineNew, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestAddOn : public UFlowNodeAddOn
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Test)
	int32 Value = 0;
};

UCLASS(Hidden, NotBlueprintable, EditInlineNew, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestAddOnV2 : public UFlowNodeClassReplacementTestAddOn
{
	GENERATED_BODY()
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestAsset : public UFlowAsset
{
	GENERATED_BODY()

public:
	UFlowNodeClassReplacementTestAsset()
	{
		DeniedNodeClasses.Add(UFlowNodeClassReplacementTestAddOnV2::StaticClass());
	}
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowGraphValidationTestVectorNode : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowGraphValidationTestVectorNode()
	{
		OutputPins.Add(FFlowPin(TEXT("MyVec"), FFlowPinType_Vector::GetPinTypeNameStatic()));
	}
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowGraphValidationTestBoolNode : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowGraphValidationTestBoolNode()
	{
		InputPins.Add(FFlowPin(TEXT("MyBool"), FFlowPinType_Bool::GetPinTypeNameStatic()));
	}
};

USTRUCT()
struct FFlowNodeClassReplacementTestSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = Test)
	TObjectPtr<UFlowNodeClassReplacementTestAddOn> OwnedObject;
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestOwnedTimer : public UFlowNode_Timer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Test)
	FFlowNodeClassReplacementTestSettings Settings;

	void SetTestOwnedValue(int32 Value)
	{
		Settings.OwnedObject = NewObject<UFlowNodeClassReplacementTestAddOn>(this);
		Settings.OwnedObject->Value = Value;
	}

	UFlowNodeClassReplacementTestAddOn* GetTestOwnedObject() const { return Settings.OwnedObject; }
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowNodeClassReplacementTestOwnedTimerV2 : public UFlowNodeClassReplacementTestOwnedTimer
{
	GENERATED_BODY()
};
