// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNodeAddOn_Details.h"
#include "AddOns/FlowNodeAddOn.h"

#include "DetailLayoutBuilder.h"

void FFlowNodeAddOn_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// hide class properties while editing node addon instance placed in the graph
	if (DetailLayout.HasClassDefaultObject() == false)
	{
		DetailLayout.HideCategory(TEXT("FlowNode"));
		DetailLayout.HideCategory(TEXT("FlowNodeAddOn"));
	}

	// Call base template to set up rebuild delegate wiring
	TFlowDataPinValueOwnerCustomization<UFlowNodeAddOn>::CustomizeDetails(DetailLayout);
}
