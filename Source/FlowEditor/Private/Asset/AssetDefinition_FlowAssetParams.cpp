// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/AssetDefinition_FlowAssetParams.h"
#include "Asset/FlowAssetParams.h"
#include "Asset/FlowAssetParamsUtils.h"
#include "FlowEditorLogChannels.h"
#include "FlowEditorModule.h"
#include "ContentBrowserMenuContexts.h"
#include "ToolMenus.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_FlowAssetParams)

#define LOCTEXT_NAMESPACE "AssetDefinition_FlowAssetParams"

FText UAssetDefinition_FlowAssetParams::GetAssetDisplayName() const
{
	return LOCTEXT("GetAssetDisplayName", "Flow Asset Params");
}

FLinearColor UAssetDefinition_FlowAssetParams::GetAssetColor() const
{
	return FLinearColor(255, 196, 128);
}

TSoftClassPtr<UObject> UAssetDefinition_FlowAssetParams::GetAssetClass() const
{
	return UFlowAssetParams::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_FlowAssetParams::GetAssetCategories() const
{
	static const auto Categories = {FFlowAssetCategoryPaths::Flow};
	return Categories;
}

FAssetSupportResponse UAssetDefinition_FlowAssetParams::CanLocalize(const FAssetData& InAsset) const
{
	return FAssetSupportResponse::Supported();
}

namespace MenuExtension_FlowAssetParams
{
	static void ExecuteCreateChildParams(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
		if (!Context)
		{
			UE_LOG(LogFlowEditor, Warning, TEXT("No valid context for Create Child Params action"));
			return;
		}

		const TArray<UFlowAssetParams*>& SelectedParams = Context->LoadSelectedObjects<UFlowAssetParams>();
		if (SelectedParams.Num() != 1)
		{
			UE_LOG(LogFlowEditor, Warning, TEXT("Create Child Params requires exactly one selected Flow Asset Params"));
			return;
		}

		UFlowAssetParams* ParentParams = SelectedParams[0];
		if (!IsValid(ParentParams))
		{
			UE_LOG(LogFlowEditor, Error, TEXT("Invalid Flow Asset Params selected for Create Child Params"));
			return;
		}

		constexpr bool bShowDialogs = true;
		FFlowAssetParamsUtils::CreateChildParamsAsset(*ParentParams, bShowDialogs);
	}

	static void RegisterContextMenu()
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UFlowAssetParams::StaticClass());

			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
			Section.AddDynamicEntry("Flow Asset Params Commands", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				const TAttribute<FText> Label = LOCTEXT("FlowAssetParams_CreateChildParams", "Create Child Params");
				const TAttribute<FText> ToolTip = LOCTEXT("FlowAssetParams_CreateChildParamsTooltip", "Creates a new Flow Asset Params inheriting from the selected params.");
				const FSlateIcon Icon = FSlateIcon();

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteCreateChildParams);
				UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda([](const FToolMenuContext& InContext)
				{
					const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
					return Context && Context->SelectedAssets.Num() == 1;
				});
				InSection.AddMenuEntry("FlowAssetParams_CreateChildParams", Label, ToolTip, Icon, UIAction);
			}));
		}));
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, &RegisterContextMenu);
}

#undef LOCTEXT_NAMESPACE
