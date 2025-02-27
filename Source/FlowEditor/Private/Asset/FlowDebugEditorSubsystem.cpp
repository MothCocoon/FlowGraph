// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDebugEditorSubsystem.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowMessageLogListing.h"

#include "FlowSubsystem.h"

#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Templates/Function.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDebugEditorSubsystem)

#define LOCTEXT_NAMESPACE "FlowDebugEditorSubsystem"

UFlowDebugEditorSubsystem::UFlowDebugEditorSubsystem()
{
	FEditorDelegates::BeginPIE.AddUObject(this, &UFlowDebugEditorSubsystem::OnBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &UFlowDebugEditorSubsystem::OnEndPIE);

	UFlowSubsystem::OnInstancedTemplateAdded.BindUObject(this, &UFlowDebugEditorSubsystem::OnInstancedTemplateAdded);
	UFlowSubsystem::OnInstancedTemplateRemoved.BindUObject(this, &UFlowDebugEditorSubsystem::OnInstancedTemplateRemoved);
}

void UFlowDebugEditorSubsystem::OnInstancedTemplateAdded(UFlowAsset* FlowAsset)
{
	if (!RuntimeLogs.Contains(FlowAsset))
	{
		RuntimeLogs.Add(FlowAsset, FFlowMessageLogListing::GetLogListing(FlowAsset, EFlowLogType::Runtime));
		FlowAsset->OnRuntimeMessageAdded().AddUObject(this, &UFlowDebugEditorSubsystem::OnRuntimeMessageAdded);
	}
}

void UFlowDebugEditorSubsystem::OnInstancedTemplateRemoved(UFlowAsset* FlowAsset) const
{
	FlowAsset->OnRuntimeMessageAdded().RemoveAll(this);
}

void UFlowDebugEditorSubsystem::OnRuntimeMessageAdded(const UFlowAsset* FlowAsset, const TSharedRef<FTokenizedMessage>& Message) const
{
	const TSharedPtr<class IMessageLogListing> Log = RuntimeLogs.FindRef(FlowAsset);
	if (Log.IsValid())
	{
		Log->AddMessage(Message);
		Log->OnDataChanged().Broadcast();
	}
}

void UFlowDebugEditorSubsystem::OnBeginPIE(const bool bIsSimulating)
{
	// clear all logs from a previous session
	RuntimeLogs.Empty();
}

void UFlowDebugEditorSubsystem::OnEndPIE(const bool bIsSimulating)
{
	for (const TPair<TWeakObjectPtr<UFlowAsset>, TSharedPtr<class IMessageLogListing>>& Log : RuntimeLogs)
	{
		if (Log.Key.IsValid() && Log.Value->NumMessages(EMessageSeverity::Warning) > 0)
		{
			FNotificationInfo Info{FText::FromString(TEXT("Flow Graph reported in-game issues"))};
			Info.ExpireDuration = 15.0;

			Info.HyperlinkText = FText::Format(LOCTEXT("OpenFlowAssetHyperlink", "Open {0}"), FText::FromString(Log.Key->GetName()));
			Info.Hyperlink = FSimpleDelegate::CreateLambda([this, Log]()
			{
				UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				if (AssetEditorSubsystem->OpenEditorForAsset(Log.Key.Get()))
				{
					AssetEditorSubsystem->FindEditorForAsset(Log.Key.Get(), true)->InvokeTab(FFlowAssetEditor::RuntimeLogTab);
				}
			});

			const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
}

void ForEachGameWorld(const TFunction<void(UWorld*)>& Func)
{
	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			Func(PlayWorld);
		}
	}
}

bool AreAllGameWorldPaused()
{
	bool bPaused = true;
	ForEachGameWorld([&](const UWorld* World)
	{
		bPaused = bPaused && World->bDebugPauseExecution;
	});
	return bPaused;
}

void UFlowDebugEditorSubsystem::PausePlaySession()
{
	bool bPaused = false;
	ForEachGameWorld([&](UWorld* World)
	{
		if (!World->bDebugPauseExecution)
		{
			World->bDebugPauseExecution = true;
			bPaused = true;
		}
	});
	if (bPaused)
	{
		GUnrealEd->PlaySessionPaused();
	}
}

bool UFlowDebugEditorSubsystem::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}

#undef LOCTEXT_NAMESPACE
