// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "DetailCustomizations/FlowComponentDetails.h"

#include "FlowComponent.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "GameplayTagsManager.h"
#include "Graph/FlowGraphSettings.h"

FFlowComponentDetails::~FFlowComponentDetails()
{
	UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.RemoveAll(this);
}

void FFlowComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const UFlowGraphSettings* Settings = GetDefault<UFlowGraphSettings>();
	UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.AddSP(this, &FFlowComponentDetails::ResolveCategoriesMeta);	
	
	IdentityTagsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowComponent, IdentityTags));

	const ECategoryPriority::Type Priority = Settings->bMarkFlowCategoryImportant ? ECategoryPriority::Important : ECategoryPriority::Default;
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Flow", FText::GetEmpty(), Priority);
	
	Category.AddProperty(IdentityTagsHandle);
}

void FFlowComponentDetails::ResolveCategoriesMeta(const TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const
{
	if (PropertyHandle->IsSamePropertyNode(IdentityTagsHandle))
	{
		MetaString = GetDefault<UFlowGraphSettings>()->GetIdentityTagCategories(IdentityTagsHandle->GetOuterBaseClass());
	}
}
