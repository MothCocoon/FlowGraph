#include "FlowWorldSettings.h"
#include "FlowGraphComponent.h"

AFlowWorldSettings::AFlowWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlowGraph(CreateDefaultSubobject<UFlowGraphComponent>(TEXT("FlowGraph")))
{
}
