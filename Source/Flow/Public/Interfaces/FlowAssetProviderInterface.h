// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"

#include "FlowAssetProviderInterface.generated.h"

class UFlowAsset;
class UFlowAssetParams;

/**
 * Interface to define a UFlowAsset provider.
 * This is used for filtering in FFlowAssetParamsPtrCustomization.
 */
UINTERFACE(MinimalAPI, Blueprintable, DisplayName = "Flow Asset Provider Interface")
class UFlowAssetProviderInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowAssetProviderInterface
{
	GENERATED_BODY()

public:
	/* Provide a FlowAsset for use in FFlowAssetParamsPtr resolution. */
	UFUNCTION(BlueprintImplementableEvent, Category = FlowAssetParams, DisplayName = "ProvideFlowAsset")
	UFlowAsset* K2_ProvideFlowAsset() const;
	virtual UFlowAsset* ProvideFlowAsset() const;

	/* Provide the resolved FlowAssetParams associated with this provider's root flow, if any. Returns nullptr by default. */
	UFUNCTION(BlueprintNativeEvent, Category = FlowAssetParams)
	UFlowAssetParams* GetRootFlowParams() const;
};
