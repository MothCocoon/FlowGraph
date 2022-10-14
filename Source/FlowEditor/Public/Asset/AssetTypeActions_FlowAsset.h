// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AssetTypeActions_Base.h"
#include "Toolkits/IToolkitHost.h"

#include "FlowEditorDefines.h"

class FLOWEDITOR_API FAssetTypeActions_FlowAsset : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override { return FColor(255, 196, 128); }

	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

	/**
	 * Documentation: https://github.com/MothCocoon/FlowGraph/wiki/Visual-Diff
	 * Set macro value to 1, if you made these changes to the engine: https://github.com/EpicGames/UnrealEngine/pull/9659
	 */
#if ENABLE_FLOW_DIFF
	virtual void PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const override;
#endif
};
