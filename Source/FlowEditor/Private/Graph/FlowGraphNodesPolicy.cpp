// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphNodesPolicy.h"
#include "Nodes/FlowNodeBase.h"
#include "Graph/FlowGraphSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNodesPolicy)

#if WITH_EDITOR
EFlowGraphPolicyResult FFlowGraphNodesPolicy::IsNodeAllowedByPolicy(const UFlowNodeBase* FlowNodeBase) const
{
	if (!IsValid(FlowNodeBase))
	{
		return EFlowGraphPolicyResult::TentativeForbidden;
	}

	const FString NodeCategoryString = UFlowGraphSettings::GetNodeCategoryForNode(*FlowNodeBase);

	const bool bIsInAllowedCategory = !AllowedCategories.IsEmpty() && IsAnySubcategory(NodeCategoryString, AllowedCategories);
	if (bIsInAllowedCategory)
	{
		return EFlowGraphPolicyResult::Allowed;
	}

	const bool bIsInDisallowedCategory = !DisallowedCategories.IsEmpty() && IsAnySubcategory(NodeCategoryString, DisallowedCategories);
	if (bIsInDisallowedCategory)
	{
		return EFlowGraphPolicyResult::Forbidden;
	}

	if (AllowedCategories.IsEmpty())
	{
		// If the AllowedCategories is empty, then we consider any node that isn't disallowed, as allowed
		return EFlowGraphPolicyResult::TentativeAllowed;
	}
	else
	{
		return EFlowGraphPolicyResult::TentativeForbidden;
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