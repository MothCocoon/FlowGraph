// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "FlowAssetEditorContext.generated.h"

class UFlowAsset;
class FFlowAssetEditor;

UCLASS()
class FLOWEDITOR_API UFlowAssetEditorContext : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Tool Menus")
	UFlowAsset* GetFlowAsset() const;

	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
};
