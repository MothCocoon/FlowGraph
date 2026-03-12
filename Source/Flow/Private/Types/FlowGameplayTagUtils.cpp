// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowGameplayTagUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGameplayTagUtils)

bool FFlowGameplayTagRequirements::RequirementsMet(const FGameplayTagContainer& Container) const
{
	const bool bHasRequired = Container.HasAll(RequireTags);
	const bool bHasIgnored = Container.HasAny(IgnoreTags);
	const bool bMatchQuery = TagQuery.IsEmpty() || TagQuery.Matches(Container);

	return bHasRequired && !bHasIgnored && bMatchQuery;
}

bool FFlowGameplayTagRequirements::IsEmpty() const
{
	return (RequireTags.Num() == 0 && IgnoreTags.Num() == 0 && TagQuery.IsEmpty());
}

FString FFlowGameplayTagRequirements::ToString() const
{
	FString Str;

	if (RequireTags.Num() > 0)
	{
		Str += FString::Printf(TEXT("require: %s "), *RequireTags.ToStringSimple());
	}
	if (IgnoreTags.Num() > 0)
	{
		Str += FString::Printf(TEXT("ignore: %s "), *IgnoreTags.ToStringSimple());
	}
	if (!TagQuery.IsEmpty())
	{
		Str += TagQuery.GetDescription();
	}

	return Str;
}

bool FFlowGameplayTagRequirements::operator==(const FFlowGameplayTagRequirements& Other) const
{
	return RequireTags == Other.RequireTags && IgnoreTags == Other.IgnoreTags && TagQuery == Other.TagQuery;
}

bool FFlowGameplayTagRequirements::operator!=(const FFlowGameplayTagRequirements& Other) const
{
	return !(*this == Other);
}

FGameplayTagQuery FFlowGameplayTagRequirements::ConvertTagFieldsToTagQuery() const
{
	const bool bHasRequireTags = !RequireTags.IsEmpty();
	const bool bHasIgnoreTags = !IgnoreTags.IsEmpty();

	if (!bHasIgnoreTags && !bHasRequireTags)
	{
		return FGameplayTagQuery{};
	}

	// FGameplayTagContainer::RequirementsMet is HasAll(RequireTags) && !HasAny(IgnoreTags);
	FGameplayTagQueryExpression RequiredTagsQueryExpression = FGameplayTagQueryExpression().AllTagsMatch().AddTags(RequireTags);
	FGameplayTagQueryExpression IgnoreTagsQueryExpression = FGameplayTagQueryExpression().NoTagsMatch().AddTags(IgnoreTags);

	FGameplayTagQueryExpression RootQueryExpression;
	if (bHasRequireTags && bHasIgnoreTags)
	{
		RootQueryExpression = FGameplayTagQueryExpression().AllExprMatch().AddExpr(RequiredTagsQueryExpression).AddExpr(IgnoreTagsQueryExpression);
	}
	else if (bHasRequireTags)
	{
		RootQueryExpression = RequiredTagsQueryExpression;
	}
	else // bHasIgnoreTags
	{
		RootQueryExpression = IgnoreTagsQueryExpression;
	}

	// Build the expression
	return FGameplayTagQuery::BuildQuery(RootQueryExpression);
}
