// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FlowPrivateNode.generated.h"

// This class does not need to be modified.
UINTERFACE()
class FLOW_API UFlowPrivateNode : public UInterface
{
	GENERATED_BODY()
};

/**
 * 定制化的流程节点类接口
 * 如果FlowNode子类实现了IFlowPrivateNode接口, 说明只提供给对应的FlowAsset子类使用
 */
class FLOW_API IFlowPrivateNode
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual bool IsAllowedFlowAssetClass(TSubclassOf<class UFlowAsset> FlowAssetClass) const;
};
