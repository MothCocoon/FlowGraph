// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "DetailCustomizations/FlowActorDetails.h"

#include "FlowComponent.h"
#include "Graph/FlowGraphSettings.h"

#include "ActorDetailsDelegates.h"
#include "Algo/AllOf.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayTagsManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "ISinglePropertyView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SGameplayTagPicker.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowDetails"

// Duplicate for UE::GameplayTags::EditorUtilities
namespace FlowActorDetails::TagHelpers
{
	FGameplayTag GameplayTagTryImportText(const FString& Text)
	{
		FGameplayTag Tag;
		FGameplayTag::StaticStruct()->ImportText(*Text, &Tag, /*OwnerObject*/nullptr, PPF_None, nullptr, FGameplayTag::StaticStruct()->GetName(), /*bAllowNativeOverride*/true);
		return Tag;
	}

	FGameplayTagContainer GameplayTagContainerTryImportText(const FString& Text)
	{
		FGameplayTagContainer TagContainer;
		FGameplayTagContainer::StaticStruct()->ImportText(*Text, &TagContainer, /*OwnerObject*/nullptr, PPF_None, nullptr, FGameplayTagContainer::StaticStruct()->GetName(), /*bAllowNativeOverride*/true);
		return TagContainer;
	}
}

FFlowActorDetailsBuilder::FFlowActorDetailsBuilder(const FGetSelectedActors& GetSelectedActors)
	: Getter(GetSelectedActors)
{
}

FFlowActorDetailsBuilder::~FFlowActorDetailsBuilder()
{
	UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.RemoveAll(this);
}

void FFlowActorDetailsBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildBuilder)
{
	if (!UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.IsBoundToObject(this))
	{
		UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.AddSP(this, &FFlowActorDetailsBuilder::ResolveCategoriesMeta);
	}

	const TArray<UFlowComponent*> Components = FFlowActorDetails::GetSelectedFlowComponents(Getter);

	// Tag picker reads categories while its widget is constructed, before the property handle is known
	EditedComponents.Reset();
	EditedComponents.Append(Components);

	// Tag categories are configured per class, so each class gets its own row and picker filter
	TMap<UClass*, TArray<UObject*>> ComponentsByClass;
	for (UFlowComponent* Component : Components)
	{
		ComponentsByClass.FindOrAdd(Component->GetClass()).Add(Component);
	}

	for (const TPair<UClass*, TArray<UObject*>>& ClassComponents : ComponentsByClass)
	{
		const FText RowName = ComponentsByClass.Num() == 1
			                      ? LOCTEXT("RowIdentityTags", "Identity Tags")
			                      : FText::Format(LOCTEXT("RowIdentityTagsForClass", "Identity Tags: {0}"), ClassComponents.Key->GetDisplayNameText());

		if (ClassComponents.Value.Num() == 1)
		{
			AddSinglePropertyRow(ChildBuilder, ClassComponents.Value[0], RowName);
		}
		else
		{
			if (IDetailPropertyRow* Row = ChildBuilder.AddExternalObjectProperty(ClassComponents.Value, GET_MEMBER_NAME_CHECKED(UFlowComponent, IdentityTags)))
			{
				Row->DisplayName(RowName);
			}
		}
	}
}

FName FFlowActorDetailsBuilder::GetName() const
{
	static const FName Name("FActorFlowDetailsBuilder");
	return Name;
}

void FFlowActorDetailsBuilder::AddSinglePropertyRow(IDetailChildrenBuilder& ChildBuilder, UObject* Component, const FText& RowName)
{
	// Special case because AddExternalObjectProperty breaks sliders
	FPropertyEditorModule& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams Params;
	Params.NamePlacement = EPropertyNamePlacement::Hidden;
	const TSharedPtr<ISinglePropertyView> PropertyView = Module.CreateSingleProperty(Component, GET_MEMBER_NAME_CHECKED(UFlowComponent, IdentityTags), Params);

	if (PropertyView.IsValid())
	{
		FUIAction Copy, Paste;
		TSharedPtr<IPropertyHandle> ViewHandle = PropertyView->GetPropertyHandle();
		if (ViewHandle.IsValid())
		{
			ViewHandle->CreateDefaultPropertyCopyPasteActions(Copy, Paste);
			Paste = FUIAction(
				FExecuteAction::CreateStatic(&FFlowActorDetailsBuilder::PasteTags, ViewHandle),
				FCanExecuteAction::CreateStatic(&FFlowActorDetailsBuilder::CanPasteTags, ViewHandle));
		}

		ChildBuilder.AddCustomRow(RowName)
		            .CopyAction(Copy)
		            .PasteAction(Paste)
		            .NameContent()
			[
				SNew(STextBlock)
				.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
				.Text(RowName)
			]
			.ValueContent()
			[
				PropertyView.ToSharedRef()
			];
	}
}

void FFlowActorDetailsBuilder::PasteTags(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
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
		}
	}
}

bool FFlowActorDetailsBuilder::CanPasteTags(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	if (!StructPropertyHandle.IsValid())
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

void FFlowActorDetailsBuilder::ResolveCategoriesMeta(const TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const
{
	const FProperty* Property = PropertyHandle.IsValid() ? PropertyHandle->GetProperty() : nullptr;
	if (Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UFlowComponent, IdentityTags))
	{
		TArray<UObject*> OuterObjects;
		PropertyHandle->GetOuterObjects(OuterObjects);

		const bool bEditedHere = !OuterObjects.IsEmpty() && Algo::AllOf(OuterObjects, [this](const UObject* Object)
		{
			return EditedComponents.Contains(Object);
		});

		if (bEditedHere)
		{
			MetaString = GetDefault<UFlowGraphSettings>()->GetIdentityTagCategories(PropertyHandle->GetOuterBaseClass());
		}
	}
}

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
