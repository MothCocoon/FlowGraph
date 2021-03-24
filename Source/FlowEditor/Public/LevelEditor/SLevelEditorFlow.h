#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;

class SLevelEditorFlow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLevelEditorFlow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void CreateFlowWidget();

	void OnFlowChanged(const FAssetData& NewAsset);
	FString GetFlowPath() const;

	FName FlowPath;
};
