// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_SwitchCase.h"
#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "AddOns/FlowNodeAddOn_PredicateOR.h"
#include "FlowSettings.h"
#include "Nodes/FlowNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_SwitchCase)

#define LOCTEXT_NAMESPACE "FlowNodeAddOn_SwitchCase"

const FName UFlowNodeAddOn_SwitchCase::DefaultCaseName = "Case";

UFlowNodeAddOn_SwitchCase::UFlowNodeAddOn_SwitchCase()
	: Super()
	, CaseName(DefaultCaseName)
	, OutputPinName(DefaultCaseName)
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn_SwitchCase;
	Category = TEXT("Switch");
#endif
}

#if WITH_EDITOR

void UFlowNodeAddOn_SwitchCase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, CaseName))
	{
		if (CaseName.IsNone())
		{
			CaseName = DefaultCaseName;
		}

		RequestReconstructionOnOwningFlowNode();
	}
}

TArray<FFlowPin> UFlowNodeAddOn_SwitchCase::GetContextOutputs() const
{
	const UFlowNode* FlowNodeOwner = GetFlowNode();
	check(IsValid(FlowNodeOwner));

	int32 DuplicateCount = 0;
	int32 DuplicateCountForThis = 0;
	int32 ThisIndex = INDEX_NONE;

	const TArray<UFlowNodeAddOn*>& OwnerAddOns = FlowNodeOwner->GetFlowNodeAddOnChildren();
	for (int32 Index = 0; Index < OwnerAddOns.Num(); ++Index)
	{
		const UFlowNodeAddOn_SwitchCase* SwitchCaseAddOn = Cast<UFlowNodeAddOn_SwitchCase>(OwnerAddOns[Index]);
		if (!IsValid(SwitchCaseAddOn))
		{
			continue;
		}

		const bool bIsThisAddOn = SwitchCaseAddOn == this;

		if (CaseName == SwitchCaseAddOn->CaseName)
		{
			++DuplicateCount;
		}

		if (bIsThisAddOn)
		{
			ThisIndex = Index;

			DuplicateCountForThis = DuplicateCount;
		}
	}

	check(ThisIndex != INDEX_NONE);

	if (DuplicateCount > 1)
	{
		OutputPinName = FName(FString::Printf(TEXT("%s (%d)"), *CaseName.ToString(), DuplicateCountForThis));
	}
	else
	{
		OutputPinName = FName(FString::Printf(TEXT("%s"), *CaseName.ToString()));
	}

	return { FFlowPin(OutputPinName) };
}
#endif

EFlowAddOnAcceptResult UFlowNodeAddOn_SwitchCase::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}
	else
	{
		// All AddOn children MUST implement IFlowPredicateInterface
		// (so do not return Super's implementation which will return Undetermined)
		return EFlowAddOnAcceptResult::Reject;
	}
}

bool UFlowNodeAddOn_SwitchCase::TryTriggerForCase_Implementation() const
{
	bool bResult = false;
	FLOW_ASSERT_ENUM_MAX(EFlowPredicateCombinationRule, 2);
	if (BranchCombinationRule == EFlowPredicateCombinationRule::AND)
	{
		bResult = UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(AddOns);
	}
	else
	{
		check(BranchCombinationRule == EFlowPredicateCombinationRule::OR);

		bResult = UFlowNodeAddOn_PredicateOR::EvaluatePredicateOR(AddOns);
	}

	if (bResult)
	{
		constexpr bool bFinish = true;
		GetFlowNode()->TriggerOutput(OutputPinName, bFinish);
	}

	return bResult;
}

FText UFlowNodeAddOn_SwitchCase::K2_GetNodeTitle_Implementation() const
{
	if (GetDefault<UFlowSettings>()->bUseAdaptiveNodeTitles)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowPredicateCombinationRule, 2);
		if (BranchCombinationRule != EFlowPredicateCombinationRule::AND)
		{
			return FText::Format(LOCTEXT("SwitchCaseTitle", "{0} ({1})"), { FText::FromName(OutputPinName), UEnum::GetDisplayValueAsText(BranchCombinationRule) });
		}
		else
		{
			return FText::Format(LOCTEXT("SwitchCaseTitle", "{0}"), { FText::FromName(OutputPinName) });
		}
	}

	return Super::K2_GetNodeTitle_Implementation();
}

#undef LOCTEXT_NAMESPACE