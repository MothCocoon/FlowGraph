// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphUtils.h"
#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSettings.h"

#include "FlowAsset.h"

#include "Toolkits/ToolkitManager.h"

TSharedPtr<FFlowAssetEditor> FFlowGraphUtils::GetFlowAssetEditor(const UEdGraph* Graph)
{
	check(Graph);

	TSharedPtr<FFlowAssetEditor> FlowAssetEditor;
	if (const UFlowAsset* FlowAsset = Cast<const UFlowGraph>(Graph)->GetFlowAsset())
	{
		const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(FlowAsset);
		if (FoundAssetEditor.IsValid())
		{
			FlowAssetEditor = StaticCastSharedPtr<FFlowAssetEditor>(FoundAssetEditor);
		}
	}
	return FlowAssetEditor;
}

TSharedPtr<SFlowGraphEditor> FFlowGraphUtils::GetFlowGraphEditor(const UEdGraph* Graph)
{
	TSharedPtr<SFlowGraphEditor> FlowGraphEditor;
	
	const TSharedPtr<FFlowAssetEditor> FlowEditor = GetFlowAssetEditor(Graph);
	if (FlowEditor.IsValid())
	{
		FlowGraphEditor = FlowEditor->GetFlowGraph();
	}

	return FlowGraphEditor;
}

FString FFlowGraphUtils::RemovePrefixFromNodeText(const FText& Source)
{
	FString SourceString = Source.ToString();
	TArray<FString> NodePrefixes = UFlowGraphSettings::Get()->NodePrefixesToRemove;
	
	for (FString Prefix : NodePrefixes)
	{
		Prefix = FName::NameToDisplayString(Prefix, false);
		if (SourceString.StartsWith(Prefix))
		{
			SourceString.MidInline(Prefix.Len(), MAX_int32, EAllowShrinking::No);
			SourceString = SourceString.TrimStart();
		}
	}
	
	return SourceString;
}
