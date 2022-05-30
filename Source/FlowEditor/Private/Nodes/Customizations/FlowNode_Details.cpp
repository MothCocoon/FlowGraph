// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowNode_Details.h"
#include "PropertyEditing.h"

void FFlowNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// hide class properties while editing node instance placed in the graph
	if (DetailLayout.HasClassDefaultObject() == false)
	{
		DetailLayout.HideCategory(TEXT("FlowNode"));
	}
}
