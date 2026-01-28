// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDataPinValueOwnerCustomization.h"

#include "Asset/FlowAssetParams.h"
#include "Nodes/FlowNodeBase.h"
#include "FlowExecutableActorComponent.h"

using FFlowAssetParamsCustomization = TFlowDataPinValueOwnerCustomization<UFlowAssetParams>;
using FFlowNodeBaseCustomization = TFlowDataPinValueOwnerCustomization<UFlowNodeBase>;
using FFlowExecutableActorComponentCustomization = TFlowDataPinValueOwnerCustomization<UFlowExecutableActorComponent>;
