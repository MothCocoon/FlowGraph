// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Debugger/FlowDebuggerSubsystem.h"
#include "Debugger/FlowDebuggerSettings.h"

#include "FlowAsset.h"
#include "FlowSubsystem.h"

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDebuggerSubsystem)

UFlowDebuggerSubsystem::UFlowDebuggerSubsystem()
	: bPausedAtFlowBreakpoint(false)
{
	UFlowSubsystem::OnInstancedTemplateAdded.BindUObject(this, &ThisClass::OnInstancedTemplateAdded);
	UFlowSubsystem::OnInstancedTemplateRemoved.BindUObject(this, &ThisClass::OnInstancedTemplateRemoved);
}

void UFlowDebuggerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FFlowExecutionGate::SetGate(this);
}

void UFlowDebuggerSubsystem::Deinitialize()
{
	if (FFlowExecutionGate::GetGate() == this)
	{
		FFlowExecutionGate::SetGate(nullptr);
	}

	Super::Deinitialize();
}

bool UFlowDebuggerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create an instance if there is no override implementation defined elsewhere
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);
	if (ChildClasses.Num() > 0)
	{
		return false;
	}

	return true;
}

void UFlowDebuggerSubsystem::OnInstancedTemplateAdded(UFlowAsset* AssetTemplate)
{
	check(IsValid(AssetTemplate));

	AssetTemplate->OnPinTriggered.BindUObject(this, &ThisClass::OnPinTriggered);
}

void UFlowDebuggerSubsystem::OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate)
{
	check(IsValid(AssetTemplate));

	AssetTemplate->OnPinTriggered.Unbind();

	OnDebuggerFlowAssetTemplateRemoved.Broadcast(*AssetTemplate);
}

void UFlowDebuggerSubsystem::OnPinTriggered(UFlowAsset* FlowAsset, const FGuid& NodeGuid, const FName& PinName)
{
	check(IsValid(FlowAsset));

	if (FindBreakpoint(NodeGuid, PinName))
	{
		MarkAsHit(*FlowAsset, NodeGuid, PinName);
	}

	// Node breakpoints waits on any pin triggered
	MarkAsHit(*FlowAsset, NodeGuid);
}

void UFlowDebuggerSubsystem::AddBreakpoint(const FGuid& NodeGuid)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	FNodeBreakpoint& NodeBreakpoint = Settings->NodeBreakpoints.FindOrAdd(NodeGuid);

	NodeBreakpoint.Breakpoint.SetActive(true);
	SaveSettings();
}

void UFlowDebuggerSubsystem::AddBreakpoint(const FGuid& NodeGuid, const FName& PinName)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	FNodeBreakpoint& NodeBreakpoint = Settings->NodeBreakpoints.FindOrAdd(NodeGuid);
	FFlowBreakpoint& PinBreakpoint = NodeBreakpoint.PinBreakpoints.FindOrAdd(PinName);

	PinBreakpoint.SetEnabled(true);
	SaveSettings();
}

void UFlowDebuggerSubsystem::RemoveAllBreakpoints(const TWeakObjectPtr<UFlowAsset> FlowAsset)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	for (auto& [NodeGuid, Node] : FlowAsset->GetNodes())
	{
		if (Settings->NodeBreakpoints.Contains(NodeGuid))
		{
			Settings->NodeBreakpoints.Remove(NodeGuid);
		}
	}

	SaveSettings();
}

void UFlowDebuggerSubsystem::RemoveAllBreakpoints(const FGuid& NodeGuid)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();

	if (Settings->NodeBreakpoints.Contains(NodeGuid))
	{
		Settings->NodeBreakpoints.Remove(NodeGuid);
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::RemoveNodeBreakpoint(const FGuid& NodeGuid)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	if (FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(NodeGuid))
	{
		if (NodeBreakpoint->PinBreakpoints.IsEmpty())
		{
			// no pin breakpoints here, remove the entire entry
			Settings->NodeBreakpoints.Remove(NodeGuid);
		}
		else
		{
			// there are pin breakpoints here, only deactivate node breakpoint
			NodeBreakpoint->Breakpoint.SetActive(false);
		}

		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::RemovePinBreakpoint(const FGuid& NodeGuid, const FName& PinName)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	if (FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(NodeGuid))
	{
		if (NodeBreakpoint->PinBreakpoints.Contains(PinName))
		{
			NodeBreakpoint->PinBreakpoints.Remove(PinName);
		}

		if (!NodeBreakpoint->Breakpoint.IsActive() && NodeBreakpoint->PinBreakpoints.IsEmpty())
		{
			// no breakpoints remained, remove the entire entry
			Settings->NodeBreakpoints.Remove(NodeGuid);
		}

		SaveSettings();
	}
}

#if WITH_EDITOR
void UFlowDebuggerSubsystem::RemoveObsoletePinBreakpoints(const UEdGraphNode* Node)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	if (FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(Node->NodeGuid))
	{
		bool bAnythingRemoved = false;

		TSet<FName> PinNames;
		PinNames.Reserve(Node->Pins.Num());
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			PinNames.Emplace(Pin->PinName);
		}

		TArray<FName> PinsToRemove;
		PinsToRemove.Reserve(NodeBreakpoint->PinBreakpoints.Num());

		for (const TPair<FName, FFlowBreakpoint>& PinBreakpoint : NodeBreakpoint->PinBreakpoints)
		{
			if (!PinNames.Contains(PinBreakpoint.Key))
			{
				PinsToRemove.Add(PinBreakpoint.Key);
			}
		}

		for (const FName& PinName : PinsToRemove)
		{
			NodeBreakpoint->PinBreakpoints.Remove(PinName);
			bAnythingRemoved = true;
		}

		if (NodeBreakpoint->IsEmpty())
		{
			Settings->NodeBreakpoints.Remove(Node->NodeGuid);
			bAnythingRemoved = true;
		}

		if (bAnythingRemoved)
		{
			SaveSettings();
		}
	}
}
#endif

void UFlowDebuggerSubsystem::ToggleBreakpoint(const FGuid& NodeGuid)
{
	if (FindBreakpoint(NodeGuid) == nullptr)
	{
		AddBreakpoint(NodeGuid);
	}
	else
	{
		RemoveNodeBreakpoint(NodeGuid);
	}
}

void UFlowDebuggerSubsystem::ToggleBreakpoint(const FGuid& NodeGuid, const FName& PinName)
{
	if (FindBreakpoint(NodeGuid, PinName) == nullptr)
	{
		AddBreakpoint(NodeGuid, PinName);
	}
	else
	{
		RemovePinBreakpoint(NodeGuid, PinName);
	}
}

FFlowBreakpoint* UFlowDebuggerSubsystem::FindBreakpoint(const FGuid& NodeGuid)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(NodeGuid);
	if (NodeBreakpoint && NodeBreakpoint->Breakpoint.IsActive())
	{
		return &NodeBreakpoint->Breakpoint;
	}

	return nullptr;
}

FFlowBreakpoint* UFlowDebuggerSubsystem::FindBreakpoint(const FGuid& NodeGuid, const FName& PinName)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(NodeGuid);
	return NodeBreakpoint ? NodeBreakpoint->PinBreakpoints.Find(PinName) : nullptr;
}

bool UFlowDebuggerSubsystem::HasAnyBreakpoints(const TWeakObjectPtr<UFlowAsset> FlowAsset)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
	{
		if (Settings->NodeBreakpoints.Find(Node.Key))
		{
			return true;
		}
	}

	return false;
}

void UFlowDebuggerSubsystem::SetBreakpointEnabled(const FGuid& NodeGuid, const bool bEnabled)
{
	if (FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(NodeGuid))
	{
		NodeBreakpoint->SetEnabled(bEnabled);
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::SetBreakpointEnabled(const FGuid& NodeGuid, const FName& PinName, const bool bEnabled)
{
	if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(NodeGuid, PinName))
	{
		PinBreakpoint->SetEnabled(bEnabled);
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::SetAllBreakpointsEnabled(const TWeakObjectPtr<UFlowAsset> FlowAsset, const bool bEnabled)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
	{
		if (FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(Node.Key))
		{
			if (NodeBreakpoint->Breakpoint.IsActive())
			{
				NodeBreakpoint->Breakpoint.SetEnabled(bEnabled);
			}

			for (auto& [Name, PinBreakpoint] : NodeBreakpoint->PinBreakpoints)
			{
				PinBreakpoint.SetEnabled(bEnabled);
			}
		}
	}

	SaveSettings();
}

bool UFlowDebuggerSubsystem::IsBreakpointEnabled(const FGuid& NodeGuid)
{
	if (const FFlowBreakpoint* PinBreakpoint = FindBreakpoint(NodeGuid))
	{
		return PinBreakpoint->IsEnabled();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsBreakpointEnabled(const FGuid& NodeGuid, const FName& PinName)
{
	if (const FFlowBreakpoint* PinBreakpoint = FindBreakpoint(NodeGuid, PinName))
	{
		return PinBreakpoint->IsEnabled();
	}

	return false;
}

void UFlowDebuggerSubsystem::RequestHaltFlowExecution(const UFlowAsset& FlowAssetInstance, const FGuid& NodeGuid)
{
	bHaltFlowExecution = true;
	HaltedOnFlowAssetInstance = &FlowAssetInstance;
	HaltedOnNodeGuid = NodeGuid;
}

void UFlowDebuggerSubsystem::ClearHaltFlowExecution()
{
	bHaltFlowExecution = false;
	HaltedOnFlowAssetInstance.Reset();
	HaltedOnNodeGuid.Invalidate();
}

void UFlowDebuggerSubsystem::ClearLastHitBreakpoint()
{
	if (!LastHitNodeGuid.IsValid())
	{
		return;
	}

	// Pin breakpoint "hit" state lives in the PinBreakpoints map, node breakpoint "hit" lives on NodeBreakpoint.Breakpoint.
	if (!LastHitPinName.IsNone())
	{
		if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(LastHitNodeGuid, LastHitPinName))
		{
			PinBreakpoint->MarkAsHit(false);
		}
	}
	else
	{
		if (FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(LastHitNodeGuid))
		{
			NodeBreakpoint->MarkAsHit(false);
		}
	}

	LastHitNodeGuid.Invalidate();
	LastHitPinName = NAME_None;
}

void UFlowDebuggerSubsystem::MarkAsHit(const UFlowAsset& FlowAsset, const FGuid& NodeGuid)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
	{
		if (FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(Node.Key))
		{
			if (NodeBreakpoint->Breakpoint.IsActive() && NodeBreakpoint->Breakpoint.IsEnabled())
			{
				return true;
			}

			for (auto& [Name, PinBreakpoint] : NodeBreakpoint->PinBreakpoints)
			{
				if (PinBreakpoint.IsEnabled())
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UFlowDebuggerSubsystem::HasAnyBreakpointsDisabled(const TWeakObjectPtr<UFlowAsset> FlowAsset)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
	{
		if (FNodeBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(Node.Key))
		{
			if (NodeBreakpoint->Breakpoint.IsActive() && !NodeBreakpoint->Breakpoint.IsEnabled())
			{
				return true;
			}

			for (auto& [Name, PinBreakpoint] : NodeBreakpoint->PinBreakpoints)
			{
				if (!PinBreakpoint.IsEnabled())
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UFlowDebuggerSubsystem::TryMarkAsHit(const UFlowNode* Node)
{
	if (FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(Node->NodeGuid))
	{
		if (NodeBreakpoint->IsEnabled())
		{
			// Ensure only one breakpoint location is "hit" at a time.
			ClearLastHitBreakpoint();

			NodeBreakpoint->MarkAsHit(true);

			LastHitNodeGuid = NodeGuid;
			LastHitPinName = NAME_None;

			RequestHaltFlowExecution(FlowAsset, NodeGuid);

			OnDebuggerBreakpointHit.Broadcast(FlowAsset, NodeGuid);

			PauseSession(FlowAsset);
		}
	}

	return false;
}

void UFlowDebuggerSubsystem::MarkAsHit(const UFlowAsset& FlowAsset, const FGuid& NodeGuid, const FName& PinName)
{
	if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Node->NodeGuid, PinName))
	{
		if (PinBreakpoint->IsEnabled())
		{
			// Ensure only one breakpoint location is "hit" at a time.
			ClearLastHitBreakpoint();

			PinBreakpoint->MarkAsHit(true);

			LastHitNodeGuid = NodeGuid;
			LastHitPinName = PinName;

			RequestHaltFlowExecution(FlowAsset, NodeGuid);

			OnDebuggerBreakpointHit.Broadcast(FlowAsset, NodeGuid);

			PauseSession(FlowAsset);
		}
	}

	return false;
}

void UFlowDebuggerSubsystem::PauseSession(const UFlowAsset& FlowAsset)
{
	SetPause(FlowAsset, true);
}

void UFlowDebuggerSubsystem::ResumeSession(const UFlowAsset& FlowAsset)
{
	SetPause(FlowAsset, false);
}

void UFlowDebuggerSubsystem::SetPause(const UFlowAsset& FlowAsset, const bool bPause)
{
	// Default bWasPaused to opposite of bPause
	// (which we hope to get a better measure if we can get access to what we need)
	bool bWasPaused = !bPause;

	AGameModeBase* GameMode = nullptr;
	APlayerController* PlayerController = nullptr;

	const UWorld* World = FlowAsset.GetWorld();
	if (IsValid(World))
	{
		GameMode = World->GetAuthGameMode();

		if (IsValid(GameMode))
		{
			bWasPaused = GameMode->IsPaused();
		}

		const UGameInstance* GameInstance = World->GetGameInstance();
		if (IsValid(GameInstance))
		{
			PlayerController = GameInstance->GetFirstLocalPlayerController();
		}
	}

	if (bWasPaused != bPause)
	{
		if (bPause)
		{
			// Pausing (from an unpaused state)

			if (IsValid(PlayerController))
			{
				if (IsValid(GameMode))
				{
					GameMode->SetPause(PlayerController);
				}

				if (AWorldSettings* WorldSettings = PlayerController->GetWorldSettings())
				{
					WorldSettings->ForceNetUpdate();
				}
			}

			// Broadcast the Pause event
			OnDebuggerPaused.Broadcast(FlowAsset);
		}
		else
		{
			// Resuming (from a paused state)

			ClearHaltFlowExecution();

			// Replay any Flow propagation that was deferred while execution was halted.
			FFlowExecutionGate::FlushDeferredTriggerInputs();

			// Intentionally do NOT clear hit flags here. The editor-specific resume path will clear the last-hit
			// breakpoint safely (without racing against immediate breakpoint hits during flush).
			if (IsValid(GameMode))
			{
				(void) GameMode->ClearPause();
			}

			// Broadcast the Resume event
			OnDebuggerResumed.Broadcast(FlowAsset);
		}
	}
}

void UFlowDebuggerSubsystem::ClearHitBreakpoints()
{
	bPausedAtFlowBreakpoint = false;

	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	for (TPair<FGuid, FNodeBreakpoint>& NodeBreakpoint : Settings->NodeBreakpoints)
	{
		NodeBreakpoint.Value.Breakpoint.MarkAsHit(false);

		for (TPair<FName, FFlowBreakpoint>& PinBreakpoint : NodeBreakpoint.Value.PinBreakpoints)
		{
			PinBreakpoint.Value.MarkAsHit(false);
		}
	}

	LastHitNodeGuid.Invalidate();
	LastHitPinName = NAME_None;
}

bool UFlowDebuggerSubsystem::IsBreakpointHit(const FGuid& NodeGuid)
{
	if (const FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(NodeGuid))
	{
		return NodeBreakpoint->IsHit();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsBreakpointHit(const FGuid& NodeGuid, const FName& PinName)
{
	if (const FFlowBreakpoint* PinBreakpoint = FindBreakpoint(NodeGuid, PinName))
	{
		return PinBreakpoint->IsHit();
	}

	return false;
}

void UFlowDebuggerSubsystem::SaveSettings()
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	Settings->SaveConfig();
}