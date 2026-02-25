// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_PredicateRequireGameplayTags.h"
#include "FlowLogChannels.h"
#include "Nodes/FlowNode.h"
#include "Logging/LogMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_PredicateRequireGameplayTags)

UFlowNodeAddOn_PredicateRequireGameplayTags::UFlowNodeAddOn_PredicateRequireGameplayTags()
	: Super()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn_Predicate;
	Category = TEXT("DataPins");
#endif
}

bool UFlowNodeAddOn_PredicateRequireGameplayTags::EvaluatePredicate_Implementation() const
{
	if (Requirements.IsEmpty())
	{
		// And Empty Requirements results in a "true" result
		return true;
	}

	FGameplayTagContainer TagsValue;
	
	// Sourcing the tags from the data pin
	if (!TryGetTagsToCheckFromDataPin(TagsValue))
	{
		return false;
	}

	// Execute the Tags vs the Requirements
	const bool bResult = Requirements.RequirementsMet(TagsValue);
	return bResult;
}

#if WITH_EDITOR
void UFlowNodeAddOn_PredicateRequireGameplayTags::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateNodeConfigText();
}

void UFlowNodeAddOn_PredicateRequireGameplayTags::OnEditorPinConnectionsChanged(const TArray<FFlowPinConnectionChange>& Changes)
{
	Super::OnEditorPinConnectionsChanged(Changes);

	UpdateNodeConfigText();
}
#endif

bool UFlowNodeAddOn_PredicateRequireGameplayTags::TryGetTagsToCheckFromDataPin(FGameplayTagContainer& TagsToCheckValue) const
{
	static const FName TagsName = GET_MEMBER_NAME_CHECKED(UFlowNodeAddOn_PredicateRequireGameplayTags, Tags);

	const EFlowDataPinResolveResult ResultEnum = TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(TagsName, TagsToCheckValue);

	if (FlowPinType::IsSuccess(ResultEnum))
	{
		return true;
	}
	else
	{
		UE_LOG(LogFlow, Error, TEXT("Cannot EvaluatePredicate on a data pin value we cannot resolve: %s"), *UEnum::GetDisplayValueAsText(ResultEnum).ToString());

		return false;
	}
}

void UFlowNodeAddOn_PredicateRequireGameplayTags::UpdateNodeConfigText_Implementation()
{
#if WITH_EDITOR
	const FName TagsName = GET_MEMBER_NAME_CHECKED(UFlowNodeAddOn_PredicateRequireGameplayTags, Tags);
	FTextBuilder TextBuilder;
	if (Requirements.IsEmpty())
	{
		const FName RequirementsName = GET_MEMBER_NAME_CHECKED(UFlowNodeAddOn_PredicateRequireGameplayTags, Requirements);
		TextBuilder.AppendLine(FString::Printf(TEXT("<not configured - Must have configured %s>"), *RequirementsName.ToString()));
	}
	else if (Tags.IsEmpty() && !GetFlowNode()->IsInputConnected(TagsName))
	{
		TextBuilder.AppendLine(FString::Printf(TEXT("<not configured - %s must have a default value or a connected pin>"), *TagsName.ToString()));
	}
	else
	{
		TextBuilder.AppendLine(Requirements.ToString());
	}

	SetNodeConfigText(TextBuilder.ToText());
#endif // WITH_EDITOR
}