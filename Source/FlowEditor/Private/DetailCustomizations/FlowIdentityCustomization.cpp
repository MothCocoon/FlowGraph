// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "DetailCustomizations/FlowIdentityCustomization.h"

#include "Types/FlowIdentity.h"
#include "FlowComponent.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSettings.h"

#include "GameplayTagsManager.h"

#include "ActorPickerMode.h"
#include "ActorTreeItem.h"
#include "LevelEditor.h"
#include "SceneOutlinerModule.h"
#include "Subsystems/EditorActorSubsystem.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "SGameplayTagChip.h"
#include "SLevelViewport.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SSpinBox.h"

#define LOCTEXT_NAMESPACE "FlowIdentityCustomization"

//////////////////////////////////////////////////////////////////////////
// SFlowIdentityTagSelector

void SFlowIdentityTagSelector::Construct(const FArguments& InArgs, const TSharedPtr<IPropertyHandle>& InIdentityTagsHandle)
{
	WeakTagsHandle = InIdentityTagsHandle;

	FMenuBuilder MenuBuilder(true, nullptr);

	const TSharedRef<SWidget> HeaderContent =
		SNew(SBox)
		.Padding(15, 2)
		[
			SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "Menu.Label")
			.Text(this, &SFlowIdentityTagSelector::GetSourceName)
			.ToolTipText(this, &SFlowIdentityTagSelector::GetSourceTooltip)
		];
	MenuBuilder.AddWidget(HeaderContent, FText::GetEmpty(), true);


	MenuBuilder.BeginSection(NAME_None, LOCTEXT("ApplyTag", "Apply Tags"));
	{
		FMenuBarBuilder ApplyActions(nullptr);

		ApplyActions.AddMenuEntry(LOCTEXT("SetAll", "Set All"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SFlowIdentityTagSelector::ApplyAll)));
		ApplyActions.AddMenuEntry(LOCTEXT("AppendAll", "Append All"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SFlowIdentityTagSelector::AppendAll)));
		ApplyActions.AddMenuEntry(LOCTEXT("Clear", "Clear"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SFlowIdentityTagSelector::ClearTags)));

		const TSharedRef<SWidget> ApplyActionsWidget =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				ApplyActions.MakeWidget()
			]
			+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.HasDownArrow(false)
				.ContentPadding(4.0f)
				.ToolTipText(LOCTEXT("Settings", "Settings"))
				.ButtonContent()
				[
					SNew(SImage)
					.DesiredSizeOverride(FVector2D(16))
					.Image(FAppStyle::Get().GetBrush("Icons.Toolbar.Settings"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				.OnGetMenuContent_Static(&SFlowIdentityTagSelector::CreateSettingsMenu)
			];

		MenuBuilder.AddWidget(ApplyActionsWidget, FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("SelectTag", "Select Tags"));
	{
		FMenuBarBuilder HorizSection(nullptr);
		const int32 TagWindowSize = GetDefault<UFlowGraphEditorSettings>()->FlowIdentity_MinWindowWidth / 2;

		const TSharedRef<SWidget> OptionsContent =
			SNew(SBox)
			.MinDesiredHeight(100)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(5, 2, 5, 5)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "Menu.Label")
						.Text(LOCTEXT("SelectedComponentTags", "Component Tags"))
						.MinDesiredWidth(TagWindowSize)
					]
					+ SVerticalBox::Slot()
					[
						SNew(SBorder)
						.Padding(FMargin(6, 5))
						.BorderImage(FSlateStyleRegistry::FindSlateStyle("GameplayTagStyle")->GetBrush("GameplayTags.Container"))
						[
							SAssignNew(TagOptions, SVerticalBox)
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(5, 2, 5, 5)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "Menu.Label")
						.Text(LOCTEXT("IdentityTags", "Identity Tags"))
						.MinDesiredWidth(TagWindowSize)
					]
					+ SVerticalBox::Slot()
					[
						SNew(SBorder)
						.Padding(FMargin(6, 5))
						.BorderImage(FSlateStyleRegistry::FindSlateStyle("GameplayTagStyle")->GetBrush("GameplayTags.Container"))
						[
							SAssignNew(CurrentTagsPanel, SVerticalBox)
						]
					]
				]
			];

		RebuildTagOptions();
		MenuBuilder.AddWidget(OptionsContent, FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	ChildSlot
	[
		MenuBuilder.MakeWidget()
	];
}

TSharedRef<SWidget> SFlowIdentityTagSelector::CreateSettingsMenu()
{
	FMenuBuilder MenuBuilder(false, nullptr);

	const FUIAction UIAction_CloseOnAction(
		FExecuteAction::CreateLambda([]()
		{
			UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
			Settings->bFlowIdentity_CloseOnAction = !Settings->bFlowIdentity_CloseOnAction;
			Settings->SaveConfig();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			return GetDefault<UFlowGraphEditorSettings>()->bFlowIdentity_CloseOnAction;
		})
	);
	MenuBuilder.AddMenuEntry(LOCTEXT("Settings_CloseOnAction", "Close On Action"), FText::GetEmpty(), FSlateIcon(), UIAction_CloseOnAction, NAME_None, EUserInterfaceActionType::ToggleButton);

	const FUIAction UIAction_ToggleShortNames(
		FExecuteAction::CreateLambda([]()
		{
			UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
			Settings->bFlowIdentity_DisplayShortNames = !Settings->bFlowIdentity_DisplayShortNames;
			Settings->SaveConfig();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			return GetDefault<UFlowGraphEditorSettings>()->bFlowIdentity_DisplayShortNames;
		})
	);
	MenuBuilder.AddMenuEntry(LOCTEXT("Settings_ShortNames", "Short Names"), FText::GetEmpty(), FSlateIcon(), UIAction_ToggleShortNames, NAME_None, EUserInterfaceActionType::ToggleButton);

	const TSharedRef<SWidget> SizeEditor =
		SNew(SSpinBox<int32>)
		.MinValue(100)
		.EnableSlider(false)
		.Value_Lambda([]() { return GetDefault<UFlowGraphEditorSettings>()->FlowIdentity_MinWindowWidth; })
		.OnValueCommitted_Lambda([](const int32 Value, const ETextCommit::Type CommitInfo)
		{
			if (CommitInfo == ETextCommit::OnEnter)
			{
				UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
				Settings->FlowIdentity_MinWindowWidth = Value;
				Settings->SaveConfig();
			}
		})
		.OnValueChanged_Lambda([](const int32 InValue)
		{
			UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
			Settings->FlowIdentity_MinWindowWidth = InValue;
		});

	MenuBuilder.AddWidget(SizeEditor, LOCTEXT("Settings_Size", "Size"));
	return MenuBuilder.MakeWidget();
}

void SFlowIdentityTagSelector::SetSourceFromComponent(UFlowComponent* Component)
{
	SelectionSource.Reset();
	TagOptions->ClearChildren();

	SelectionSource = Component;

	RebuildTagOptions();
}

TArray<FGameplayTagContainer> SFlowIdentityTagSelector::GetCurrentTags() const
{
	TArray<FGameplayTagContainer> Tags;

	const TSharedPtr<IPropertyHandle> TagsHandle = WeakTagsHandle.Pin();
	if (TagsHandle.IsValid() && TagsHandle->IsValidHandle())
	{
		TArray<const void*> RawData;
		TagsHandle->AccessRawData(RawData);
		for (const void* RawPtr : RawData)
		{
			if (RawPtr)
			{
				Tags.Add(*static_cast<const FGameplayTagContainer*>(RawPtr));
			}
		}
	}

	return Tags;
}

FText SFlowIdentityTagSelector::GetSourceName() const
{
	FString ComponentLabel = TEXT("None");
	FString ActorLabel = TEXT("None");

	if (const UFlowComponent* FlowComponent = SelectionSource.Get())
	{
		ComponentLabel = FString::Printf(TEXT("%s [%s]"), *FlowComponent->GetName(), *GetNameSafe(FlowComponent->GetClass()));

		if (const AActor* Owner = FlowComponent->GetOwner())
		{
			ActorLabel = FString::Printf(TEXT("%s [%s]"), *Owner->GetActorLabel(), *GetNameSafe(Owner->GetClass()));
		}
	}

	return FText::Format(LOCTEXT("TagSourceLabel", "Actor: {0}\nComponent: {1}"),
	                     FText::FromString(ActorLabel),
	                     FText::FromString(ComponentLabel)
	);
}

FText SFlowIdentityTagSelector::GetSourceTooltip() const
{
	FString CompLabel = TEXT("None");
	FString ActorLabel = TEXT("None");

	const UFlowComponent* FlowComponent = SelectionSource.Get();
	if (FlowComponent)
	{
		CompLabel = FString::Printf(TEXT("%s [%s]"), *FlowComponent->GetName(), *GetNameSafe(FlowComponent->GetClass()));

		if (const AActor* Owner = FlowComponent->GetOwner())
		{
			ActorLabel = FString::Printf(TEXT("%s [%s]"), *Owner->GetActorLabel(), *GetNameSafe(Owner->GetClass()));
		}
	}

	return FText::Format(LOCTEXT("TagSourceTooltip", "Actor: {0}\nComponent: {1}\nPath: {2}"),
	                     FText::FromString(ActorLabel),
	                     FText::FromString(CompLabel),
	                     FText::FromString(GetPathNameSafe(FlowComponent))
	);
}

void SFlowIdentityTagSelector::RebuildTagOptions()
{
	const TSharedPtr<IPropertyHandle> TagsHandle = WeakTagsHandle.Pin();
	if (!TagsHandle.IsValid() || !TagsHandle->IsValidHandle())
	{
		return;
	}

	TagOptions->ClearChildren();
	CurrentTagsPanel->ClearChildren();

	auto CreateMessage = [](const FText& Message)
	{
		return SNew(STextBlock)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			.Text(Message);
	};

	TArray<FString> Prefixes;
	if (GetDefault<UFlowGraphEditorSettings>()->bFlowIdentity_DisplayShortNames)
	{
		const TArray<FGameplayTag>& Categories = GetDefault<UFlowGraphSettings>()->DefaultIdentityTagCategories;
		if (Categories.Num() == 1)
		{
			Prefixes.Add(Categories[0].ToString() + TEXT("."));
		}
		else
		{
			for (auto& Category : Categories)
			{
				FGameplayTag Parent = Category.RequestDirectParent();
				if (Parent.IsValid())
				{
					Prefixes.Add(Parent.ToString() + TEXT("."));
				}
			}
		}
	}

	auto GetTagString = [&Prefixes](const FGameplayTag& GameplayTag)
	{
		FString String = GameplayTag.ToString();
		if (GameplayTag.IsValid())
		{
			for (auto& Prefix : Prefixes)
			{
				if (String.RemoveFromStart(Prefix))
				{
					break;
				}
			}
		}

		return String;
	};

	// Build component tags
	if (const UFlowComponent* Component = SelectionSource.Get())
	{
		TArray<FGameplayTag> AllTags = Component->IdentityTags.GetGameplayTagArray();
		AllTags.StableSort([](const FGameplayTag& A, const FGameplayTag& B) { return A < B; });

		for (const FGameplayTag& GameplayTag : AllTags)
		{
			TagOptions->AddSlot()
			          .AutoHeight()
			          .Padding(FMargin(0, 2))
			[
				SNew(SGameplayTagChip)
				.ShowClearButton(false)
				.Text(FText::FromString(GetTagString(GameplayTag) + TEXT("  ")))
				.IsSelected(this, &SFlowIdentityTagSelector::IsTagSelected, GameplayTag)
				.OnEditPressed_Lambda([this, GameplayTag]()
				{
					if (GetTagState(GameplayTag) == ECheckBoxState::Checked)
					{
						SetTagSelected(GameplayTag, false);
					}
					else
					{
						SetTagSelected(GameplayTag, true);
					}

					return FReply::Handled();
				})
			];
		}

		if (AllTags.IsEmpty())
		{
			TagOptions->AddSlot()
			          .AutoHeight()
			          .Padding(FMargin(0, 2))
			[
				CreateMessage(LOCTEXT("NoIdentityTags", "No Tags"))
			];
		}
	}
	else
	{
		TagOptions->AddSlot()
		          .AutoHeight()
		          .Padding(FMargin(0, 2))
		[
			CreateMessage(LOCTEXT("NoComponentSelected", "No Component"))
		];
	}

	// Build current tags
	{
		TMap<FGameplayTag, int32> AllTags;
		TArray<FGameplayTagContainer> CurrentTags = GetCurrentTags();
		for (auto& Tags : CurrentTags)
		{
			for (auto& GameplayTag : Tags)
			{
				AllTags.FindOrAdd(GameplayTag) += 1;
			}
		}
		AllTags.KeyStableSort([](const FGameplayTag& A, const FGameplayTag& B) { return A < B; });

		for (auto& [GameplayTag, Count] : AllTags)
		{
			CurrentTagsPanel->AddSlot()
			                .AutoHeight()
			                .Padding(FMargin(0, 2))
			[
				SNew(SGameplayTagChip)
				.IsSelected(Count == CurrentTags.Num())
				.OnClearPressed_Lambda([this, GameplayTag]()
				{
					SetTagSelected(GameplayTag, false);
					return FReply::Handled();
				})
				.Text(FText::FromString(GetTagString(GameplayTag)))
			];
		}

		if (AllTags.IsEmpty())
		{
			CurrentTagsPanel->AddSlot()
			                .AutoHeight()
			                .Padding(FMargin(0, 2))
			[
				CreateMessage(LOCTEXT("NoTags", "Empty"))
			];
		}
	}
}

ECheckBoxState SFlowIdentityTagSelector::GetTagState(const FGameplayTag GameplayTag) const
{
	TArray<FGameplayTagContainer> CurrentTags = GetCurrentTags();
	if (!CurrentTags.IsEmpty())
	{
		int32 NumMatches = 0;
		for (auto& Tags : CurrentTags)
		{
			if (Tags.HasTagExact(GameplayTag))
			{
				NumMatches++;
			}
		}

		if (NumMatches == 0)
		{
			return ECheckBoxState::Unchecked;
		}
		else if (NumMatches == CurrentTags.Num())
		{
			return ECheckBoxState::Checked;
		}
	}

	return ECheckBoxState::Undetermined;
}

void SFlowIdentityTagSelector::SetTagSelected(const FGameplayTag GameplayTag, const bool bNewSelected)
{
	const TSharedPtr<IPropertyHandle> TagsHandle = WeakTagsHandle.Pin();
	if (TagsHandle.IsValid() && TagsHandle->IsValidHandle())
	{
		TArray<FString> Values;

		TArray<FGameplayTagContainer> CurrentTags = GetCurrentTags();
		for (auto& Tags : CurrentTags)
		{
			if (bNewSelected)
			{
				Tags.AddTag(GameplayTag);
			}
			else
			{
				Tags.RemoveTag(GameplayTag);
			}

			const FString Value = TagsToPropertyString(Tags);
			Values.Add(Value);
		}

		TagsHandle->SetPerObjectValues(Values);

		RebuildTagOptions();
	}
}

void SFlowIdentityTagSelector::ApplyAll()
{
	const TSharedPtr<IPropertyHandle> TagsHandle = WeakTagsHandle.Pin();
	if (TagsHandle.IsValid() && TagsHandle->IsValidHandle())
	{
		FGameplayTagContainer Tags;
		if (const UFlowComponent* Component = SelectionSource.Get())
		{
			Tags = Component->IdentityTags;
		}

		const FString Value = TagsToPropertyString(Tags);
		TagsHandle->SetValueFromFormattedString(Value);
	}

	ActionExecuted();
}

void SFlowIdentityTagSelector::AppendAll()
{
	const TSharedPtr<IPropertyHandle> TagsHandle = WeakTagsHandle.Pin();
	if (TagsHandle.IsValid() && TagsHandle->IsValidHandle())
	{
		if (const UFlowComponent* Component = SelectionSource.Get())
		{
			TArray<FGameplayTagContainer> CurrentTags = GetCurrentTags();

			TArray<FString> Values;
			for (auto& Tags : CurrentTags)
			{
				Tags.AppendTags(Component->IdentityTags);

				const FString Value = TagsToPropertyString(Tags);
				Values.Add(Value);
			}

			TagsHandle->SetPerObjectValues(Values);
		}
	}

	ActionExecuted();
}

void SFlowIdentityTagSelector::ClearTags()
{
	const TSharedPtr<IPropertyHandle> TagsHandle = WeakTagsHandle.Pin();
	if (TagsHandle.IsValid() && TagsHandle->IsValidHandle())
	{
		const FString Value = TagsToPropertyString(FGameplayTagContainer::EmptyContainer);
		TagsHandle->SetValueFromFormattedString(Value);
	}

	ActionExecuted();
}

void SFlowIdentityTagSelector::ActionExecuted()
{
	if (GetDefault<UFlowGraphEditorSettings>()->bFlowIdentity_CloseOnAction)
	{
		FSlateApplication::Get().DismissAllMenus();
	}
	else
	{
		RebuildTagOptions();
	}

	OnAction.ExecuteIfBound();
}

//////////////////////////////////////////////////////////////////////////
// FFlowIdentityCustomization

void FFlowIdentityCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;
	StructPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowIdentityCustomization::UpdateCachedStructs));
	StructPropertyHandle->SetOnPropertyResetToDefault(FSimpleDelegate::CreateSP(this, &FFlowIdentityCustomization::UpdateCachedStructs));
	StructPropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowIdentityCustomization::UpdateCachedStructs));
	
	IdentityTagsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowIdentity, IdentityTags));
	
	UpdateCachedStructs();

	if (StructPropertyHandle->HasMetaData(TEXT("ShowOnlyInnerProperties")))
	{
		return;
	}

	HeaderWidget = CreateHeaderWidget();

	bool bAutoExpand = true;
	{
		const FString& Meta = StructPropertyHandle->GetMetaData(TEXT("AutoExpand"));
		if (!Meta.IsEmpty())
		{
			LexFromString(bAutoExpand, Meta);
		}
	}

	FUIAction Action;
	Action.ExecuteAction.BindSP(this, &FFlowIdentityCustomization::FindMatchingPopup);
	Action.CanExecuteAction.BindStatic(&FFlowIdentityCustomization::CanOpenMatchingPopup);

	HeaderRow
		.ShouldAutoExpand(bAutoExpand)
		.AddCustomContextMenuAction(Action, LOCTEXT("FindMatchingPopup", "Find matching actors"), LOCTEXT("FindMatchingPopup_Tooltip", "Works in PIE"))
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			HeaderWidget.ToSharedRef()
		];
}

void FFlowIdentityCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Insert header row above children
	if (StructPropertyHandle->HasMetaData(TEXT("ShowOnlyInnerProperties")))
	{
		HeaderWidget = CreateHeaderWidget();

		ChildBuilder
			.AddCustomRow(LOCTEXT("HeaderRow", "Header Row"))
			.ValueContent()
			[
				HeaderWidget.ToSharedRef()
			];
	}

	OwningView = ChildBuilder.GetParentCategory().GetParentLayout().GetDetailsViewSharedPtr();

	UGameplayTagsManager::Get().OnGetCategoriesMetaFromPropertyHandle.AddSP(this, &FFlowIdentityCustomization::ResolveCategoriesMeta);

	ChildBuilder.AddProperty(IdentityTagsHandle.ToSharedRef());
	ChildBuilder.AddProperty(PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowIdentity, IdentityMatchType)).ToSharedRef());
}

void FFlowIdentityCustomization::UpdateCachedStructs()
{
	TArray<void*> StructPtrs;
	StructPropertyHandle->AccessRawData(StructPtrs);

	const FStructProperty* Property = CastFieldChecked<FStructProperty>(StructPropertyHandle->GetProperty());
	check(Property->Struct);
		
	CachedIdentities.Reset(StructPtrs.Num());
	for (void* RawPtr : StructPtrs)
	{
		if (RawPtr)
		{
			TInstancedStruct<FFlowIdentity>& Struct = CachedIdentities.AddDefaulted_GetRef();
			Struct.InitializeAsScriptStruct(Property->Struct, static_cast<uint8*>(RawPtr));
		}
	}
}

bool FFlowIdentityCustomization::IsActorAllowed(const AActor* Actor) const
{
	const UFlowComponent* FlowComponent = Actor->FindComponentByClass<UFlowComponent>();
	if (FlowComponent && FlowComponent->IdentityTags.IsValid())
	{
		return true;
	}
	return false;
}

bool FFlowIdentityCustomization::IsActorMatches(const AActor* Actor) const
{
	const UFlowComponent* FlowComponent = Actor->FindComponentByClass<UFlowComponent>();
	if (FlowComponent && FlowComponent->IdentityTags.IsValid())
	{
		for (const auto& Struct : CachedIdentities)
		{
			const FFlowIdentity* Identity = Struct.GetPtr();
			if (Identity && FlowTypes::HasMatchingTags(FlowComponent->IdentityTags, Identity->IdentityTags, Identity->IdentityMatchType))
			{
				return true;
			}
		}
	}
	return false;
}

void FFlowIdentityCustomization::ResolveCategoriesMeta(TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const
{
	if (PropertyHandle->IsSamePropertyNode(IdentityTagsHandle))
	{
		const FString Categories = StructPropertyHandle->GetMetaData("Categories");
		if (Categories.IsEmpty())
		{
			const TArray<FGameplayTag>& GameplayTags = GetDefault<UFlowGraphSettings>()->DefaultIdentityTagCategories;
			MetaString = FString::JoinBy(GameplayTags, TEXT(","), [](const FGameplayTag& Tag) { return Tag.ToString(); });
		}
		else
		{
			MetaString = Categories;
		}
	}
}

TSharedRef<SWidget> FFlowIdentityCustomization::CreateHeaderWidget()
{
	const FMargin IconPadding(2, 0);

	return SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(IconPadding)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.ContentPadding(4.0f)
				.ToolTipText(LOCTEXT("UseSelectedActorIdentity", "Apply selected actor Identity"))
				.Content()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Previous"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				.OnClicked_Lambda([this]()
				{
					this->OpenTagPicker_UseCurrentSelection();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(IconPadding)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.HasDownArrow(false)
				.ContentPadding(4.0f)
				.ToolTipText(LOCTEXT("SelectActorIdentity", "Pick actor identity from Level"))
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.SelectInViewport"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				.OnGetMenuContent(this, &FFlowIdentityCustomization::MenuContent_ActorPicker)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(IconPadding)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.OnClicked_Lambda([this]()
				{
					this->UseActor_FromEyeDrop();
					return FReply::Handled();
				})
				.ContentPadding(4.0f)
				.ForegroundColor(FSlateColor::UseForeground())
				.IsFocusable(false)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.EyeDropper"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(IconPadding)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.HasDownArrow(false)
				.ContentPadding(4.0f)
				.ToolTipText(LOCTEXT("FindMatchingActors", "Find matching actors"))
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Symbols.SearchGlass"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				.OnGetMenuContent(this, &FFlowIdentityCustomization::MenuContent_FindMatching)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(PopUpAnchor, SSpacer).Size(FVector2D::One())
			]
		];
}

TSharedRef<SWidget> FFlowIdentityCustomization::CreateTagPicker()
{
	TagSelector = SNew(SFlowIdentityTagSelector, IdentityTagsHandle);
	return TagSelector.ToSharedRef();
}

void FFlowIdentityCustomization::OpenTagPicker_UseCurrentSelection()
{
	const TSharedRef<SWidget> Widget = CreateTagPicker();
	UseActor_SelectedInViewport();

	FSlateApplication::Get().PushMenu(
		HeaderWidget.ToSharedRef(),
		FWidgetPath(),
		Widget,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

void FFlowIdentityCustomization::OpenTagPicker_UseActor(AActor* Actor)
{
	const TSharedRef<SWidget> Widget = CreateTagPicker();
	if (TagSelector)
	{
		TagSelector->SetSourceFromActor(Actor);
		TagSelector->OnAction = FSimpleDelegate::CreateSPLambda(this, [this]()
		{
			if (OwningView.IsValid())
			{
				if (const TSharedPtr<FTabManager> TabManager = OwningView->GetHostTabManager())
				{
					if (const TSharedPtr<SDockTab> DockTab = TabManager->GetOwnerTab())
					{
						DockTab->DrawAttention();
					}
				}
			}
		});
	}

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedPtr<SLevelViewport> LevelEditor = LevelEditorModule.GetFirstActiveLevelViewport();

	FSlateApplication::Get().PushMenu(
		LevelEditor.ToSharedRef(),
		FWidgetPath(),
		Widget,
		FSlateApplication::Get().GetCursorPos() + FVector2D(0, 50),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

void FFlowIdentityCustomization::FocusActor(AActor* Actor)
{
	if (Actor)
	{
		if (GIntraFrameDebuggingGameThread)
		{
			return;
		}

		if (GEditor->PlayWorld && !GEditor->bIsSimulatingInEditor)
		{
			FEditorDelegates::OnSwitchBeginPIEAndSIE.RemoveAll(this);
			FEditorDelegates::OnSwitchBeginPIEAndSIE.AddSPLambda(this, [this, WeakActor = MakeWeakObjectPtr(Actor)](const bool bIsSimulating)
			{
				if (bIsSimulating)
				{
					FEditorDelegates::OnSwitchBeginPIEAndSIE.RemoveAll(this);
					if (AActor* SimActor = EditorUtilities::GetSimWorldCounterpartActor(WeakActor.Get()))
					{
						FocusActor(SimActor);
					}
					else
					{
						FocusActor(WeakActor.Get());
					}
				}
			});
			GEditor->RequestToggleBetweenPIEandSIE();
			return;
		}

		GEditor->SelectNone(/*bNoteSelectionChange=*/ false, false, false);
		GEditor->SelectActor(Actor, /*bSelected=*/ true, /*bNotify=*/ true);
		GEditor->NoteSelectionChange();

		GEditor->MoveViewportCamerasToActor(*Actor, true);

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
		if (LevelEditorTab.IsValid())
		{
			LevelEditorTab->DrawAttention();
		}
	}
}

void FFlowIdentityCustomization::UseActor_SelectedInViewport() const
{
	TArray<AActor*> SelectedActors = GEditor->GetEditorSubsystem<UEditorActorSubsystem>()->GetSelectedLevelActors();
	for (const AActor* SelectedActor : SelectedActors)
	{
		UFlowComponent* FlowComponent = SelectedActor ? SelectedActor->FindComponentByClass<UFlowComponent>() : nullptr;
		if (FlowComponent)
		{
			if (TagSelector)
			{
				TagSelector->SetSourceFromComponent(FlowComponent);
			}
			break;
		}
	}
}

void FFlowIdentityCustomization::UseActor_Explicit(AActor* Actor) const
{
	if (TagSelector)
	{
		TagSelector->SetSourceFromActor(Actor);
	}
}

void FFlowIdentityCustomization::UseActor_FromEyeDrop()
{
	const FActorPickerModeModule& ActorPickerMode = FModuleManager::Get().GetModuleChecked<FActorPickerModeModule>("ActorPickerMode");

	if (ActorPickerMode.IsInActorPickingMode())
	{
		ActorPickerMode.EndActorPickingMode();
	}
	else
	{
		const FOnGetAllowedClasses OnGetAllowedClasses = FOnGetAllowedClasses::CreateLambda([](TArray<const UClass*>& Classes)
		{
			Classes.Add(AActor::StaticClass());
		});

		const FOnShouldFilterActor OnShouldFilterActor = FOnShouldFilterActor::CreateSP(this, &FFlowIdentityCustomization::IsActorAllowed);
		const FOnActorPicked OnActorPicked = FOnActorPicked::CreateSP(this, &FFlowIdentityCustomization::OpenTagPicker_UseActor);

		ActorPickerMode.BeginActorPickingMode(OnGetAllowedClasses, OnShouldFilterActor, OnActorPicked);

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
		if (LevelEditorTab.IsValid())
		{
			LevelEditorTab->DrawAttention();
		}
	}
}

TSharedRef<SWidget> FFlowIdentityCustomization::MenuContent_ActorPicker()
{
	FMenuBuilder MenuBuilder(false, nullptr);
	const FUIAction NoAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; }));

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("UseSelected", "Use actor selected in viewport"), FText::GetEmpty(), FSlateIcon(),
		                         FUIAction(FExecuteAction::CreateSP(this, &FFlowIdentityCustomization::UseActor_SelectedInViewport))
		);

		const FOnShouldFilterActor OnShouldFilter = FOnShouldFilterActor::CreateSP(this, &FFlowIdentityCustomization::IsActorAllowed);
		const FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::Get().LoadModuleChecked<FSceneOutlinerModule>(TEXT("SceneOutliner"));

		FSceneOutlinerInitializationOptions InitOptions;
		InitOptions.Filters->AddFilterPredicate<FActorTreeItem>(OnShouldFilter);
		InitOptions.bFocusSearchBoxWhenOpened = true;

		InitOptions.ColumnMap.Add(FSceneOutlinerBuiltInColumnTypes::Label(), FSceneOutlinerColumnInfo(ESceneOutlinerColumnVisibility::Visible, 0));
		InitOptions.ColumnMap.Add(FSceneOutlinerBuiltInColumnTypes::ActorInfo(), FSceneOutlinerColumnInfo(ESceneOutlinerColumnVisibility::Visible, 10));

		const TSharedPtr<SWidget> MenuContent =
			SNew(SBox)
			.WidthOverride(350)
			.HeightOverride(300)
			[
				SceneOutlinerModule.CreateActorPicker(InitOptions, FOnActorPicked::CreateSP(this, &FFlowIdentityCustomization::UseActor_Explicit))
			];

		MenuBuilder.AddWidget(MenuContent.ToSharedRef(), FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	const TSharedRef<SWidget> PickerWidget = SNew(SBox)
		.Padding(0, 0, 0, -15)
		[
			CreateTagPicker()
		];
	MenuBuilder.AddWidget(PickerWidget, FText::GetEmpty(), true);

	return SNew(SBox)
		[
			MenuBuilder.MakeWidget()
		];
}

TSharedRef<SWidget> FFlowIdentityCustomization::MenuContent_FindMatching()
{	
	const FOnShouldFilterActor OnShouldFilter = FOnShouldFilterActor::CreateSP(this, &FFlowIdentityCustomization::IsActorMatches);

	const FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::Get().LoadModuleChecked<FSceneOutlinerModule>(TEXT("SceneOutliner"));

	FSceneOutlinerInitializationOptions InitOptions;
	InitOptions.Filters->AddFilterPredicate<FActorTreeItem>(OnShouldFilter);
	InitOptions.bFocusSearchBoxWhenOpened = true;

	InitOptions.ColumnMap.Add(FSceneOutlinerBuiltInColumnTypes::Label(), FSceneOutlinerColumnInfo(ESceneOutlinerColumnVisibility::Visible, 0));
	InitOptions.ColumnMap.Add(FSceneOutlinerBuiltInColumnTypes::ActorInfo(), FSceneOutlinerColumnInfo(ESceneOutlinerColumnVisibility::Visible, 10));

	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseMatching", "Browse Matching Actors"));
	{
		const int32 Num = StructPropertyHandle->GetNumPerObjectValues();
		if (Num > 1)
		{
			MenuBuilder.AddMenuEntry(FText::Format(LOCTEXT("Multiple filters", "Showing any matches for {0} selected identities"), FText::AsNumber(Num)), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}

		const TSharedPtr<SWidget> MenuContent =
			SNew(SBox)
			.WidthOverride(350)
			.HeightOverride(300)
			[
				SceneOutlinerModule.CreateActorPicker(InitOptions, FOnActorPicked::CreateSP(this, &FFlowIdentityCustomization::FocusActor))
			];

		MenuBuilder.AddWidget(MenuContent.ToSharedRef(), FText::GetEmpty(), true);
	}

	return MenuBuilder.MakeWidget();
}

bool FFlowIdentityCustomization::CanOpenMatchingPopup()
{
	if (GEditor)
	{
		if (!GEditor->PlayWorld)
		{
			return true;
		}

		// Avoids triggering ensure in UEditorEngine::ToggleBetweenPIE and SIE for PlayInNewWindow mode
		for (const FWorldContext& WorldContext : GEditor->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::PIE && !WorldContext.RunAsDedicated)
			{
				const FSlatePlayInEditorInfo* SlatePlayInEditorInfo = GEditor->SlatePlayInEditorMap.Find(WorldContext.ContextHandle);
				return SlatePlayInEditorInfo && SlatePlayInEditorInfo->DestinationSlateViewport.IsValid();
			}
		}
	}

	return false;
}

void FFlowIdentityCustomization::FindMatchingPopup()
{
	FVector2d Position;
	if (PopUpAnchor.IsValid())
	{
		const FGeometry& Geometry = PopUpAnchor->GetCachedGeometry();
		Position = Geometry.LocalToAbsolute(Geometry.GetLocalSize());
	}
	else
	{
		Position = FSlateApplication::Get().GetCursorPos();
	}

	FSlateApplication::Get().PushMenu(
		HeaderWidget.ToSharedRef(),
		FWidgetPath(),
		MenuContent_FindMatching(),
		Position,
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

#undef LOCTEXT_NAMESPACE
