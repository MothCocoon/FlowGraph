// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowAgentDocWriter.h"

#include "Engine/Blueprint.h"
#include "FlowCatalogQuery.h"
#include "FlowMCPMutationContext.h"
#include "Nodes/FlowAgentDoc.h"
#include "Nodes/FlowNodeBase.h"

namespace
{
	/** Escapes a string for embedding in a C++ TEXT("...") literal. */
	FString EscapeForCppLiteral(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Escaped.ReplaceInline(TEXT("\r\n"), TEXT("\\n"));
		Escaped.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		Escaped.ReplaceInline(TEXT("\t"), TEXT("\\t"));
		return Escaped;
	}

	FString MakeNameListLiteral(const TArray<FString>& Values)
	{
		TArray<FString> Literals;
		Literals.Reserve(Values.Num());
		for (const FString& Value : Values)
		{
			Literals.Add(FString::Printf(TEXT("TEXT(\"%s\")"), *EscapeForCppLiteral(Value)));
		}
		return FString::Printf(TEXT("{ %s }"), *FString::Join(Literals, TEXT(", ")));
	}

	FFlowAgentDoc MakeDocFromRequest(const FFlowAgentDocWriteRequest& Request)
	{
		FFlowAgentDoc Doc;
		Doc.Guidance = Request.Guidance;
		for (const FString& Tag : Request.Tags)
		{
			Doc.Tags.Add(FName(*Tag));
		}
		for (const FString& Article : Request.Articles)
		{
			Doc.Articles.Add(FName(*Article));
		}
		return Doc;
	}
}

FString FFlowAgentDocWriter::MakeSourceSnippet(
	const FString& ClassName,
	const FFlowAgentDocWriteRequest& Request)
{
	// A file-static constant rather than a constructor assignment: it costs no per-CDO memory and no
	// editor-only data on the class, and it keeps the doc next to the implementation it describes.
	return FString::Printf(
		TEXT("// Paste this override into the class's .cpp (and declare it in the header):\n")
		TEXT("#if WITH_EDITOR\n")
		TEXT("const FFlowAgentDoc& %s::GetAgentDoc() const\n")
		TEXT("{\n")
		TEXT("\tstatic const FFlowAgentDoc Doc = MakeAgentDoc(\n")
		TEXT("\t\t/*Guidance*/ TEXT(\"%s\"),\n")
		TEXT("\t\t/*Tags*/     %s,\n")
		TEXT("\t\t/*Articles*/ %s,\n")
		TEXT("\treturn Doc;\n")
		TEXT("}\n")
		TEXT("#endif\n"),
		*ClassName,
		*EscapeForCppLiteral(Request.Guidance),
		*MakeNameListLiteral(Request.Tags),
		*MakeNameListLiteral(Request.Articles));
}

FString FFlowAgentDocWriter::MakeScriptSnippet(const FFlowAgentDocWriteRequest& Request)
{
	// A script class has no header to declare a C++ override in, so its doc goes in the class body as
	// defaults on the inherited struct. The EDITOR guard is mandatory, not stylistic: AgentDoc is
	// editor-only data, and touching it outside the guard fails the whole script module's compile with
	// "Cannot use editor-only property AgentDoc outside of an EDITOR block" - which reverts every
	// script to the last good build, not just this class.
	FString Snippet;
	Snippet += TEXT("#if EDITOR\n");
	Snippet += FString::Printf(TEXT("\tdefault AgentDoc.Guidance = \"%s\";\n"), *EscapeForCppLiteral(Request.Guidance));
	for (const FString& Tag : Request.Tags)
	{
		Snippet += FString::Printf(TEXT("\tdefault AgentDoc.Tags.Add(n\"%s\");\n"), *EscapeForCppLiteral(Tag));
	}
	for (const FString& Article : Request.Articles)
	{
		Snippet += FString::Printf(TEXT("\tdefault AgentDoc.Articles.Add(n\"%s\");\n"), *EscapeForCppLiteral(Article));
	}
	Snippet += TEXT("#endif\n");
	return Snippet;
}

FFlowAgentDocWriteResult FFlowAgentDocWriter::Write(
	const FFlowAgentDocWriteRequest& Request,
	const FFlowMCPMutationOptions& MutationOptions,
	FFlowMCPMutationReport& OutReport)
{
	FFlowAgentDocWriteResult Result;

	if (Request.Guidance.IsEmpty())
	{
		Result.ErrorMessage = TEXT("Guidance is required - a doc without one reads as undocumented.");
		return Result;
	}

	UClass* TargetClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(Request.ClassName);
	if (!TargetClass)
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("No Flow node or addon class resolved from \"%s\" (an ambiguous stem resolves to nothing by design)."),
			*Request.ClassName);
		return Result;
	}

	Result.ClassPath = TargetClass->GetPathName();
	Result.Origin = UFlowCatalogQuery::GetClassOrigin(TargetClass);

#if WITH_EDITOR
	UFlowNodeBase* DefaultNode = Cast<UFlowNodeBase>(TargetClass->GetDefaultObject());
	if (!DefaultNode)
	{
		Result.ErrorMessage = FString::Printf(TEXT("%s has no usable class default object."), *Result.ClassPath);
		return Result;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(TargetClass->ClassGeneratedBy);
	if (!Blueprint)
	{
		// Compiled-in defaults cannot be authored at runtime, so say so plainly and hand back the
		// source instead of performing a write that disappears on restart. Which source depends on the
		// origin: a script class has no header, so handing it a C++ override would be unusable advice.
		Result.bPersisted = false;

		const bool bIsScriptClass =
			Result.Origin == EFlowClassOrigin::AngelScript || Result.Origin == EFlowClassOrigin::Unknown;
		if (bIsScriptClass)
		{
			Result.SourceSnippet = MakeScriptSnippet(Request);
			Result.Note = FString::Printf(
				TEXT("%s is a script class, so its doc lives in its .as source. Paste the returned block into the class body. ")
				TEXT("Keep the #if EDITOR guard - AgentDoc is editor-only data, and assigning it outside the guard fails the ")
				TEXT("whole script module's compile and reverts every script to the last good build."),
				*TargetClass->GetName());
		}
		else
		{
			Result.SourceSnippet = MakeSourceSnippet(TargetClass->GetName(), Request);
			Result.Note = FString::Printf(
				TEXT("%s is a native class, so its doc lives in source rather than in a saved default. Paste the returned snippet into its implementation file and declare the override in its header."),
				*TargetClass->GetName());
		}
		return Result;
	}

	FFlowMCPMutationContext MutationContext(MutationOptions);

	FString Error;
	if (!MutationContext.Begin(Error) || !MutationContext.PrepareAsset(Blueprint, Error))
	{
		MutationContext.Abort();
		Result.ErrorMessage = Error;
		return Result;
	}

	const FFlowAgentDoc PreviousDoc = DefaultNode->GetAgentDoc();
	MutationContext.RegisterDryRunRollback([DefaultNode, PreviousDoc]()
	{
		DefaultNode->SetAgentDoc(PreviousDoc);
	});

	MutationContext.Modify(DefaultNode);
	DefaultNode->SetAgentDoc(MakeDocFromRequest(Request));

	if (!MutationContext.FinalizeAsset(Blueprint, Blueprint, /*bMarkBlueprintModified=*/true, Error) ||
		!MutationContext.Complete(OutReport, Error))
	{
		Result.ErrorMessage = Error;
		return Result;
	}

	Result.bPersisted = !MutationOptions.bDryRun;
	if (MutationOptions.bDryRun)
	{
		Result.Note = TEXT("Dry run - nothing was written. Re-run with bDryRun false to apply.");
	}
	return Result;
#else
	Result.ErrorMessage = TEXT("Agent docs can only be authored in an editor build.");
	return Result;
#endif
}
