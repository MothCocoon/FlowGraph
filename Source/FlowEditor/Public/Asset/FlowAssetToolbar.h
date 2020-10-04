#pragma once

#include "CoreMinimal.h"

class FExtender;
class FToolBarBuilder;
class SWidget;

class FFlowAssetEditor;

//////////////////////////////////////////////////////////////////////////
// Flow Asset Toolbar

class FFlowAssetToolbar final : public TSharedFromThis<FFlowAssetToolbar>
{
public:
	FFlowAssetToolbar(TSharedPtr<FFlowAssetEditor> InNodeEditor);

	void AddToolbar(FToolBarBuilder& ToolbarBuilder);

private:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
};
