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

#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/Function.h"
#include "UnrealEdGlobals.h"
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
	// Clear only the last-hit breakpoint to return to enabled/disabled visuals without racing against
	// a newly hit breakpoint during FlushDeferredTriggerInputs().
	ClearHaltFlowExecution();
	ClearLastHitBreakpoint();

	// Editor-level resume event (also used by Advance Single Frame).
	// This does not necessarily flow through AGameModeBase::ClearPause(), so we must unhalt Flow here.
	ResumeSession();

	FFlowExecutionGate::FlushDeferredTriggerInputs();
}

void UFlowDebugEditorSubsystem::OnEndPIE(const bool bIsSimulating)
{
	// Ensure we don't carry over a halted state between PIE sessions.
	ClearHitBreakpoints();

	ClearHaltFlowExecution();
	FFlowExecutionGate::FlushDeferredTriggerInputs();

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

void UFlowDebugEditorSubsystem::PauseSession()
{
	// do not call Super, non-PIE world has its only Pause/Resume logic

	constexpr bool bShouldBePaused = true;
	const bool bWasPaused = GUnrealEd->SetPIEWorldsPaused(bShouldBePaused);
	if (!bWasPaused)
	{
		GUnrealEd->PlaySessionPaused();
	}
}

void UFlowDebugEditorSubsystem::ResumeSession()
{
	// do not call Super, non-PIE world has its only Pause/Resume logic

	constexpr bool bShouldBePaused = false;
	const bool bWasPaused = GUnrealEd->SetPIEWorldsPaused(bShouldBePaused);
	if (bWasPaused)
	{
		GUnrealEd->PlaySessionResumed();
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