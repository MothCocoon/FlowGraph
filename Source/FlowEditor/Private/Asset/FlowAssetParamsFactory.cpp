// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetParamsFactory.h"

#include "Asset/FlowAssetParams.h"
#include "Asset/FlowAssetParamsUtils.h"

#include "ContentBrowserModule.h"
#include "Framework/Application/SlateApplication.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowAssetParamsFactory"

UFlowAssetParamsFactory::UFlowAssetParamsFactory()
{
	SupportedClass = UFlowAssetParams::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

bool UFlowAssetParamsFactory::ConfigureProperties()
{
	SelectedParentParams.Reset();
	return ShowParentPickerDialog();
}

bool UFlowAssetParamsFactory::ShowParentPickerDialog()
{
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	// Holds current required parent selection (the params asset)
	TSharedPtr<FAssetData> CurrentSelection = MakeShared<FAssetData>();

	FAssetPickerConfig ParamsPickerConfig;
	ParamsPickerConfig.Filter.ClassPaths.Add(UFlowAssetParams::StaticClass()->GetClassPathName());
	ParamsPickerConfig.InitialAssetViewType = EAssetViewType::List;
	ParamsPickerConfig.SelectionMode = ESelectionMode::Single;
	ParamsPickerConfig.bAllowNullSelection = false;
	ParamsPickerConfig.bFocusSearchBoxWhenOpened = true;

	ParamsPickerConfig.OnAssetSelected = FOnAssetSelected::CreateLambda(
		[CurrentSelection](const FAssetData& AssetData)
		{
			*CurrentSelection = AssetData;
		});

	const TSharedRef<SWidget> ParamsPicker = ContentBrowserModule.Get().CreateAssetPicker(ParamsPickerConfig);

	bool bUserAccepted = false;

	TSharedPtr<SWindow> PickerWindow = SNew(SWindow)
		.Title(LOCTEXT("CreateChildParamsTitle", "Create Flow Asset Params"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(850, 600))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	PickerWindow->SetContent(
		SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SVerticalBox)

			// Parent picker
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("CreateChildParamsHelp", "Choose Parent Flow Asset Params:\n"))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						ParamsPicker
					]
			]

			// Buttons
			+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(SButton)
								.Text(LOCTEXT("OK", "OK"))
								.IsEnabled_Lambda([CurrentSelection]()
									{
										return CurrentSelection->IsValid();
									})
								.OnClicked_Lambda([&bUserAccepted, PickerWindow]()
									{
										bUserAccepted = true;
										PickerWindow->RequestDestroyWindow();
										return FReply::Handled();
									})
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
								.Text(LOCTEXT("Cancel", "Cancel"))
								.OnClicked_Lambda([PickerWindow]()
									{
										PickerWindow->RequestDestroyWindow();
										return FReply::Handled();
									})
						]
				]
		]
	);

	FSlateApplication::Get().AddModalWindow(PickerWindow.ToSharedRef(), nullptr);

	if (!bUserAccepted || !CurrentSelection->IsValid())
	{
		return false;
	}

	SelectedParentParams = TSoftObjectPtr<UFlowAssetParams>(CurrentSelection->ToSoftObjectPath());
	if (SelectedParentParams.IsNull())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoParentSelected", "You must select a parent Flow Asset Params asset."));

		return false;
	}

	return true;
}

UObject* UFlowAssetParamsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (SelectedParentParams.IsNull())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FactoryMissingParent", "No parent params were selected."));

		return nullptr;
	}

	UFlowAssetParams* Parent = SelectedParentParams.LoadSynchronous();
	if (!IsValid(Parent))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ParentLoadFail", "Failed to load selected parent params:\n{0}"),
				FText::FromString(SelectedParentParams.ToString())));

		return nullptr;
	}

	FText FailureReason;
	constexpr bool bShowDialogs = true;
	UFlowAssetParams* NewParams = FFlowAssetParamsUtils::CreateChildParamsAsset(*Parent, bShowDialogs, &FailureReason);

	// FactoryCreateNew expects the created asset (or nullptr). The helper already shows dialogs/logs on failure.
	return NewParams;
}

#undef LOCTEXT_NAMESPACE