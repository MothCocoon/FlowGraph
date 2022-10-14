// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

/**
 * Documentation: https://github.com/MothCocoon/FlowGraph/wiki/Asset-Search
 * Set macro value to 1, if you made these changes to the engine: https://github.com/EpicGames/UnrealEngine/pull/9070
 */
#include "FlowEditorDefines.h"
#if ENABLE_FLOW_SEARCH
#include "CoreMinimal.h"
#include "IAssetIndexer.h"

class UFlowAsset;
class FSearchSerializer;

class FLOWEDITOR_API FFlowAssetIndexer : public IAssetIndexer
{
public:
	virtual FString GetName() const override { return TEXT("FlowAsset"); }
	virtual int32 GetVersion() const override;
	virtual void IndexAsset(const UObject* InAssetObject, FSearchSerializer& Serializer) const override;

private:
	// Variant of FBlueprintIndexer::IndexGraphs
	void IndexGraph(const UFlowAsset* InFlowAsset, FSearchSerializer& Serializer) const;
};
#endif
