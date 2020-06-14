#include "LevelEditor/SLevelEditorFlow.h"
#include "FlowAsset.h"
#include "FlowWorldSettings.h"

#include "Editor.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "PropertyCustomizationHelpers.h"
#include "SLevelOfDetailBranchNode.h"

#define LOCTEXT_NAMESPACE "SLevelEditorFlow"

void SLevelEditorFlow::Construct(const FArguments& InArgs)
{
	CreateFlowWidget();

	FEditorDelegates::OnMapOpened.AddRaw(this, &SLevelEditorFlow::OnMapOpened);
}

void SLevelEditorFlow::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
	CreateFlowWidget();
}

void SLevelEditorFlow::CreateFlowWidget()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (const AFlowWorldSettings* WorldSettings = Cast<AFlowWorldSettings>(World->GetWorldSettings()))
		{
			FlowPath = WorldSettings->FlowAsset ? FName(*WorldSettings->FlowAsset->GetPathName()) : FName();
		}
	}

	TSharedPtr<SObjectPropertyEntryBox> FlowPropertyBox = SNew(SObjectPropertyEntryBox)
		.AllowedClass(UFlowAsset::StaticClass())
		.DisplayThumbnail(false)
		.OnObjectChanged(this, &SLevelEditorFlow::OnFlowChanged)
		.ObjectPath(this, &SLevelEditorFlow::GetFlowPath);

	const TSharedRef<SWidget> FlowWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			FlowPropertyBox.ToSharedRef()
		];

	ChildSlot
		[
			SNew(SLevelOfDetailBranchNode)
			.UseLowDetailSlot(FMultiBoxSettings::UseSmallToolBarIcons)
			.LowDetail()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						FlowWidget
					]
			]
			.HighDetail()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Top)
					.Padding(5.0f)
					[
						FlowWidget
					]
			]
		];
}

void SLevelEditorFlow::OnFlowChanged(const FAssetData& NewAsset)
{
	FlowPath = NewAsset.ObjectPath;

	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (AFlowWorldSettings* WorldSettings = Cast<AFlowWorldSettings>(World->GetWorldSettings()))
		{
			if (UObject* NewObject = NewAsset.GetAsset())
			{
				WorldSettings->FlowAsset = Cast<UFlowAsset>(NewObject);
			}
			else
			{
				WorldSettings->FlowAsset = nullptr;
			}

			WorldSettings->MarkPackageDirty();
		}
	}
}

FString SLevelEditorFlow::GetFlowPath() const
{
	return FlowPath.IsValid() ? FlowPath.ToString() : FString();
}

#undef LOCTEXT_NAMESPACE