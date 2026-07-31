// Fill out your copyright notice in the Description page of Project Settings.


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

void FFlowComponentDetails::ResolveCategoriesMeta(TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const
{
	if (PropertyHandle->IsSamePropertyNode(IdentityTagsHandle))
	{
		const UFlowGraphSettings* Settings = GetDefault<UFlowGraphSettings>();

		if (Settings->ComponentClassIdentityTagCategories.IsEmpty())
		{
			MetaString = FString::JoinBy(Settings->DefaultIdentityTagCategories, TEXT(","), [](const FGameplayTag& Tag) { return Tag.ToString(); });
		}
		else
		{
			for (const UClass* Class = IdentityTagsHandle->GetOuterBaseClass(); Class; Class = Class->GetSuperClass())
			{
				if (const FGameplayTagContainer* Tags = Settings->ComponentClassIdentityTagCategories.Find(Class))
				{
					MetaString = FString::JoinBy(*Tags, TEXT(","), [](const FGameplayTag& Tag) { return Tag.ToString(); });
				};
			}
		}
	}
}
