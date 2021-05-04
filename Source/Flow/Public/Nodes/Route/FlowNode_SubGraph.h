#pragma once

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

	UPROPERTY(SaveGame)
	FString SavedAssetInstanceName;

protected:
	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

public:
	virtual void ForceFinishNode() override;

protected:
	virtual void OnGameSaveLoaded_Implementation() override;

public:
#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual UObject* GetAssetToEdit() override;

	virtual bool SupportsContextPins() const override { return true; }

	virtual TArray<FName> GetContextInputs() override;
	virtual TArray<FName> GetContextOutputs() override;

	// UObject
	virtual void PostLoad() override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

private:
	void SubscribeToAssetChanges();
#endif
};
