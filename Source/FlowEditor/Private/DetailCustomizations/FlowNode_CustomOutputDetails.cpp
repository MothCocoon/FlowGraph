// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_CustomOutputDetails.h"
#include "DetailLayoutBuilder.h"
#include "FlowAsset.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomOutputDetails"

FFlowNode_CustomOutputDetails::FFlowNode_CustomOutputDetails()
{
	check(bExcludeReferencedEvents == false);
}

void FFlowNode_CustomOutputDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// For backward compatability, these localized texts are in FlowNode_CustomOutputDetails, 
	//  not FlowNode_CustomNodeBase, so passing them in to a common function.

	static const FText CustomRowNameText = LOCTEXT("CustomRowName", "Event Name");
	static const FText EventNameText = LOCTEXT("EventName", "Event Name");

	CustomizeDetailsInternal(DetailLayout, CustomRowNameText, EventNameText);
}

IDetailCategoryBuilder& FFlowNode_CustomOutputDetails::CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const
{
	return DetailLayout.EditCategory("CustomOutput", LOCTEXT("CustomEventsCategory", "Custom Output"));
}

TArray<FName> FFlowNode_CustomOutputDetails::BuildEventNames(const UFlowAsset& FlowAsset) const
{
	return FlowAsset.GetCustomOutputs();
}

#undef LOCTEXT_NAMESPACE
