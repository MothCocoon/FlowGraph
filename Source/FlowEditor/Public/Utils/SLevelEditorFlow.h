// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;

class FLOWEDITOR_API SLevelEditorFlow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLevelEditorFlow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void CreateFlowWidget();

	FString GetFlowAssetPath() const;
	void OnFlowChanged(const FAssetData& NewAsset);
	
	class UFlowComponent* FindFlowComponent() const;
	
	FString FlowAssetPath;
};
