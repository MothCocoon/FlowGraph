// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetParamsTypes.h"
#include "Asset/FlowAssetParams.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAssetParamsTypes)

UFlowAssetParams* FFlowAssetParamsPtr::ResolveFlowAssetParams() const
{
	return AssetPtr.LoadSynchronous();
}
