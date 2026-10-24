// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowNodeBlueprintCreator.h"

#include "AddOns/FlowNodeAddOn.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "FlowCatalogQuery.h"
#include "FlowMCPMutationContext.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/FlowNodeBlueprintFactory.h"
#include "UObject/Package.h"

namespace
{
	// Deliberately not UFlowGraphImporter::ResolveNodeClass. That resolver answers "which node type
	// may be placed in a graph" and is about to grow placement-specific rejections; a parent class
	// for a new Blueprint is a different question that must keep resolving native classes outside
	// Flow so they can be rejected here with a Flow-specific message rather than "not found".
	UClass* ResolveParentClass(const FString& ClassName)
	{
		if (ClassName.IsEmpty())
		{
			return nullptr;
		}

		if (ClassName.Contains(TEXT("/Script/")) || ClassName.Contains(TEXT(".")))
		{
			if (UClass* Loaded = LoadObject<UClass>(nullptr, *ClassName))
			{
				return Loaded;
			}
		}

		if (UClass* Prefixed = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Flow.%s"), *ClassName)))
		{
			return Prefixed;
		}

		return FindObject<UClass>(nullptr, *ClassName);
	}

	FString MakeFallbackNote(const FFlowNodeBlueprintCreateRequest& Request)
	{
		const bool bNoDisplayName = Request.DisplayName.IsEmpty();
		const bool bNoDescription = Request.Description.IsEmpty();
		if (!bNoDisplayName && !bNoDescription)
		{
			return FString();
		}

		if (bNoDisplayName && bNoDescription)
		{
			return TEXT("No DisplayName or Description supplied: the palette will show the class name and the class tooltip.");
		}

		return bNoDisplayName
			? TEXT("No DisplayName supplied: the palette will show the class name.")
			: TEXT("No Description supplied: the palette will show the class tooltip.");
	}
}

FFlowNodeBlueprintCreateResult FFlowNodeBlueprintCreator::Create(
	const FFlowNodeBlueprintCreateRequest& Request,
	const FFlowMCPMutationOptions& MutationOptions,
	FFlowMCPMutationReport& OutReport)
{
	FFlowNodeBlueprintCreateResult Result;

#if WITH_EDITOR
	if (Request.PackagePath.IsEmpty())
	{
		Result.ErrorMessage = TEXT("PackagePath is required.");
		return Result;
	}

	if (Request.AssetName.IsEmpty())
	{
		Result.ErrorMessage = TEXT("AssetName is required.");
		return Result;
	}

	UClass* ParentClass = ResolveParentClass(Request.ParentClass);
	if (!ParentClass)
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("Could not resolve a parent class from \"%s\"."),
			*Request.ParentClass);
		return Result;
	}

	Result.ParentClassPath = ParentClass->GetPathName();

	// Which subtree the parent sits in decides the asset class, so a parent in neither - including
	// UFlowNodeBase itself - is ambiguous and is rejected rather than guessed at.
	const bool bIsAddOn = ParentClass->IsChildOf(UFlowNodeAddOn::StaticClass());
	const bool bIsNode = ParentClass->IsChildOf(UFlowNode::StaticClass());
	if (!bIsAddOn && !bIsNode)
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("%s derives from neither UFlowNode nor UFlowNodeAddOn, so no Flow Blueprint asset class applies to it."),
			*Result.ParentClassPath);
		return Result;
	}

	// The same condition the factory would hit, checked here so its modal dialog stays unreachable.
	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("%s cannot be used as a Blueprint parent class."),
			*Result.ParentClassPath);
		return Result;
	}

	const FString AssetPath = Request.PackagePath / Request.AssetName;
	if (!FPackageName::IsValidLongPackageName(AssetPath))
	{
		Result.ErrorMessage = FString::Printf(TEXT("\"%s\" is not a valid package path."), *AssetPath);
		return Result;
	}

	const FString AssetObjectPath = AssetPath + TEXT(".") + Request.AssetName;
	if (FindObject<UObject>(nullptr, *AssetObjectPath))
	{
		Result.ErrorMessage = FString::Printf(TEXT("An asset already exists at %s."), *AssetPath);
		return Result;
	}

	FFlowMCPMutationContext MutationContext(MutationOptions);

	FString Error;
	if (!MutationContext.Begin(Error))
	{
		MutationContext.Abort();
		Result.ErrorMessage = Error;
		return Result;
	}

	Result.AssetPath = AssetPath;
	Result.Note = MakeFallbackNote(Request);

	// A dry run stops here rather than creating and rolling back. Everything above is validation, so
	// there is nothing left to preview, and undoing a freshly created package is a far riskier way to
	// arrive at the same answer.
	if (MutationContext.IsDryRun())
	{
		Result.AssetClassPath = bIsAddOn
			? UFlowNodeAddOnBlueprint::StaticClass()->GetPathName()
			: UFlowNodeBlueprint::StaticClass()->GetPathName();

		if (!MutationContext.Complete(OutReport, Error))
		{
			Result.ErrorMessage = Error;
		}
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		MutationContext.Abort();
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create package %s."), *AssetPath);
		return Result;
	}

	// Held as UFactory, and each branch names only its own concrete factory: the shared
	// UFlowNodeBaseBlueprintFactory carries no FLOWEDITOR_API on the class itself, so naming it as a
	// type from this module would not link.
	UFactory* Factory = nullptr;
	if (bIsAddOn)
	{
		UFlowNodeAddOnBlueprintFactory* AddOnFactory = NewObject<UFlowNodeAddOnBlueprintFactory>();
		AddOnFactory->ParentClass = ParentClass;
		Factory = AddOnFactory;
	}
	else
	{
		UFlowNodeBlueprintFactory* NodeFactory = NewObject<UFlowNodeBlueprintFactory>();
		NodeFactory->ParentClass = ParentClass;
		Factory = NodeFactory;
	}

	// Sourced from the factory itself so its check(BlueprintClass->IsChildOf(SupportedClass)) is
	// unfalsifiable, rather than restating the pairing here where the two could drift apart.
	UClass* BlueprintClass = Factory->GetSupportedClass();
	Result.AssetClassPath = BlueprintClass->GetPathName();

	UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		BlueprintClass,
		Package,
		FName(*Request.AssetName),
		RF_Public | RF_Standalone | RF_Transactional,
		nullptr,
		GWarn));

	if (!NewBlueprint)
	{
		MutationContext.Abort();
		Result.ErrorMessage = FString::Printf(
			TEXT("The Flow Blueprint factory declined to create %s from parent %s."),
			*AssetPath, *Result.ParentClassPath);
		return Result;
	}

	if (!Request.DisplayName.IsEmpty())
	{
		NewBlueprint->BlueprintDisplayName = Request.DisplayName;
	}
	if (!Request.Description.IsEmpty())
	{
		NewBlueprint->BlueprintDescription = Request.Description;
	}

	// Compile before AssetCreated below. That broadcast reaches UAssetManager::OnInMemoryAssetCreated,
	// which only registers a primary asset once the generated class resolves, so an uncompiled
	// Blueprint is silently skipped and gains no PrimaryAssetId until a resave or editor restart.
	// SkipGarbageCollection because this op is reachable from a script running under Python: a UE GC
	// here broadcasts PreGarbageCollect, whose handler calls PyGC_Collect() against a live CPython
	// eval frame and access-violates. The flag suppresses only that trailing CollectGarbage call.
	FCompilerResultsLog CompileResults;
	FKismetEditorUtilities::CompileBlueprint(
		NewBlueprint, EBlueprintCompileOptions::SkipGarbageCollection, &CompileResults);
	Result.bCompiled = CompileResults.NumErrors == 0;
	for (const TSharedRef<FTokenizedMessage>& Message : CompileResults.Messages)
	{
		if (Message->GetSeverity() == EMessageSeverity::Error)
		{
			Result.CompileErrors.Add(Message->ToText().ToString());
		}
	}

	FAssetRegistryModule::AssetCreated(NewBlueprint);

	if (NewBlueprint->GeneratedClass)
	{
		Result.GeneratedClassPath = NewBlueprint->GeneratedClass->GetPathName();
	}

	if (!MutationContext.PrepareAsset(NewBlueprint, Error))
	{
		MutationContext.Abort();
		Result.ErrorMessage = Error;
		return Result;
	}

	// bMarkBlueprintModified false: the compile above already settled the Blueprint's status, and
	// marking it modified afterward would leave a freshly created asset reading as dirty.
	if (!MutationContext.FinalizeAsset(NewBlueprint, NewBlueprint, /*bMarkBlueprintModified=*/false, Error))
	{
		MutationContext.Abort();
		Result.ErrorMessage = Error;
		return Result;
	}

	// Verified through the catalog rather than by inspecting the asset, so the op fails for the same
	// reason FindFlowNodeTypes would come up empty rather than for a parallel reimplementation of it.
	if (NewBlueprint->GeneratedClass)
	{
		const UClass* Resolved =
			UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(Result.GeneratedClassPath);
		Result.bResolvedInCatalog = Resolved == NewBlueprint->GeneratedClass;
	}

	if (!MutationContext.Complete(OutReport, Error))
	{
		Result.ErrorMessage = Error;
		return Result;
	}

	return Result;
#else
	Result.ErrorMessage = TEXT("Flow node Blueprints can only be created in an editor build.");
	return Result;
#endif
}
