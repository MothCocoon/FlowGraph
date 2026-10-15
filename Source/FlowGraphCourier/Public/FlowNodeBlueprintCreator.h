// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"

struct FFlowMCPMutationOptions;
struct FFlowMCPMutationReport;

/** What to create. The asset class is deliberately absent - it is derived from ParentClass. */
struct FFlowNodeBlueprintCreateRequest
{
	FString PackagePath;
	FString AssetName;

	/** Short name or full path. Must be under UFlowNode or under UFlowNodeAddOn. */
	FString ParentClass;

	/** Both optional. Empty leaves the node falling back to its class name and class tooltip. */
	FString DisplayName;
	FString Description;
};

/** Outcome of a Flow node or add-on Blueprint creation. */
struct FFlowNodeBlueprintCreateResult
{
	FString AssetPath;

	/** The asset class actually created, derived from the parent rather than supplied. */
	FString AssetClassPath;

	FString GeneratedClassPath;
	FString ParentClassPath;

	bool bCompiled = false;
	TArray<FString> CompileErrors;

	/** Whether the new class resolves through the same catalog path FindFlowNodeTypes uses. */
	bool bResolvedInCatalog = false;

	/** Non-fatal observations, such as a display name or description left to fall back. */
	FString Note;

	FString ErrorMessage;
};

/**
 * Creates Blueprints that Flow will actually recognise.
 *
 * Exists because the asset class, not the parent class, is what makes a Flow node Blueprint
 * discoverable: UFlowGraphSchema and FlowCatalogQuery both filter by asset class, so a plain
 * UBlueprint parented to a UFlowNode compiles cleanly and is then invisible to the palette and the
 * catalog alike. Callers therefore do not choose an asset class - it is derived from the parent.
 *
 * Drives UFlowNodeBaseBlueprintFactory rather than IAssetTools::CreateAsset, for two reasons. The
 * factory adds the default K2_ExecuteInput and K2_Cleanup event nodes that an editor-created node
 * gets. And IAssetTools broadcasts FAssetRegistryModule::AssetCreated from inside its own call,
 * which would fire before this code could compile the Blueprint - leaving UAssetManager unable to
 * resolve a generated class and silently skipping the PrimaryAssetId until the next editor launch.
 *
 * Both of the factory's own failure modes are unreachable from here by construction: its
 * check(BlueprintClass->IsChildOf(SupportedClass)) cannot fire because the argument is sourced from
 * SupportedClass itself, and its ShowCannotCreateBlueprintDialog() modal - which would block
 * forever with no user present - is pre-empted by validating the same conditions first.
 */
class FLOWGRAPHCOURIER_API FFlowNodeBlueprintCreator
{
public:
	static FFlowNodeBlueprintCreateResult Create(
		const FFlowNodeBlueprintCreateRequest& Request,
		const FFlowMCPMutationOptions& MutationOptions,
		FFlowMCPMutationReport& OutReport);
};
