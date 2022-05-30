// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "LevelEditor/SLevelEditorFlow.h"
#include "FlowAsset.h"
#include "FlowComponent.h"
#include "FlowWorldSettings.h"

#include "Graph/FlowGraphSettings.h"

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
	UFlowComponent* FlowComponent = FindFlowComponent();
	if (FlowComponent && FlowComponent->RootFlow)
	{
		FlowPath = FName(*FlowComponent->RootFlow->GetPathName());
	}
	else
	{
		FlowPath = FName();
	}

	const TSharedRef<SWidget> FlowWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SObjectPropertyEntryBox)
					.AllowedClass(UFlowGraphSettings::Get()->WorldAssetClass)
					.DisplayThumbnail(false)
					.OnObjectChanged(this, &SLevelEditorFlow::OnFlowChanged)
					.ObjectPath(this, &SLevelEditorFlow::GetFlowPath)
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

	if (UFlowComponent* FlowComponent = FindFlowComponent())
	{
		if (UObject* NewObject = NewAsset.GetAsset())
		{
			FlowComponent->RootFlow = Cast<UFlowAsset>(NewObject);
		}
		else
		{
			FlowComponent->RootFlow = nullptr;
		}

		const bool bSuccess = FlowComponent->MarkPackageDirty();
		ensureMsgf(bSuccess, TEXT("World Settings couldn't be marked dirty while changing the assigned Flow Asset."));
	}
}

FString SLevelEditorFlow::GetFlowPath() const
{
	return FlowPath.IsValid() ? FlowPath.ToString() : FString();
}

UFlowComponent* SLevelEditorFlow::FindFlowComponent() const
{
	if (const UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (const AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			if (UActorComponent* FoundComponent = WorldSettings->GetComponentByClass(UFlowComponent::StaticClass()))
			{
				return Cast<UFlowComponent>(FoundComponent);
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
