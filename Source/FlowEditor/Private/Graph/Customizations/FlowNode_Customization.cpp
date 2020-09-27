#include "FlowNode_Customization.h"
#include "PropertyEditing.h"

void FFlowNode_Customization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// hide class properties while editing node instance placed in the graph
	if (DetailLayout.HasClassDefaultObject() == false)
	{
		DetailLayout.HideCategory(TEXT("FlowNode"));
	}
}
