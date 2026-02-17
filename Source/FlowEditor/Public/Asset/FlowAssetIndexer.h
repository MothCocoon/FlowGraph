// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "IAssetIndexer.h"
#include "UObject/Object.h"

class UFlowAsset;
class FSearchSerializer;

/**
 * Documentation: https://github.com/MothCocoon/FlowGraph/wiki/Asset-Search
 */
class FLOWEDITOR_API FFlowAssetIndexer : public IAssetIndexer
{
public:
	virtual FString GetName() const override { return TEXT("FlowAsset"); }
	virtual int32 GetVersion() const override;
	virtual void IndexAsset(const UObject* InAssetObject, FSearchSerializer& Serializer) const override;

private:
	/* Variant of FBlueprintIndexer::IndexGraphs. */
	void IndexGraph(const UFlowAsset* InFlowAsset, FSearchSerializer& Serializer) const;
};
