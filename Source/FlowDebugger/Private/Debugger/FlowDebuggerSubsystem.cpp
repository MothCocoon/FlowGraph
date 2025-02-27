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

void UFlowDebuggerSubsystem::CreateTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type, bool bEnabled)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	check(Settings);
	FFlowTraitSettings& TraitSettings = Settings->PerNodeTraits.FindOrAdd(OwnerNode->NodeGuid);

	// ensure that this node doesn't already contain a trait with provided type
	checkSlow(!TraitSettings.NodeTraits.ContainsByPredicate([Type](const FFlowDebugTrait& Trait)
		{
		return Trait.Type == Type;
		}));

	TraitSettings.NodeTraits.Emplace(Type, bEnabled);
	SaveSettings();
}

void UFlowDebuggerSubsystem::CreateTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bEnabled)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	check(Settings);
	FFlowTraitSettings& TraitSettings = Settings->PerNodeTraits.FindOrAdd(OwnerPin->GetOwningNode()->NodeGuid);

	// ensure that this pin doesn't already contain a trait with provided type
	checkSlow(!TraitSettings.PinTraits.ContainsByPredicate([OwnerPin, Type](const FFlowDebugTrait& Trait)
		{
		return Trait.PinId == OwnerPin->PinId && Trait.Type == Type;
		}));

	TraitSettings.PinTraits.Emplace(Type, OwnerPin->PinId, bEnabled);
	SaveSettings();
}

void UFlowDebuggerSubsystem::RemoveTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type)
{
	RemoveNodeTraitByPredicate(OwnerNode, [Type](const FFlowDebugTrait& Trait)
	{
		return Trait.Type == Type;
	});
}

void UFlowDebuggerSubsystem::RemoveTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	RemovePinTraitByPredicate(OwnerPin, [OwnerPin, Type](const FFlowDebugTrait& Trait)
	{
		return Trait.PinId == OwnerPin->PinId && Trait.Type == Type;
	});
}

void UFlowDebuggerSubsystem::RemoveNodeTraitByPredicate(const UEdGraphNode* OwnerNode, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate)
{
	if (TArray<FFlowDebugTrait>* Traits = GetNodeTraits(OwnerNode))
	{
		if (Traits->RemoveAllSwap(Predicate, EAllowShrinking::No))
		{
			if (Traits->IsEmpty())
			{
				ClearNodeTraits(OwnerNode);
			}
			SaveSettings();
		}
	}
}

void UFlowDebuggerSubsystem::RemovePinTraitByPredicate(const UEdGraphNode* OwnerNode, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate)
{
	if (TArray<FFlowDebugTrait>* Traits = GetPinTraits(OwnerNode))
	{
		if (Traits->RemoveAllSwap(Predicate, EAllowShrinking::No))
		{
			if (Traits->IsEmpty())
			{
				ClearPinTraits(OwnerNode);
			}
			SaveSettings();
		}
	}
}

void UFlowDebuggerSubsystem::RemovePinTraitByPredicate(const UEdGraphPin* OwnerPin, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate)
{
	const UEdGraphNode* OwnerNode = Cast<UEdGraphNode>(OwnerPin->GetOwningNode());
	check(OwnerNode);

	if (TArray<FFlowDebugTrait>* Traits = GetPinTraits(OwnerNode))
	{
		if (Traits->RemoveAllSwap(Predicate, EAllowShrinking::No))
		{
			if (Traits->IsEmpty())
			{
				ClearPinTraits(OwnerPin);
			}
			SaveSettings();
		}
	}
}

void UFlowDebuggerSubsystem::ClearNodeTraits(const UEdGraphNode* OwnerNode)
{
	if (FFlowTraitSettings* TraitSettings = GetPerNodeSettings(OwnerNode))
	{
		TraitSettings->NodeTraits.Empty();

		// if all settings data is default, we can remove it from the map
		if (*TraitSettings == FFlowTraitSettings())
		{
			UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
			Settings->PerNodeTraits.Remove(OwnerNode->NodeGuid);
		}

		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::ClearPinTraits(const UEdGraphNode* OwnerNode)
{
	if (FFlowTraitSettings* TraitSettings = GetPerNodeSettings(OwnerNode))
	{
		TraitSettings->PinTraits.Empty();

		// if all settings data is default, we can remove it from the map
		if (*TraitSettings == FFlowTraitSettings())
		{
			UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
			Settings->PerNodeTraits.Remove(OwnerNode->NodeGuid);
		}

		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::ClearPinTraits(const UEdGraphPin* OwnerPin)
{
	UEdGraphNode* OwnerNode = Cast<UEdGraphNode>(OwnerPin->GetOwningNode());
	check(OwnerNode);

	if (FFlowTraitSettings* TraitSettings = GetPerNodeSettings(OwnerNode))
	{
		TraitSettings->PinTraits.RemoveAllSwap([OwnerPin](const FFlowDebugTrait& Trait)
		{
			return Trait.PinId == OwnerPin->PinId;
		});

		// if all settings data is default, we can remove it from the map
		if (*TraitSettings == FFlowTraitSettings())
		{
			UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
			Settings->PerNodeTraits.Remove(OwnerNode->NodeGuid);
		}

		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::CleanupTraits(const UEdGraphNode* OwnerNode)
{
	RemovePinTraitByPredicate(OwnerNode, [OwnerNode](const FFlowDebugTrait& Trait)
	{
		auto Predicate = [Trait](const UEdGraphPin* Pin)
		{
			return Pin->PinId == Trait.PinId;
		};
		return OwnerNode->Pins.ContainsByPredicate(Predicate) == false;
	});
}

FFlowDebugTrait* UFlowDebuggerSubsystem::FindTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (TArray<FFlowDebugTrait>* Traits = GetNodeTraits(OwnerNode))
	{
		for (FFlowDebugTrait& Trait : *Traits)
		{
			if (Trait.Type == Type)
			{
				return &Trait;
			}
		}
	}

	return nullptr;
}

FFlowDebugTrait* UFlowDebuggerSubsystem::FindTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	const UEdGraphNode* OwnerNode = Cast<UEdGraphNode>(OwnerPin->GetOwningNode());
	check(OwnerNode);

	if (TArray<FFlowDebugTrait>* Traits = GetPinTraits(OwnerNode))
	{
		for (FFlowDebugTrait& Trait : *Traits)
		{
			if (Trait.PinId == OwnerPin->PinId && Trait.Type == Type)
			{
				return &Trait;
			}
		}
	}

	return nullptr;
}

void UFlowDebuggerSubsystem::SetTraitEnabled(const UEdGraphNode* OwnerNode, EFlowTraitType Type, bool bIsEnabled)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		Trait->bEnabled = bIsEnabled;
		SaveSettings();
	}
}

void UFlowDebuggerSubsystem::SetTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bIsEnabled)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		Trait->bEnabled = bIsEnabled;
		SaveSettings();
	}
}

bool UFlowDebuggerSubsystem::IsTraitEnabled(const UEdGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		return Trait->IsEnabled();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		return Trait->IsEnabled();
	}

	return false;
}

void UFlowDebuggerSubsystem::ToggleTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (FindTrait(OwnerNode, Type))
	{
		RemoveTrait(OwnerNode, Type);
	}
	else
	{
		CreateTrait(OwnerNode, Type, true);
	}
}

void UFlowDebuggerSubsystem::ToggleTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	if (FindTrait(OwnerPin, Type))
	{
		RemoveTrait(OwnerPin, Type);
	}
	else
	{
		CreateTrait(OwnerPin, Type, true);
	}
}

TArray<EFlowTraitType> UFlowDebuggerSubsystem::SetAllTraitsHit(const UEdGraphNode* OwnerNode, bool bHit)
{
	TArray<EFlowTraitType> HitTraitTypes;
	for (EFlowTraitType Type : TEnumRange<EFlowTraitType>())
	{
		if (SetTraitHit(OwnerNode, Type, bHit))
		{
			HitTraitTypes.Add(Type);
		}
	}

	return HitTraitTypes;
}

TArray<EFlowTraitType> UFlowDebuggerSubsystem::SetAllTraitsHit(const UEdGraphPin* OwnerPin, bool bHit)
{
	TArray<EFlowTraitType> HitTraitTypes;
	for (EFlowTraitType Type : TEnumRange<EFlowTraitType>())
	{
		if (SetTraitHit(OwnerPin, Type, bHit))
		{
			HitTraitTypes.Add(Type);
		}
	}

	return HitTraitTypes;
}


bool UFlowDebuggerSubsystem::SetTraitHit(const UEdGraphNode* OwnerNode, EFlowTraitType Type, bool bHit)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		Trait->bHit = bHit;
		return true;
	}

	return false;
}

bool UFlowDebuggerSubsystem::SetTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bHit)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		Trait->bHit = bHit;
		return true;
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsTraitHit(const UEdGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		return Trait->IsHit();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		return Trait->IsHit();
	}

	return false;
}

FFlowTraitSettings* UFlowDebuggerSubsystem::GetPerNodeSettings(const UEdGraphNode* OwnerNode)
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	return Settings->PerNodeTraits.Find(OwnerNode->NodeGuid);
}

TArray<FFlowDebugTrait>* UFlowDebuggerSubsystem::GetNodeTraits(const UEdGraphNode* OwnerNode)
{
	FFlowTraitSettings* Settings = GetPerNodeSettings(OwnerNode);

	// return nullptr if there's no node traits associated w/ this flow node
	return (!Settings || Settings->NodeTraits.IsEmpty()) ? nullptr : &Settings->NodeTraits;
}

TArray<FFlowDebugTrait>* UFlowDebuggerSubsystem::GetPinTraits(const UEdGraphNode* OwnerNode)
{
	FFlowTraitSettings* Settings = GetPerNodeSettings(OwnerNode);

	// return nullptr if there's no pin traits associated w/ this flow node
	return (!Settings || Settings->PinTraits.IsEmpty()) ? nullptr : &Settings->PinTraits;
}

void UFlowDebuggerSubsystem::SaveSettings()
{
	UFlowDebuggerSettings* Settings = GetMutableDefault<UFlowDebuggerSettings>();
	Settings->SaveConfig();
}

#undef LOCTEXT_NAMESPACE
