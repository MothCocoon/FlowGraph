// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_Details.h"
#include "Nodes/FlowNode.h"

#include "DetailLayoutBuilder.h"

void FFlowNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Hide class-level category when editing an instance (not the CDO)
	if (!DetailLayout.HasClassDefaultObject())
	{
		DetailLayout.HideCategory(TEXT("FlowNode"));
	}

	// Call base template to set up rebuild delegate wiring
	TFlowDataPinValueOwnerCustomization<UFlowNode>::CustomizeDetails(DetailLayout);
}
