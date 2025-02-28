// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Debugger/FlowDebuggerSubsystem.h"
#include "Debugger/FlowDebuggerSettings.h"

#include "Editor/UnrealEdEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDebuggerSubsystem)

#define LOCTEXT_NAMESPACE "FlowDebuggerSubsystem"

UFlowDebuggerSubsystem::UFlowDebuggerSubsystem()
{
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

void UFlowDebuggerSubsystem::AddBreakpoint(const UEdGraphNode* Node)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	FFlowBreakpoint& NodeBreakpoint = Settings->NodeBreakpoints.FindOrAdd(Node->NodeGuid);

	NodeBreakpoint.SetEnabled(true);
	SaveSettings();
}

void UFlowDebuggerSubsystem::AddBreakpoint(const UEdGraphPin* Pin)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	FFlowBreakpoint& PinBreakpoint = Settings->PinBreakpoints.FindOrAdd(Pin->PinId);

	PinBreakpoint.SetEnabled(true);
	SaveSettings();
}

void UFlowDebuggerSubsystem::RemoveAllBreakpoints(const UEdGraphNode* Node)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();

	if (Settings->NodeBreakpoints.Contains(Node->NodeGuid))
	{
		Settings->NodeBreakpoints.Remove(Node->NodeGuid);
		SaveSettings();
	}

	for (const UEdGraphPin* Pin : Node->Pins)
	{
		if (Settings->PinBreakpoints.Contains(Pin->PinId))
		{
			Settings->PinBreakpoints.Remove(Pin->PinId);
			SaveSettings();
		}
	}
}

void UFlowDebuggerSubsystem::RemoveNodeBreakpoint(const UEdGraphNode* Node)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	if (Settings->NodeBreakpoints.Contains(Node->NodeGuid))
	{
		Settings->NodeBreakpoints.Remove(Node->NodeGuid);
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::RemovePinBreakpoint(const UEdGraphPin* Pin)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	if (Settings->PinBreakpoints.Contains(Pin->PinId))
	{
		Settings->PinBreakpoints.Remove(Pin->PinId);
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::RemoveObsoletePinBreakpoints(const UEdGraphNode* Node)
{
	// UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	// if (FFlowBreakpoint* NodeBreakpoint = Settings->NodeBreakpoints.Find(Node->NodeGuid))
	// {
	// 	TSet<FGuid> PinGuids;
	// 	PinGuids.Reserve(Node->Pins.Num());
	// 	for (const UEdGraphPin* Pin : Node->Pins)
	// 	{
	// 		PinGuids.Emplace(Pin->PinId);
	// 	}
	//
	// 	Settings->PinBreakpoints.RemoveAllSwap([PinGuids](const FFlowBreakpoint& PinBreakpoint)
	// 	{
	// 		return !PinGuids.Contains(PinBreakpoint.GetPinId());
	// 	});
	//
	// 	// if all settings data is default, we can remove it from the map
	// 	if (*NodeBreakpoint == FFlowBreakpoint())
	// 	{
	// 		Settings->NodeBreakpoints.Remove(Node->NodeGuid);
	// 	}
	//
	// 	SaveSettings();
	// }
}

void UFlowDebuggerSubsystem::ToggleBreakpoint(const UEdGraphNode* Node)
{
	if (FindBreakpoint(Node) == nullptr)
	{
		AddBreakpoint(Node);
	}
	else
	{
		RemoveNodeBreakpoint(Node);
	}
}

void UFlowDebuggerSubsystem::ToggleBreakpoint(const UEdGraphPin* Pin)
{
	if (FindBreakpoint(Pin) == nullptr)
	{
		AddBreakpoint(Pin);
	}
	else
	{
		RemovePinBreakpoint(Pin);
	}
}

FFlowBreakpoint* UFlowDebuggerSubsystem::FindBreakpoint(const UEdGraphNode* Node)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	return Settings->NodeBreakpoints.Find(Node->NodeGuid);
}

FFlowBreakpoint* UFlowDebuggerSubsystem::FindBreakpoint(const UEdGraphPin* Pin)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	return Settings->PinBreakpoints.Find(Pin->PinId);
}

void UFlowDebuggerSubsystem::SetBreakpointEnabled(const UEdGraphNode* Node, const bool bEnabled)
{
	if (FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(Node))
	{
		NodeBreakpoint->SetEnabled(bEnabled);
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::SetBreakpointEnabled(const UEdGraphPin* Pin, const bool bEnabled)
{
	if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Pin))
	{
		PinBreakpoint->SetEnabled(bEnabled);
		SaveSettings();
	}
}

bool UFlowDebuggerSubsystem::IsBreakpointEnabled(const UEdGraphNode* Node)
{
	if (const FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Node))
	{
		return PinBreakpoint->IsEnabled();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsBreakpointEnabled(const UEdGraphPin* Pin)
{
	if (const FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Pin))
	{
		return PinBreakpoint->IsEnabled();
	}

	return false;
}

bool UFlowDebuggerSubsystem::MarkAsHit(const UEdGraphNode* Node)
{
	if (FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(Node))
	{
		NodeBreakpoint->MarkAsHit(true);
		return true;
	}

	return false;
}

bool UFlowDebuggerSubsystem::MarkAsHit(const UEdGraphPin* Pin)
{
	if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Pin))
	{
		PinBreakpoint->MarkAsHit(true);
		return true;
	}

	return false;
}

void UFlowDebuggerSubsystem::ResetHit(const UEdGraphNode* Node)
{
	if (FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(Node))
	{
		NodeBreakpoint->MarkAsHit(false);
	}
}

void UFlowDebuggerSubsystem::ResetHit(const UEdGraphPin* Pin)
{
	if (FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Pin))
	{
		PinBreakpoint->MarkAsHit(false);
	}
}

bool UFlowDebuggerSubsystem::IsBreakpointHit(const UEdGraphNode* Node)
{
	if (const FFlowBreakpoint* NodeBreakpoint = FindBreakpoint(Node))
	{
		return NodeBreakpoint->IsHit();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsBreakpointHit(const UEdGraphPin* Pin)
{
	if (const FFlowBreakpoint* PinBreakpoint = FindBreakpoint(Pin))
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

#undef LOCTEXT_NAMESPACE
