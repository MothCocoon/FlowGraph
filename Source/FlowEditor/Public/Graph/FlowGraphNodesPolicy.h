// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowGraphNodesPolicy.generated.h"

class UFlowNodeBase;

USTRUCT()
struct FFlowGraphNodesPolicy
{
	GENERATED_BODY();

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TArray<FString> AllowedCategories;

	UPROPERTY(Config, EditAnywhere, Category = "Nodes")
	TArray<FString> DisallowedCategories;
#endif

#if WITH_EDITOR
public:
	bool IsNodeAllowedByPolicy(const UFlowNodeBase* FlowNodeBase) const;

protected:
	static bool IsAnySubcategory(const FString& CheckCategory, const TArray<FString>& Categories);
#endif
};
