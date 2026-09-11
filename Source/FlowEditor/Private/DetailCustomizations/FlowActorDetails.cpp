// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors


#include "DetailCustomizations/FlowActorDetails.h"
#include "ActorDetailsDelegates.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "FlowComponent.h"
#include "IDetailChildrenBuilder.h"
#include "ISinglePropertyView.h"
#include "SGameplayTagContainerCombo.h"
#include "SGameplayTagPicker.h"
#include "Graph/FlowGraphSettings.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "FlowDetails"

// Duplicate for UE::GameplayTags::EditorUtilities
namespace FlowActorDetails::TagHelpers
{	
	FString GameplayTagExportText(const FGameplayTag Tag)
	{
		FString ExportString;
		FGameplayTag::StaticStruct()->ExportText(ExportString, &Tag, &Tag, /*OwnerObject*/nullptr, /*PortFlags*/0, /*ExportRootScope*/nullptr);
		return ExportString;
	}

	FGameplayTag GameplayTagTryImportText(const FString& Text)
	{
		FGameplayTag Tag;
		FGameplayTag::StaticStruct()->ImportText(*Text, &Tag, /*OwnerObject*/nullptr, PPF_None, nullptr, FGameplayTag::StaticStruct()->GetName(), /*bAllowNativeOverride*/true);
		return Tag;
	}

	FString GameplayTagContainerExportText(const FGameplayTagContainer& TagContainer)
	{
		FString ExportString;
		FGameplayTagContainer::StaticStruct()->ExportText(ExportString, &TagContainer, &TagContainer, /*OwnerObject*/nullptr, /*PortFlags*/0, /*ExportRootScope*/nullptr);
		return ExportString;
	}

	FGameplayTagContainer GameplayTagContainerTryImportText(const FString& Text)
	{
		FGameplayTagContainer TagContainer;
		FGameplayTagContainer::StaticStruct()->ImportText(*Text, &TagContainer, /*OwnerObject*/nullptr, PPF_None, nullptr, FGameplayTagContainer::StaticStruct()->GetName(), /*bAllowNativeOverride*/true);
		return TagContainer;
	}

}


class FFlowActorDetailsBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FFlowActorDetailsBuilder>
{
public:
	FFlowActorDetailsBuilder(const FGetSelectedActors& GetSelectedActors)
		: Getter(GetSelectedActors)
	{
		
	}

	//~ Begin IDetailCustomNodeBuilder interface
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override { }
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildBuilder) override
	{		
		TArray<UFlowComponent*> Components = FFlowActorDetails::GetSelectedFlowComponents(Getter);
		if (Components.Num() == 1)
		{
			// Special case because AddExternalObjectProperty breaks sliders
			const FText IdentityRow = LOCTEXT("RowIdentityTags", "Identity Tags");
				
			FPropertyEditorModule& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			FSinglePropertyParams Params;
			Params.NamePlacement = EPropertyNamePlacement::Hidden;
			PropertyView = Module.CreateSingleProperty(Components[0], GET_MEMBER_NAME_CHECKED(UFlowComponent, IdentityTags), Params);
			
			if (PropertyView.IsValid())
			{
				FUIAction Copy, Paste;
				TSharedPtr<IPropertyHandle> ViewHandle = PropertyView->GetPropertyHandle();
				if (ViewHandle.IsValid())
				{
					ViewHandle->CreateDefaultPropertyCopyPasteActions(Copy, Paste);						
					Paste = FUIAction(
							FExecuteAction::CreateSP(this, &FFlowActorDetailsBuilder::PasteTags), 
							FCanExecuteAction::CreateSP(this, &FFlowActorDetailsBuilder::CanPasteTags) );
				}
				
				ChildBuilder.AddCustomRow(IdentityRow)
					.CopyAction(Copy)
					.PasteAction(Paste)				
					.NameContent()
				[
					SNew(STextBlock).Font(IPropertyTypeCustomizationUtils::GetRegularFont())
					.Text(IdentityRow)
				]
				.ValueContent()
				[
					PropertyView.ToSharedRef()
				];
			}	
		}			
		else
		{
			TArray<UObject*> Objects;
			Objects.Append(Components);
			ChildBuilder.AddExternalObjectProperty(Objects, GET_MEMBER_NAME_CHECKED(UFlowComponent, IdentityTags));
		}		
	}
	
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual FName GetName() const override
	{
		static const FName Name("FActorFlowDetailsBuilder");
		return Name;
	}
	//~ End IDetailCustomNodeBuilder interface	
	
	
	void PasteTags()
	{
		if (!PropertyView.IsValid())
		{
			return;
		}
		
		TSharedPtr<IPropertyHandle> StructPropertyHandle = PropertyView->GetPropertyHandle();
		if (!StructPropertyHandle.IsValid())
		{
			return;
		}
		

		FString PastedText;
		FPlatformApplicationMisc::ClipboardPaste(PastedText);
		bool bHandled = false;

		// Try to paste single tag
		const FGameplayTag PastedTag = FlowActorDetails::TagHelpers::GameplayTagTryImportText(PastedText);
		if (PastedTag.IsValid())
		{
			TArray<FString> NewValues;
			SGameplayTagPicker::EnumerateEditableTagContainersFromPropertyHandle(StructPropertyHandle.ToSharedRef(), [&NewValues, PastedTag](const FGameplayTagContainer& EditableTagContainer)
			{
				FGameplayTagContainer TagContainerCopy = EditableTagContainer;
				TagContainerCopy.AddTag(PastedTag);

				NewValues.Add(TagContainerCopy.ToString());
				return true;
			});

			FScopedTransaction Transaction(LOCTEXT("GameplayTagContainerCustomization_PasteTag", "Paste Gameplay Tag"));
			StructPropertyHandle->SetPerObjectValues(NewValues);
			bHandled = true;
		}

		// Try to paste a container
		if (!bHandled)
		{
			const FGameplayTagContainer PastedTagContainer = FlowActorDetails::TagHelpers::GameplayTagContainerTryImportText(PastedText);
			if (PastedTagContainer.IsValid())
			{
				// From property
				FScopedTransaction Transaction(LOCTEXT("GameplayTagContainerCustomization_PasteTagContainer", "Paste Gameplay Tag Container"));
				StructPropertyHandle->SetValueFromFormattedString(PastedText);
				bHandled = true;
			}
		}
	}

	bool CanPasteTags()
	{
		if (!PropertyView.IsValid() || !PropertyView->GetPropertyHandle().IsValid())
		{
			return false;
		}

		FString PastedText;
		FPlatformApplicationMisc::ClipboardPaste(PastedText);
		
		const FGameplayTag PastedTag = FlowActorDetails::TagHelpers::GameplayTagTryImportText(PastedText);
		if (PastedTag.IsValid())
		{
			return true;
		}

		const FGameplayTagContainer PastedTagContainer = FlowActorDetails::TagHelpers::GameplayTagContainerTryImportText(PastedText);
		if (PastedTagContainer.IsValid())
		{
			return true;
		}

		return false;
	}
	
private:
	FGetSelectedActors Getter;	
	TSharedPtr<ISinglePropertyView> PropertyView;
};


FFlowActorDetails::~FFlowActorDetails()
{
	OnExtendActorDetails.RemoveAll(this);
}

void FFlowActorDetails::Register()
{
	OnExtendActorDetails.AddSP(this, &FFlowActorDetails::AddFlowCategory);	
}

void FFlowActorDetails::AddFlowCategory(class IDetailLayoutBuilder& Details, const FGetSelectedActors& GetSelectedActors)
{	
	const bool bEnabled = GetDefault<UFlowGraphSettings>()->bShowFlowTagsInActorDetails;

	if (!bEnabled)
	{
		return;
	}
	
	if (GetSelectedActors.IsBound())
	{
		const TArray<UFlowComponent*> Components = FFlowActorDetails::GetSelectedFlowComponents(GetSelectedActors);
		if (!Components.IsEmpty())
		{
			const ECategoryPriority::Type Priority = GetDefault<UFlowGraphSettings>()->bMarkFlowCategoryImportant ? ECategoryPriority::Important : ECategoryPriority::Default;			

			const FText CategoryName = FText::Format(LOCTEXT("FlowCategoryFormat", "Flow Components: {0} "), FText::AsNumber(Components.Num()));
			const FString Tooltip = FString::JoinBy(Components, LINE_TERMINATOR, [](const UObject* Object)
			{				
				const UActorComponent* Component = CastChecked<UActorComponent>(Object);
				const FString Actor = Component->GetOwner() ? Component->GetOwner()->GetActorNameOrLabel() : TEXT("None");
				return FString(Actor + TEXT(".") + Object->GetName());				
			});
			
			IDetailCategoryBuilder& Category = Details.EditCategory(TEXT("_FlowDetails"), CategoryName, Priority);
			Category.SetToolTip(FText::FromString(Tooltip));
			Category.AddCustomBuilder(MakeShared<FFlowActorDetailsBuilder>(GetSelectedActors));					
		}
	}
}

TArray<class UFlowComponent*> FFlowActorDetails::GetSelectedFlowComponents(const FGetSelectedActors& GetSelectedActors)
{
	TArray<UFlowComponent*> Components;

	if (GetSelectedActors.IsBound())
	{
		const TArray<TWeakObjectPtr<AActor>>& SelectedActors = GetSelectedActors.Execute();
		for (auto& WeakActor : SelectedActors)
		{
			const AActor* Actor = WeakActor.Get();
			if (Actor && !Actor->IsA(AWorldSettings::StaticClass()))
			{
				TInlineComponentArray<UFlowComponent*> FlowComps(Actor);		
				Components.Append(FlowComps);
			}
		}
	}		
	return Components;
}


#undef LOCTEXT_NAMESPACE
