// Fill out your copyright notice in the Description page of Project Settings.


#include "Nodes/FlowPrivateNode.h"


// Add default functionality here for any IFlowPrivateNode functions that are not pure virtual.
bool IFlowPrivateNode::IsAllowedFlowAssetClass(TSubclassOf<UFlowAsset> FlowAssetClass) const
{
	return false;
}
