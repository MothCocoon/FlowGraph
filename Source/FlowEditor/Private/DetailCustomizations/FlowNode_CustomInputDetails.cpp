// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_CustomInputDetails.h"
#include "DetailLayoutBuilder.h"
#include "FlowAsset.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomInputDetails"

FFlowNode_CustomInputDetails::FFlowNode_CustomInputDetails()
{
	bExcludeReferencedEvents = true;
}

void FFlowNode_CustomInputDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// For backward compatability, these localized texts are in FlowNode_CustomInputDetails, 
	//  not FlowNode_CustomNodeBase, so passing them in to a common function.

	static const FText CustomRowNameText = LOCTEXT("CustomRowName", "Event Name");
	static const FText EventNameText = LOCTEXT("EventName", "Event Name");

	CustomizeDetailsInternal(DetailLayout, CustomRowNameText, EventNameText);
}

IDetailCategoryBuilder& FFlowNode_CustomInputDetails::CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const
{
	return DetailLayout.EditCategory("CustomInput", LOCTEXT("CustomInputCategory", "Custom Event"));
}

TArray<FName> FFlowNode_CustomInputDetails::BuildEventNames(const UFlowAsset& FlowAsset) const
{
	return FlowAsset.GetCustomInputs();
}

#undef LOCTEXT_NAMESPACE
