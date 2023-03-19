// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDebuggerSubsystem.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowMessageLogListing.h"

#include "FlowSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Templates/Function.h"
#include "UnrealEd.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FlowDebuggerSubsystem"

UFlowDebuggerSubsystem::UFlowDebuggerSubsystem()
{
	FEditorDelegates::BeginPIE.AddUObject(this, &UFlowDebuggerSubsystem::OnBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &UFlowDebuggerSubsystem::OnEndPIE);

	UFlowSubsystem::OnInstancedTemplateAdded.BindUObject(this, &UFlowDebuggerSubsystem::OnInstancedTemplateAdded);
	UFlowSubsystem::OnInstancedTemplateRemoved.BindUObject(this, &UFlowDebuggerSubsystem::OnInstancedTemplateRemoved);
}

void UFlowDebuggerSubsystem::OnInstancedTemplateAdded(UFlowAsset* FlowAsset)
{
	if (!RuntimeLogs.Contains(FlowAsset))
	{
		RuntimeLogs.Add(FlowAsset, FFlowMessageLogListing::GetLogListing(FlowAsset, EFlowLogType::Runtime));
		FlowAsset->OnRuntimeMessageAdded().AddUObject(this, &UFlowDebuggerSubsystem::OnRuntimeMessageAdded);
	}
}

void UFlowDebuggerSubsystem::OnInstancedTemplateRemoved(UFlowAsset* FlowAsset)
{
	FlowAsset->OnRuntimeMessageAdded().RemoveAll(this);
}

void UFlowDebuggerSubsystem::OnRuntimeMessageAdded(UFlowAsset* FlowAsset, const TSharedRef<FTokenizedMessage>& Message) const
{
	const TSharedPtr<class IMessageLogListing> Log = RuntimeLogs.FindRef(FlowAsset);
	if (Log.IsValid())
	{
		Log->AddMessage(Message);
		Log->OnDataChanged().Broadcast();
	}
}

void UFlowDebuggerSubsystem::OnBeginPIE(const bool bIsSimulating)
{
	// clear all logs from a previous session
	RuntimeLogs.Empty();
}

void UFlowDebuggerSubsystem::OnEndPIE(const bool bIsSimulating)
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

void UFlowDebuggerSubsystem::PausePlaySession()
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

bool UFlowDebuggerSubsystem::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}

#undef LOCTEXT_NAMESPACE
