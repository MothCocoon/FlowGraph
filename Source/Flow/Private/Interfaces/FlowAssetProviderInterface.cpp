// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowAssetProviderInterface.h"
#include "FlowAsset.h"

UFlowAsset* IFlowAssetProviderInterface::ProvideFlowAsset() const
{
	return Execute_K2_ProvideFlowAsset(Cast<UObject>(this));
}
