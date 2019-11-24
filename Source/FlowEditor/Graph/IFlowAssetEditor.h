#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UFlowAsset;
struct Rect;

class IFlowAssetEditor : public FAssetEditorToolkit
{
public:
	/** Returns the FlowAsset inspected by the editor */
	virtual UFlowAsset* GetFlowAsset() const = 0;

	virtual void SetSelection(TArray<UObject*> SelectedObjects) = 0;
	virtual bool CanPasteNodes() const = 0;
	virtual void PasteNodesHere(const FVector2D& Location) = 0;
	virtual bool GetBoundsForSelectedNodes(FSlateRect& Rect, float Padding) = 0;
	virtual int32 GetNumberOfSelectedNodes() const = 0;
	virtual TSet<UObject*> GetSelectedNodes() const = 0;
};
