// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDebugEditorSubsystem.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowMessageLogListing.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Interfaces/FlowExecutionGate.h"
#include "FlowAsset.h"

#include "CoreGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/Function.h"
#include "Templates/UnrealTemplate.h"
#include "UnrealEdGlobals.h"
#include "UnrealEngine.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDebugEditorSubsystem)

#define LOCTEXT_NAMESPACE "FlowDebugEditorSubsystem"

UFlowDebugEditorSubsystem::UFlowDebugEditorSubsystem()
{
	FEditorDelegates::BeginPIE.AddUObject(this, &ThisClass::OnBeginPIE);
	FEditorDelegates::ResumePIE.AddUObject(this, &ThisClass::OnResumePIE);
	FEditorDelegates::EndPIE.AddUObject(this, &ThisClass::OnEndPIE);

	OnDebuggerBreakpointHit.AddUObject(this, &ThisClass::OnBreakpointHit);
}

void UFlowDebugEditorSubsystem::OnInstancedTemplateAdded(UFlowAsset* AssetTemplate)
{
	Super::OnInstancedTemplateAdded(AssetTemplate);

	if (!RuntimeLogs.Contains(AssetTemplate))
	{
		RuntimeLogs.Add(AssetTemplate, FFlowMessageLogListing::GetLogListing(AssetTemplate, EFlowLogType::Runtime));
		AssetTemplate->OnRuntimeMessageAdded().AddUObject(this, &UFlowDebugEditorSubsystem::OnRuntimeMessageAdded);
	}
}

void UFlowDebugEditorSubsystem::OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate)
{
	AssetTemplate->OnRuntimeMessageAdded().RemoveAll(this);

	Super::OnInstancedTemplateRemoved(AssetTemplate);
}

void UFlowDebugEditorSubsystem::OnRuntimeMessageAdded(const UFlowAsset* AssetTemplate, const TSharedRef<FTokenizedMessage>& Message) const
{
	const TSharedPtr<class IMessageLogListing> Log = RuntimeLogs.FindRef(AssetTemplate);
	if (Log.IsValid())
	{
		Log->AddMessage(Message);
		Log->OnDataChanged().Broadcast();
	}
}

void UFlowDebugEditorSubsystem::OnBeginPIE(const bool bIsSimulating)
{
	// Clear all logs from a previous session
	RuntimeLogs.Empty();

	// Clear any stale "hit" state from previous run
	ClearHitBreakpoints();
}

void UFlowDebugEditorSubsystem::OnResumePIE(const bool bIsSimulating)
{
	// Editor-level resume event (also used by Advance Single Frame).
	// This does not necessarily flow through AGameModeBase::ClearPause(), so we must unhalt Flow here.
	ClearLastHitBreakpoint();

	if (HaltedOnFlowAssetInstance.IsValid())
	{
		ResumeSession(*HaltedOnFlowAssetInstance.Get());
	}

	// Release the suspended Flow call stack after updating the debugger session state.
	ReleaseHalt();
}

void UFlowDebugEditorSubsystem::OnEndPIE(const bool bIsSimulating)
{
	// Ensure we don't carry over a halted state between PIE sessions.
	ClearHitBreakpoints();

	// A Flow stack halted at a breakpoint must be released, or the nested tick loop would keep the
	// editor inside a torn-down play session.
	ReleaseHalt();

	StopSession();

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

EFlowBreakAction UFlowDebugEditorSubsystem::HaltUntilReleased(const FFlowBreakContext& Context)
{
	if (!GUnrealEd || !IsValid(GUnrealEd->PlayWorld) || !FSlateApplication::IsInitialized())
	{
		// There is no play session to stop, or no Slate application to pump while stopped
		return EFlowBreakAction::Continue;
	}

	{
		TGuardValue<bool> HaltedGuard(bIsHaltedAtBreakpoint, true);
		TGuardValue<bool> DebuggingGuard(GIntraFrameDebuggingGameThread, true);

		// The level toolbar's own Resume and Stop do not work while an in-stack halt is active
		// without a registered Blueprint debugging world, so the halt has to offer its own way out.
		ShowHaltNotification(Context);

		// Keep editor-world work issued from the halted UI off the play world
		const FTemporaryPlayInEditorIDOverride PlayInEditorIDOverride(INDEX_NONE);

		// Suspends this call stack in place and pumps Slate until released, as Blueprint breakpoints do.
		// World ticks do not run during the halt, but its wall-clock duration is not removed from the next frame's delta.
		FSlateApplication::Get().EnterDebuggingMode();

		DismissHaltNotification();
	}

	if (!GEditor || GEditor->ShouldEndPlayMap())
	{
		// The session is being stopped, so unwind rather than resume into a world that is going away
		return EFlowBreakAction::Abort;
	}

	return EFlowBreakAction::Continue;
}

void UFlowDebugEditorSubsystem::ReleaseHalt()
{
	if (!bIsHaltedAtBreakpoint)
	{
		return;
	}

	// Ends the nested tick loop in HaltUntilReleased() on its next iteration. BreakFlowExecution()
	// resumes the debugger session after the halt returns if another path has not already resumed it.
	FSlateApplication::Get().LeaveDebuggingMode();
}

void UFlowDebugEditorSubsystem::ShowHaltNotification(const FFlowBreakContext& Context)
{
	DismissHaltNotification();

	const FText NodeTitle = IsValid(Context.Node) ?
		FText::FromString(Context.Node->GetName()) :
		LOCTEXT("UnknownFlowNode", "unknown node");

	FNotificationInfo Info{LOCTEXT("FlowExecutionHalted", "Flow execution halted at a breakpoint")};

	// Stays up for the whole halt, since it carries the only reliable way to continue
	Info.bFireAndForget = false;
	Info.SubText = FText::Format(
		LOCTEXT("FlowExecutionHaltedSubText", "{0} ({1})"), NodeTitle, FText::FromName(Context.PinName));

	Info.HyperlinkText = LOCTEXT("ContinueFlowExecution", "Continue");
	Info.Hyperlink = FSimpleDelegate::CreateUObject(this, &ThisClass::RequestContinue);

	HaltNotification = FSlateNotificationManager::Get().AddNotification(Info);

	if (HaltNotification.IsValid())
	{
		HaltNotification->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void UFlowDebugEditorSubsystem::DismissHaltNotification()
{
	if (HaltNotification.IsValid())
	{
		HaltNotification->SetCompletionState(SNotificationItem::CS_None);
		HaltNotification->SetFadeOutDuration(0.0f);
		HaltNotification->ExpireAndFadeout();
		HaltNotification.Reset();
	}
}

void UFlowDebugEditorSubsystem::PauseSession(UFlowAsset& FlowAssetInstance)
{
	HaltedOnFlowAssetInstance = &FlowAssetInstance;

	Super::PauseSession(FlowAssetInstance);
}

void UFlowDebugEditorSubsystem::ResumeSession(UFlowAsset& FlowAssetInstance)
{
	HaltedOnFlowAssetInstance = &FlowAssetInstance;

	Super::ResumeSession(FlowAssetInstance);
}

void UFlowDebugEditorSubsystem::StopSession()
{
	HaltedOnFlowAssetInstance.Reset();

	Super::StopSession();
}

void UFlowDebugEditorSubsystem::OnFlowDebuggerStateChanged(EFlowDebuggerState PrevState, EFlowDebuggerState NextState, UFlowAsset* FlowAssetInstance)
{
	check(PrevState != NextState);

	using namespace EFlowDebuggerState_Classifiers;

	const bool bIsPausedGameStatePrev = IsPausedGameState(PrevState);
	const bool bIsPausedGameStateNext = IsPausedGameState(NextState);

	// Report the session as paused or resumed to the rest of the editor, but deliberately do not flag
	// the play worlds as paused. While halted the game is frozen by the nested tick loop rather than
	// by that flag, and setting it would show the level toolbar's Resume button, which cannot run
	// during an in-stack halt and so would read as a wedged editor.
	if (bIsPausedGameStatePrev != bIsPausedGameStateNext && GUnrealEd && IsValid(GUnrealEd->PlayWorld))
	{
		if (bIsPausedGameStateNext)
		{
			GUnrealEd->PlaySessionPaused();
		}
		else
		{
			GUnrealEd->PlaySessionResumed();
		}
	}

	// Issue the broadcasts for specific state entry
	FLOW_ASSERT_ENUM_MAX(EFlowDebuggerState, 3);
	if (NextState == EFlowDebuggerState::Paused)
	{
		OnDebuggerPaused.Broadcast(*FlowAssetInstance);
	}
	else if (NextState == EFlowDebuggerState::Resumed)
	{
		OnDebuggerResumed.Broadcast(*FlowAssetInstance);
	}
}

void UFlowDebugEditorSubsystem::OnBreakpointHit(const UFlowNode* FlowNode) const
{
	UFlowAsset* TemplateAsset = const_cast<UFlowAsset*>(FlowNode->GetFlowAsset()->GetTemplateAsset());
	if (!IsValid(TemplateAsset))
	{
		return;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
	if (!AssetEditorSubsystem)
	{
		return;
	}

	if (!AssetEditorSubsystem->OpenEditorForAsset(TemplateAsset))
	{
		return;
	}

	TemplateAsset->SetInspectedInstance(FlowNode->GetFlowAsset());

	UFlowGraph* FlowGraph = Cast<UFlowGraph>(TemplateAsset->GetGraph());
	if (!IsValid(FlowGraph))
	{
		return;
	}

	// NOTE: This may be redundant call, but it ensures Slate re-queries breakpoint hit state and updates node overlays immediately.
	FlowGraph->NotifyGraphChanged();

	UEdGraphNode* NodeToFocus = nullptr;
	for (UEdGraphNode* Node : FlowGraph->Nodes)
	{
		UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node);
		if (IsValid(FlowGraphNode) && FlowGraphNode->NodeGuid == FlowNode->NodeGuid)
		{
			NodeToFocus = FlowGraphNode;
			break;
		}
	}

	if (!NodeToFocus)
	{
		return;
	}

	const TSharedPtr<SFlowGraphEditor> GraphEditor = FFlowGraphUtils::GetFlowGraphEditor(FlowGraph);
	if (GraphEditor.IsValid())
	{
		constexpr bool bRequestRename = false;
		constexpr bool bSelectNode = true;

		GraphEditor->JumpToNode(NodeToFocus, bRequestRename, bSelectNode);
	}
}

#undef LOCTEXT_NAMESPACE