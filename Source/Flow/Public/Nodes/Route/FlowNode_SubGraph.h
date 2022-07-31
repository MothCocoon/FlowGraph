// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "InstancedStruct.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_SubGraph.generated.h"

/**
 * Creates instance of provided Flow Asset and starts its execution
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Sub Graph"))
class FLOW_API UFlowNode_SubGraph : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	friend class UFlowAsset;
	friend class UFlowSubsystem;

	static FFlowPin StartPin;
	static FFlowPin FinishPin;

private:
	UPROPERTY(EditAnywhere, Category = "Graph")
	TSoftObjectPtr<UFlowAsset> Asset;

	UPROPERTY(EditAnywhere, SaveGame, meta=(ShowInnerProperties))
	FInstancedStruct Properties;

	TMap<FProperty*, FString> PropertiesToSet;
	UPROPERTY()
	TArray<FFlowPropertyPin> InputPropertyPins;
	UPROPERTY()
	TArray<FFlowPropertyPin> OutputPropertyPins;
	/*
	 * Allow to create instance of the same Flow Asset as the asset containing this node
	 * Enabling it may cause an infinite loop, if graph would keep creating copies of itself
	 */
	UPROPERTY(EditAnywhere, Category = "Graph")
	bool bCanInstanceIdenticalAsset;

	UPROPERTY(SaveGame)
	FString SavedAssetInstanceName;

protected:
	virtual bool CanBeAssetInstanced() const;

	virtual void SetProperties(TArray<FFlowInputOutputPin> Pins) override;
	virtual const TArray<FFlowPropertyPin> GetInputProperties() override;
	virtual const TArray<FFlowPropertyPin> GetOutputProperties() override;

	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

public:
	virtual void ForceFinishNode() override;

	virtual UObject* GetVariableHolder() override;

protected:
	virtual void OnLoad_Implementation() override;

public:
#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual UObject* GetAssetToEdit() override;

	virtual bool SupportsContextPins() const override { return true; }
	virtual bool SavesPropertyPins() const override { return true; }

	virtual TArray<FName> GetContextInputs() override;
	virtual TArray<FName> GetContextOutputs() override;

	// UObject
	virtual void PostLoad() override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

protected:
	virtual void GatherProperties() override;

private:
	void SubscribeToAssetChanges();
#endif
};
