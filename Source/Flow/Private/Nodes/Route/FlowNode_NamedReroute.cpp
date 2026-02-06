// Fill out your copyright notice in the Description page of Project Settings.


#include "Nodes/Route/FlowNode_NamedReroute.h"

#include "FlowAsset.h"

UFlowNode_NamedRerouteBase::UFlowNode_NamedRerouteBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
#endif

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

class UFlowNode_NamedRerouteDeclaration* UFlowNode_NamedRerouteBase::FindDeclarationInGraph(
	const FGuid& VariableGuid) const
{
	const auto FlowAsset = GetFlowAsset();
	check(FlowAsset);
	return FindDeclarationInArray(VariableGuid, FlowAsset->GetAllNodes());
}

void UFlowNode_NamedRerouteDeclaration::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

void UFlowNode_NamedRerouteDeclaration::PostInitProperties()
{
	Super::PostInitProperties();
	// Init the GUID
	UpdateVariableGuid(false, false);
}

void UFlowNode_NamedRerouteDeclaration::PostLoad()
{
	Super::PostLoad();
	// Init the GUID
	UpdateVariableGuid(false, false);
}

void UFlowNode_NamedRerouteDeclaration::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	
	UpdateVariableGuid(false, true);
}

#if WITH_EDITOR
void UFlowNode_NamedRerouteDeclaration::SetEditableName(const FString& NewName)
{
	VariableName = *NewName;
	MakeNameUnique();
}

void UFlowNode_NamedRerouteDeclaration::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, VariableName))
	{
		MakeNameUnique();
	}
}

void UFlowNode_NamedRerouteDeclaration::PostPasteNode(const TArray<UFlowNode*>& PastedNodes)
{
	Super::PostPasteNode(PastedNodes);

	// Only force regeneration of Guid if there's already a variable with the same one
	if (FindDeclarationInGraph(VariableGuid))
	{
		// Update Guid, and update the copied usages accordingly
		FGuid OldGuid = VariableGuid;
		UpdateVariableGuid(true, true);
		for (UFlowNode* PastedNode : PastedNodes)
		{
			auto* Usage = Cast<UFlowNode_NamedRerouteUsage>(PastedNode);
			if (Usage && Usage->DeclarationVariableGuid == OldGuid)
			{
				Usage->Declaration = this;
				Usage->DeclarationVariableGuid = VariableGuid;
			}
		}

		// Find a new name
		MakeNameUnique();
	}
	else
	{
		// If there's no existing variable with this GUID, only create it if needed
		UpdateVariableGuid(false, true);
	}
}

FText UFlowNode_NamedRerouteDeclaration::GetNodeToolTip() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return Super::GetNodeTitle();
	}
	return FText::FromString(FString::Printf(TEXT("Declaration of [ %s ]"), *GetVariableName().ToString()));
}

FText UFlowNode_NamedRerouteDeclaration::GetNodeTitle() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return Super::GetNodeTitle();
	}
	return FText::FromString(GetVariableName().ToString());
}

void UFlowNode_NamedRerouteDeclaration::MakeNameUnique()
{
	if (const auto* FlowAsset = GetFlowAsset())
	{
		auto Nodes = FlowAsset->GetAllNodes();

		int32 NameIndex = 1;
		bool bResultNameIndexValid = true;
		FName PotentialName;

		// Find an available unique name
		do
		{
			PotentialName = VariableName;
			if (NameIndex != 1)
			{
				PotentialName.SetNumber(NameIndex);
			}

			bResultNameIndexValid = true;
			for (UFlowNode* Node : Nodes)
			{
				auto* OtherDeclaration = Cast<UFlowNode_NamedRerouteDeclaration>(Node);
				if (OtherDeclaration && OtherDeclaration != this && OtherDeclaration->VariableName == PotentialName)
				{
					bResultNameIndexValid = false;
					break;
				}
			}

			NameIndex++;
		} while (!bResultNameIndexValid);

		VariableName = PotentialName;
	}
}
#endif

void UFlowNode_NamedRerouteDeclaration::UpdateVariableGuid(bool bForceGeneration, bool bAllowMarkingPackageDirty)
{
	// If we are in the editor, and we don't have a valid GUID yet, generate one.
	if (GIsEditor && !FApp::IsGame())
	{
		if (bForceGeneration || !VariableGuid.IsValid())
		{
			VariableGuid = FGuid::NewGuid();

			if (bAllowMarkingPackageDirty)
			{
				MarkPackageDirty();
			}
		}
	}
}

UFlowNode_NamedRerouteUsage::UFlowNode_NamedRerouteUsage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins.Empty();
}

bool UFlowNode_NamedRerouteUsage::IsDeclarationValid() const
{
	// Deleted nodes are marked as pending kill (see SFlowGraphEditor::DeleteSelectedNodes())
	return IsValid(Declaration);
}

void UFlowNode_NamedRerouteUsage::ExecuteInput(const FName& PinName)
{
	Finish();
	if (IsDeclarationValid())
	{
		GetFlowAsset()->TriggerInput(Declaration->GetGuid(), UFlowNode::DefaultInputPin.PinName, FConnectedPin(GetGuid(), UFlowNode::DefaultOutputPin.PinName));
	}
}

#if WITH_EDITOR
void UFlowNode_NamedRerouteUsage::PostPasteNode(const TArray<UFlowNode*>& PastedNodes)
{
	Super::PostPasteNode(PastedNodes);

	// First try to find the declaration in the copied nodes
	Declaration = FindDeclarationInArray(DeclarationVariableGuid, PastedNodes);
	if (!IsValid(Declaration))
	{
		// If unsuccessful, try to find it in the whole graph
		Declaration = FindDeclarationInGraph(DeclarationVariableGuid);
	}

	// Keep GUID in sync. In case this is pasted by itself into another graph, we don't want this node to connect up to a previously connected declaration. 
	if (IsValid(Declaration))
	{
		DeclarationVariableGuid = Declaration->GetVariableGuid();
	}

	// Save that Declaration change
	MarkPackageDirty();
}

FText UFlowNode_NamedRerouteUsage::GetNodeToolTip() const
{
	FString NodeTitle = IsDeclarationValid() ? Declaration->GetVariableName().ToString() : TEXT("Invalid named reroute");
	return FText::FromString(FString::Printf(TEXT("Usage of [ %s ]"), *NodeTitle));
}

FText UFlowNode_NamedRerouteUsage::GetNodeTitle() const
{
	FString NodeTitle = IsDeclarationValid() ? Declaration->GetVariableName().ToString() : TEXT("Invalid named reroute");
	return FText::FromString(NodeTitle);
}

EDataValidationResult UFlowNode_NamedRerouteUsage::ValidateNode()
{
	if (Super::ValidateNode() == EDataValidationResult::Invalid)
	{
		return EDataValidationResult::Invalid;
	}

	if (!IsDeclarationValid())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Invalid named reroute usage"), this);
		return EDataValidationResult::Invalid;
	}

	if (Declaration->GetFlowAsset() != GetFlowAsset())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Named reroute usage has a different flow asset than the declaration"), this);
		return EDataValidationResult::Invalid;
	}

	if (Declaration->GatherConnectedNodes().Contains(this))
	{
		ValidationLog.Error<UFlowNode>(TEXT("Loop detected in named reroute"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
