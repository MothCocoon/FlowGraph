// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphNodesPolicy.h"
#include "Nodes/FlowNodeBase.h"
#include "Graph/FlowGraphSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNodesPolicy)

#if WITH_EDITOR
bool FFlowGraphNodesPolicy::IsNodeAllowedByPolicy(const UFlowNodeBase* FlowNodeBase) const
{
	if (!IsValid(FlowNodeBase))
	{
		return false;
	}

	const FString NodeCategoryString = UFlowGraphSettings::GetNodeCategoryForNode(*FlowNodeBase);

	const bool bIsInAllowedCategory = !AllowedCategories.IsEmpty() && IsAnySubcategory(NodeCategoryString, AllowedCategories);
	if (bIsInAllowedCategory)
	{
		return true;
	}

	const bool bIsInDisallowedCategory = !DisallowedCategories.IsEmpty() && IsAnySubcategory(NodeCategoryString, DisallowedCategories);
	if (bIsInDisallowedCategory)
	{
		return false;
	}

	if (AllowedCategories.IsEmpty())
	{
		// If the AllowedCategories is empty, then we consider any node that isn't disallowed, as allowed
		return true;
	}
	else
	{
		return false;
	}
}

bool FFlowGraphNodesPolicy::IsAnySubcategory(const FString& CheckCategory, const TArray<FString>& Categories)
{
	for (const FString& Category : Categories)
	{
		if (CheckCategory.StartsWith(Category, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}
#endif