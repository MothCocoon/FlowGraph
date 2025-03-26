// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDebugEditorSubsystem.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowMessageLogListing.h"
#include "Graph/FlowGraphUtils.h"

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
	: bOverrideInspectedInstance(false)
	, bPausedAtFlowBreakpoint(false)
{
	FEditorDelegates::BeginPIE.AddUObject(this, &ThisClass::OnBeginPIE);
	FEditorDelegates::ResumePIE.AddUObject(this, &ThisClass::OnResumePIE);
	FEditorDelegates::EndPIE.AddUObject(this, &ThisClass::OnEndPIE);
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

void UFlowDebugEditorSubsystem::OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate) const
{
	AssetTemplate->OnRuntimeMessageAdded().RemoveAll(this);

	Super::OnInstancedTemplateRemoved(AssetTemplate);
}

void UFlowDebugEditorSubsystem::OnPinTriggered(const UFlowAsset* Instance, const FGuid& NodeGuid, const FName& PinName)
{
	if (bPausedAtFlowBreakpoint)
	{
		return;
	}
	
	if (ensure(Instance))
	{
		const UFlowAsset* InspectedInstance = Instance->GetTemplateAsset()->GetInspectedInstance();
		const FStringView InstanceName = Instance->GetTemplateAsset()->GetLastInspectedInstanceName();
		TWeakObjectPtr<const UWorld> DebugWorld = Instance->GetTemplateAsset()->GetWorldBeingDebugged();

		bOverrideInspectedInstance = false;

		if (InspectedInstance)
		{
			if (InspectedInstance != Instance)
			{
				return;
			}
		}
		else if (!InstanceName.IsEmpty() && InstanceName != Instance->GetDebugName())
		{
			// there is other inspected asset, from previous PIE session. Wait for it
			return;
		}
		else if (DebugWorld.IsValid() && DebugWorld.Get() != Instance->GetWorld())
		{
			return;
		}
		else
		{
			bOverrideInspectedInstance = true;
		}

		OverrideInstancePtr = Instance;
	
		Super::OnPinTriggered(Instance, NodeGuid, PinName);
	}
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
	// clear all logs from a previous session
	RuntimeLogs.Empty();
}

void UFlowDebugEditorSubsystem::OnResumePIE(const bool bIsSimulating)
{
	ClearPausedState();
}

void UFlowDebugEditorSubsystem::OnEndPIE(const bool bIsSimulating)
{
	ClearPausedState();

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

void UFlowDebugEditorSubsystem::PauseSession(const FGuid& FromNode)
{
	if (GEditor->ShouldEndPlayMap())
	{
		return;
	}
	
	if (GUnrealEd->SetPIEWorldsPaused(true))
	{
		bPausedAtFlowBreakpoint = true;

		check(OverrideInstancePtr.IsValid());
		UFlowAsset* TemplateInstance = OverrideInstancePtr->GetTemplateAsset();
		
		if (bOverrideInspectedInstance)
		{
			TemplateInstance->SetInspectedInstance(OverrideInstancePtr);
		}

		UFlowNode* FlowNode = TemplateInstance->GetNode(FromNode);
		check(FlowNode);

		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem->OpenEditorForAsset(TemplateInstance))
		{
			if (const TSharedPtr<FFlowAssetEditor> FlowAssetEditor = FFlowGraphUtils::GetFlowAssetEditor(TemplateInstance))
			{
				FlowAssetEditor->JumpToNode(FlowNode->GetGraphNode());
			}
		}
		
		GUnrealEd->PlaySessionPaused();
	}
}

void UFlowDebugEditorSubsystem::ClearPausedState()
{
	ClearHitBreakpoints();
	
	if (bPausedAtFlowBreakpoint && bOverrideInspectedInstance)
	{
		bOverrideInspectedInstance = false;
		
		// do not reset inspected instance if it does not match temporary OverrideInstance. It means user selected other instance when editor was in debug mode
		if (ensure(OverrideInstancePtr.IsValid()) && OverrideInstancePtr == OverrideInstancePtr->GetTemplateAsset()->GetInspectedInstance())
		{
			OverrideInstancePtr->GetTemplateAsset()->SetInspectedInstance(nullptr);
		}
	}

	bPausedAtFlowBreakpoint = false;
}

#undef LOCTEXT_NAMESPACE
