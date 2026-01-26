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
	AssetTemplate->OnPinTriggered.BindUObject(this, &ThisClass::OnPinTriggered);
}

void UFlowDebuggerSubsystem::OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate) const
{
	AssetTemplate->OnPinTriggered.Unbind();
}

void UFlowDebuggerSubsystem::OnPinTriggered(const UFlowNode* Node, const FName& PinName)
{
	if (bPausedAtFlowBreakpoint)
	{
		return;
	}

	if (!TryMarkAsHit(Node, PinName))
	{
		// Node breakpoints waits on any pin triggered, but check it only if there is no hit pin breakpoint
		TryMarkAsHit(Node);
	}
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

		for (TPair<FName, FFlowBreakpoint>& PinBreakpoint : NodeBreakpoint->PinBreakpoints)
		{
			if (!PinNames.Contains(PinBreakpoint.Key))
			{
				NodeBreakpoint->PinBreakpoints.Remove(PinBreakpoint.Key);
				bAnythingRemoved = true;
			}
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

bool UFlowDebuggerSubsystem::HasAnyBreakpointsEnabled(const TWeakObjectPtr<UFlowAsset> FlowAsset)
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
			NodeBreakpoint->MarkAsHit(true);
			PauseSession(Node);
			return true;
		}
	}

	return false;
}

bool UFlowDebuggerSubsystem::TryMarkAsHit(const UFlowNode* Node, const FName& PinName)
{
	if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Node->NodeGuid, PinName))
	{
		if (PinBreakpoint->IsEnabled())
		{
			PinBreakpoint->MarkAsHit(true);
			PauseSession(Node);
			return true;
		}
	}

	return false;
}

void UFlowDebuggerSubsystem::PauseSession(const UFlowNode* Node)
{
	SetPause(true);
}

void UFlowDebuggerSubsystem::ResumeSession()
{
	SetPause(false);
}

void UFlowDebuggerSubsystem::SetPause(const bool bPause)
{
	// experimental implementation, untested, shows intent for future development
	// here be dragons: same as APlayerController::SetPause, but we allow debugger to pause on clients
	if (const UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController())
			{
				if (AGameModeBase* const GameMode = GetWorld()->GetAuthGameMode())
				{
					const bool bCurrentPauseState = PlayerController->IsPaused();
					if (bPause && !bCurrentPauseState)
					{
						GameMode->SetPause(PlayerController);

						if (AWorldSettings* WorldSettings = PlayerController->GetWorldSettings())
						{
							WorldSettings->ForceNetUpdate();
						}
					}
					else if (!bPause && bCurrentPauseState)
					{
						if (GameMode->ClearPause())
						{
							ClearHitBreakpoints();
						}
					}
				}
			}
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
