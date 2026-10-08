// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "FlowCatalogQuery.h"

struct FFlowMCPMutationOptions;
struct FFlowMCPMutationReport;

/** What to write for one class's documentation. Guidance/Tags/Articles only - no description. */
struct FFlowAgentDocWriteRequest
{
	/** Stem, short class name, or full class path of the node/addon class to document. */
	FString ClassName;
	FString Guidance;
	TArray<FString> Tags;
	TArray<FString> Articles;
};

/** Outcome of an agent-doc write. */
struct FFlowAgentDocWriteResult
{
	FString ClassPath;

	/** How the class was authored, which decides where its doc can live. */
	EFlowClassOrigin Origin = EFlowClassOrigin::Unknown;
	/**
	 * True only when the write will survive an editor restart. False for a class whose defaults are
	 * compiled in: a runtime CDO write there is in-memory only, so reporting success would look like
	 * it worked and then silently vanish.
	 */
	bool bPersisted = false;

	/** For a non-persistable target, the exact source to paste. Empty otherwise. */
	FString SourceSnippet;

	/** Human-readable note explaining a false bPersisted, or an empty string. */
	FString Note;
	FString ErrorMessage;
};

/**
 * Writes the hand-authored documentation layer onto a Flow node or addon class.
 *
 * Deliberately origin-aware. A Blueprint class stores its doc as a serialized CDO default, so the
 * write is a real, savable asset edit. A native or script class returns a compiled-in constant from
 * its GetAgentDoc() override, so the only honest thing to do is hand back the source to paste rather
 * than mutate a default that will not survive the next launch.
 */
class FLOWGRAPHCOURIER_API FFlowAgentDocWriter
{
public:
	static FFlowAgentDocWriteResult Write(
		const FFlowAgentDocWriteRequest& Request,
		const FFlowMCPMutationOptions& MutationOptions,
		FFlowMCPMutationReport& OutReport);

	/** Renders the GetAgentDoc() override a non-persistable target needs, ready to paste into its .cpp. */
	static FString MakeSourceSnippet(
		const FString& ClassName,
		const FFlowAgentDocWriteRequest& Request);

	/**
	 * The AngelScript form: defaults on the inherited AgentDoc struct, inside the mandatory EDITOR
	 * guard. Verified against a live script compile - the arrays go through .Add(n"...") because a
	 * default statement compiles into __InitDefaults() rather than being a literal initializer.
	 */
	static FString MakeScriptSnippet(const FFlowAgentDocWriteRequest& Request);
};
