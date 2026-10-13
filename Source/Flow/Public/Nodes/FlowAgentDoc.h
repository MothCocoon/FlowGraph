// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"

#include "FlowAgentDoc.generated.h"

/**
 * Hand-authored, agent-facing documentation for one Flow node or addon class.
 *
 * Complements the reflected facts a catalog query can already derive from the CDO (category,
 * properties, pins, origin, deprecation) with the one thing reflection can never supply: when to
 * reach for this class. Read through UFlowNodeBase::GetAgentDoc(), which lets a class store its doc
 * either as a serialized CDO default (Blueprint) or as a compiled-in constant (native).
 *
 * Guidance and Tags are hand-authored. Articles is written by tooling and should not be maintained by hand.
 */
USTRUCT(BlueprintType)
struct FLOW_API FFlowAgentDoc
{
	GENERATED_BODY()

	/* When to use, when NOT to use, and gotchas. Prose or short bullets. */
	UPROPERTY(EditDefaultsOnly, Category = "AgentDoc", meta = (MultiLine = true))
	FString Guidance;

	/* Intent words a designer would search for. */
	UPROPERTY(EditDefaultsOnly, Category = "AgentDoc")
	TArray<FName> Tags;

	/* Typed knowledge-base slugs of the form "pattern:<slug>" or "concept:<slug>". Tool-stamped. */
	UPROPERTY(EditDefaultsOnly, Category = "AgentDoc")
	TArray<FName> Articles;

	/* A doc counts as authored once any field is set. */
	bool HasDoc() const { return !Guidance.IsEmpty() || !Tags.IsEmpty() || !Articles.IsEmpty(); }
};

/** Builds an FFlowAgentDoc in one call for a native class's GetAgentDoc() override. */
inline FFlowAgentDoc MakeAgentDoc(
	const FString& InGuidance,
	TArray<FName> InTags,
	TArray<FName> InArticles = TArray<FName>())
{
	FFlowAgentDoc Doc;
	Doc.Guidance = InGuidance;
	Doc.Tags = MoveTemp(InTags);
	Doc.Articles = MoveTemp(InArticles);
	return Doc;
}
