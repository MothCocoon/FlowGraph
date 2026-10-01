// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowMCPToolset.h"

#include "AddOns/FlowNodeAddOn.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "Find/FlowSearch.h"
#include "FlowAgentDocWriter.h"
#include "FlowAsset.h"
#include "FlowNodeUsageIndex.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowGraphDiff.h"
#include "Graph/FlowGraphEditorLayout.h"
#include "FlowGraphExporter.h"
#include "FlowCatalogQuery.h"
#include "FlowGraphImporter.h"
#include "FlowGraphReconciler.h"
#include "FlowGraphRegrapher.h"
#include "FlowGraphSubgraphQuery.h"
#include "FlowGraphValidation.h"
#include "FlowMCPMutationContext.h"
#include "FlowNodeClassReplacement.h"
#include "Graph/Collapse/FlowSubgraphSelection.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "ObjectTools.h"
#include "FlowNodeBlueprintCreator.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Nodes/FlowNodeBase.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowMCPToolset)

namespace
{
	void RaiseFlowGraphToolError(const FString& Message)
	{
		UKismetSystemLibrary::RaiseScriptError(
			FString::Printf(TEXT("FlowMCPToolset: %s"), *Message));
	}

	void DiscardFailedImport(const FTopLevelAssetPath& AssetPath, bool bPackageWasDirty)
	{
		UPackage* Package = FindPackage(nullptr, *AssetPath.GetPackageName().ToString());
		if (!Package)
		{
			return;
		}

		if (UFlowAsset* Asset = FindObject<UFlowAsset>(Package, *AssetPath.GetAssetName().ToString()))
		{
			FAssetRegistryModule::AssetDeleted(Asset);
			const FName TransientName = MakeUniqueObjectName(GetTransientPackage(), Asset->GetClass(), Asset->GetFName());
			Asset->ClearFlags(RF_Public | RF_Standalone);
			Asset->Rename(*TransientName.ToString(), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
			Asset->MarkAsGarbage();
		}

		Package->SetDirtyFlag(bPackageWasDirty);
	}

	bool ParseAssetPath(
		const FString& AssetPath,
		FTopLevelAssetPath& OutParsedPath,
		FString& OutError)
	{
		if (AssetPath.IsEmpty())
		{
			OutError = TEXT("AssetPath is required.");
			return false;
		}

		OutParsedPath = FTopLevelAssetPath(AssetPath);
		if (OutParsedPath.GetAssetName().IsNone())
		{
			OutParsedPath.TrySetPath(
				*AssetPath,
				FPackageName::GetShortFName(AssetPath));
		}

		if (OutParsedPath.GetAssetName().IsNone())
		{
			OutError = FString::Printf(
				TEXT("AssetPath could not be parsed: %s"),
				*AssetPath);
			return false;
		}

		return true;
	}

	UFlowAsset* LoadFlowAsset(
		const FString& AssetPath,
		FTopLevelAssetPath& OutParsedPath,
		FString& OutError)
	{
		if (!ParseAssetPath(AssetPath, OutParsedPath, OutError))
		{
			return nullptr;
		}

		// Checks in-memory (FindObject) before disk (LoadObject) - a disk-only lookup here would
		// miss an asset that was just created and not yet saved.
		UFlowAsset* FlowAsset = FFlowGraphReconciler::FindOrLoadFlowAsset(AssetPath);
		if (!FlowAsset)
		{
			OutError = FString::Printf(
				TEXT("FlowAsset not found: %s"),
				*AssetPath);
			return nullptr;
		}

		return FlowAsset;
	}

	bool BeginMutation(
		UFlowAsset* FlowAsset,
		FFlowMCPMutationContext& MutationContext)
	{
		FString Error;
		if (!MutationContext.Begin(Error) ||
			!MutationContext.PrepareAsset(FlowAsset, Error))
		{
			MutationContext.Abort();
			RaiseFlowGraphToolError(Error);
			return false;
		}

		return true;
	}

	bool CompleteMutation(
		UFlowAsset* FlowAsset,
		FFlowMCPMutationContext& MutationContext,
		FFlowMCPMutationReport& OutReport)
	{
		FString Error;
		if (!MutationContext.FinalizeAsset(FlowAsset, nullptr, false, Error) ||
			!MutationContext.Complete(OutReport, Error))
		{
			MutationContext.Abort();
			RaiseFlowGraphToolError(Error);
			return false;
		}

		return true;
	}

	void CopySubgraphSelectionPlan(
		const FlowSubgraphSelection::FSelectionPlanSummary& Source,
		FFlowMCPFlowSubgraphSelectionPlan& Destination)
	{
		Destination.bCanApply = Source.bCanApply;
		Destination.SelectedNodeCount = Source.SelectedNodeCount;
		Destination.EntryCount = Source.EntryCount;
		Destination.ExitCount = Source.ExitCount;
		Destination.Errors = Source.Errors;
		Destination.Warnings = Source.Warnings;
		Destination.InterfaceInputs = Source.InterfaceInputs;
		Destination.InterfaceOutputs = Source.InterfaceOutputs;
	}

	bool ParseSelectionGuids(
		const TArray<FString>& GuidStrings,
		TArray<FGuid>& OutGuids,
		FString& OutError)
	{
		OutGuids.Reset();
		OutGuids.Reserve(GuidStrings.Num());
		for (const FString& GuidString : GuidStrings)
		{
			FGuid ParsedGuid;
			if (!FGuid::Parse(GuidString, ParsedGuid))
			{
				OutError = FString::Printf(
					TEXT("SelectionGuids contains an invalid GUID: %s"),
					*GuidString);
				return false;
			}

			OutGuids.Add(ParsedGuid);
		}

		if (OutGuids.IsEmpty())
		{
			OutError = TEXT("SelectionGuids must contain at least one node GUID.");
			return false;
		}

		return true;
	}

	bool DiscardCreatedSubgraph(UFlowAsset* NewAsset)
	{
		if (!IsValid(NewAsset))
		{
			return true;
		}

		const TArray<UObject*> AssetsToDelete = {NewAsset};
		return ObjectTools::ForceDeleteObjects(AssetsToDelete, false) == AssetsToDelete.Num();
	}
}

FFlowMCPExportFlowAssetResult UFlowMCPToolset::ExportFlowAsset(
	const FFlowMCPExportFlowAssetRequest& Request)
{
	FFlowMCPExportFlowAssetResult Result;
	FString Error;
	FTopLevelAssetPath ParsedPath;
	UFlowAsset* FlowAsset = LoadFlowAsset(Request.AssetPath, ParsedPath, Error);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(Error);
		return Result;
	}

	Result.ExportedText = UFlowGraphExporter::ExportFlowGraphToString(FlowAsset);
	if (Result.ExportedText.IsEmpty())
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("Failed to export FlowAsset '%s': exported text is empty."),
			*Request.AssetPath));
		return FFlowMCPExportFlowAssetResult();
	}

	Result.AssetPath = ParsedPath.ToString();
	Result.TextLength = Result.ExportedText.Len();

	// The export above is generated from the runtime node map, which is also what a mutation writes -
	// so an export can never reveal a mutation that reached the runtime map but not the editor graph.
	// Reporting parity alongside it is what makes that class of damage visible to a caller who is
	// verifying by export.
	UFlowGraphRegrapher::CollectGraphParityIssues(FlowAsset, Result.GraphIntegrityIssues);
	return Result;
}

FFlowMCPImportAndRegraphFlowAssetResult UFlowMCPToolset::ImportAndRegraphFlowAsset(
	const FFlowMCPImportAndRegraphFlowAssetRequest& Request)
{
	FFlowMCPImportAndRegraphFlowAssetResult Result;
	if (Request.FlowGraphText.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("FlowGraphText is required."));
		return Result;
	}

	if (Request.AssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("AssetPath is required."));
		return Result;
	}

	if (!Request.AssetPath.StartsWith(TEXT("/")))
	{
		RaiseFlowGraphToolError(
			TEXT("AssetPath must start with '/' (for example, '/Game/MyFlows/MyFlow')."));
		return Result;
	}

	if (Request.Mutation.bDryRun)
	{
		RaiseFlowGraphToolError(
			TEXT("ImportAndRegraphFlowAsset does not support dry runs."));
		return Result;
	}

	FTopLevelAssetPath ParsedPath;
	FString Error;
	if (!ParseAssetPath(Request.AssetPath, ParsedPath, Error))
	{
		RaiseFlowGraphToolError(Error);
		return Result;
	}

	FFlowCourierDocument Document;
	TArray<FFlowCourierIssue> CourierIssues;
	if (!FFlowCourierConverter::ParseDocument(Request.FlowGraphText, Document, CourierIssues, Error))
	{
		RaiseFlowGraphToolError(Error);
		return Result;
	}

	TArray<FFlowGraphParsedNode> ParsedNodes;
	TArray<FFlowGraphParsedConnection> ParsedConnections;
	TArray<FGuid> ScopedNodeGuidsUnused;
	TMap<FString, FGuid> AliasMapUnused;
	FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, CourierIssues);

	for (const FFlowCourierIssue& Issue : CourierIssues)
	{
		if (Issue.Severity == EFlowValidationSeverity::Error)
		{
			RaiseFlowGraphToolError(FString::Printf(TEXT("[%s] %s: %s"), *Issue.Code, *Issue.Path, *Issue.Message));
			return Result;
		}
	}
	// Check loaded and saved assets of any class. An unsaved object in the current editor session
	// must also block import at the same path.
	UPackage* ExistingPackage = FindPackage(nullptr, *ParsedPath.GetPackageName().ToString());
	UObject* ExistingObject = ExistingPackage
		? FindObject<UObject>(ExistingPackage, *ParsedPath.GetAssetName().ToString())
		: nullptr;
	if (ExistingObject || LoadObject<UObject>(nullptr, *ParsedPath.ToString()))
	{
		RaiseFlowGraphToolError(TEXT("An asset already exists at AssetPath. Use ApplyFlowPatch to edit it."));
		return Result;
	}
	UClass* AssetClass = LoadObject<UClass>(nullptr, *Document.AssetClass);
	if (!AssetClass || !AssetClass->IsChildOf(UFlowAsset::StaticClass()))
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("AssetClass is not a resolvable FlowAsset class: %s"), *Document.AssetClass));
		return Result;
	}
	const bool bPackageWasDirty = ExistingPackage && ExistingPackage->IsDirty();
	FFlowMCPMutationContext MutationContext(Request.Mutation);
	if (!MutationContext.Begin(Error))
	{
		RaiseFlowGraphToolError(Error);
		return Result;
	}

	UFlowAsset* ImportedAsset = UFlowGraphRegrapher::ImportAndRegraphFromDocument(
			ParsedPath.ToString(),
			Document.AssetClass,
			Document.bWorldBound,
			ParsedNodes,
			ParsedConnections);
	if (!ImportedAsset)
	{
		MutationContext.CancelUnmodified();
		DiscardFailedImport(ParsedPath, bPackageWasDirty);
		RaiseFlowGraphToolError(TEXT("Failed to import and rebuild FlowAsset from Courier document."));
		return Result;
	}

	if (!MutationContext.PrepareAsset(ImportedAsset, Error))
	{
		MutationContext.CancelUnmodified();
		DiscardFailedImport(ParsedPath, bPackageWasDirty);
		RaiseFlowGraphToolError(Error);
		return Result;
	}

	MutationContext.Modify(ImportedAsset);

	if (!CompleteMutation(ImportedAsset, MutationContext, Result.Mutation))
	{
		DiscardFailedImport(ParsedPath, bPackageWasDirty);
		return FFlowMCPImportAndRegraphFlowAssetResult();
	}

	Result.AssetPath = ImportedAsset->GetPathName();
	Result.PackageName = ImportedAsset->GetOutermost()->GetName();
	Result.AssetName = ImportedAsset->GetName();
	Result.NodeCount = ImportedAsset->GetNodes().Num();

	// Verify every requested connection against the imported runtime graph. A successful import
	// alone does not establish that every connection was created.
	TArray<FString> VerifiedConnections, NotLandedConnections;
	FFlowGraphReconciler::VerifyConnectionsLanded(ImportedAsset, ParsedConnections, VerifiedConnections, NotLandedConnections);
	for (const FString& NotLandedLabel : NotLandedConnections)
	{
		Result.Findings.Add(FString::Printf(TEXT("Requested connection did not land: %s"), *NotLandedLabel));
	}

	return Result;
}

namespace
{
	UFlowAsset* LoadFlowAssetOrNull(const FString& AssetPath)
	{
		// Checks in-memory (FindObject) before disk (LoadObject) - a disk-only lookup here would
		// miss an asset that was just created and not yet saved.
		return FFlowGraphReconciler::FindOrLoadFlowAsset(AssetPath);
	}

	TArray<FString> GuidArrayToStringArray(const TArray<FGuid>& Guids)
	{
		TArray<FString> Result;
		Result.Reserve(Guids.Num());
		for (const FGuid& Guid : Guids)
		{
			Result.Add(Guid.ToString());
		}
		return Result;
	}

	FFlowMCPReconcileAddonEntry ToFlowMCPReconcileAddonEntry(const FFlowReconcileAddonEntry& Entry)
	{
		FFlowMCPReconcileAddonEntry Result;
		Result.OwnerNodeGuid = Entry.OwnerNodeGuid.ToString();
		Result.AddonGuid = Entry.AddOnGuid.ToString();
		Result.AddonType = Entry.AddOnType;
		Result.Properties = Entry.Properties;
		return Result;
	}

	TArray<FFlowMCPReconcileAddonEntry> ToFlowMCPReconcileAddonEntries(const TArray<FFlowReconcileAddonEntry>& Entries)
	{
		TArray<FFlowMCPReconcileAddonEntry> Result;
		Result.Reserve(Entries.Num());
		for (const FFlowReconcileAddonEntry& Entry : Entries)
		{
			Result.Add(ToFlowMCPReconcileAddonEntry(Entry));
		}
		return Result;
	}

	FFlowMCPReconcilePlan ToFlowMCPReconcilePlan(const FFlowReconcilePlan& Plan)
	{
		FFlowMCPReconcilePlan Result;
		Result.NodesAdded = GuidArrayToStringArray(Plan.NodesAdded);
		Result.NodesUpdated = GuidArrayToStringArray(Plan.NodesUpdated);
		Result.NodesDeleted = GuidArrayToStringArray(Plan.NodesDeleted);
		Result.ConnectionsAdded = Plan.ConnectionsAdded;
		Result.ConnectionsRemoved = Plan.ConnectionsRemoved;
		Result.AddonsAdded = ToFlowMCPReconcileAddonEntries(Plan.AddOnsAdded);
		Result.AddonsUpdated = ToFlowMCPReconcileAddonEntries(Plan.AddOnsUpdated);
		Result.AddonsDeleted = ToFlowMCPReconcileAddonEntries(Plan.AddOnsDeleted);
		Result.bIsEmpty = Plan.IsEmpty();
		return Result;
	}

	FString ValidationSeverityToString(EFlowValidationSeverity Severity)
	{
		const UEnum* SeverityEnum = StaticEnum<EFlowValidationSeverity>();
		return SeverityEnum
			? SeverityEnum->GetNameStringByValue(static_cast<int64>(Severity))
			: TEXT("Unknown");
	}

	TArray<FFlowMCPValidationFinding> ToFlowMCPValidationFindings(const TArray<FFlowValidationFinding>& Findings, bool& bOutHasBlockingError)
	{
		TArray<FFlowMCPValidationFinding> Result;
		Result.Reserve(Findings.Num());
		bOutHasBlockingError = false;

		for (const FFlowValidationFinding& Finding : Findings)
		{
			FFlowMCPValidationFinding FlowMCPFinding;
			FlowMCPFinding.Severity = ValidationSeverityToString(Finding.Severity);
			FlowMCPFinding.Code = Finding.Code;
			FlowMCPFinding.Message = Finding.Message;
			if (Finding.NodeGuid.IsValid())
			{
				FlowMCPFinding.NodeGuid = Finding.NodeGuid.ToString();
			}

			if (Finding.AddOnGuid.IsValid())
			{
				FlowMCPFinding.AddonGuid = Finding.AddOnGuid.ToString();
			}

			if (Finding.PinName != NAME_None)
			{
				FlowMCPFinding.PinName = Finding.PinName.ToString();
			}
			Result.Add(FlowMCPFinding);

			if (Finding.Severity == EFlowValidationSeverity::Error)
			{
				bOutHasBlockingError = true;
			}
		}
		return Result;
	}

	// Flattens the recursive C++-only FFlowGraphDiffAddOn tree into a flat, reflection-safe list:
	// one entry per addon in the subtree, tagged with ChangeType and ParentGuid (owner node GUID
	// at top level, else the parent addon's GUID when recursing into nested addon-of-addon children).
	void FlattenAddonDiffs(
		const TArray<FFlowGraphDiffAddOn>& AddOns,
		const FString& ChangeType,
		const FString& ParentGuid,
		TArray<FFlowMCPAddonDiffEntry>& Out)
	{
		for (const FFlowGraphDiffAddOn& AddOn : AddOns)
		{
			FFlowMCPAddonDiffEntry Entry;
			Entry.AddonGuid = AddOn.AddOnGuid.ToString();
			Entry.ParentGuid = ParentGuid;
			Entry.ChangeType = ChangeType;
			Entry.AddonType = AddOn.AddOnType;
			Entry.ChangedProperties = AddOn.ChangedProperties;
			Out.Add(Entry);

			FlattenAddonDiffs(AddOn.AddedChildren, TEXT("added"), Entry.AddonGuid, Out);
			FlattenAddonDiffs(AddOn.RemovedChildren, TEXT("removed"), Entry.AddonGuid, Out);
			FlattenAddonDiffs(AddOn.ChangedChildren, TEXT("changed"), Entry.AddonGuid, Out);
		}
	}

	FFlowMCPConnectionRef ToFlowMCPConnectionRef(const FFlowGraphDiffConnection& Connection)
	{
		FFlowMCPConnectionRef Result;
		Result.SourceGuid = Connection.SourceNodeGuid.ToString();
		Result.SourcePin = Connection.SourcePinName.ToString();
		Result.TargetGuid = Connection.TargetNodeGuid.ToString();
		Result.TargetPin = Connection.TargetPinName.ToString();
		return Result;
	}

	TArray<FFlowMCPConnectionRef> ToFlowMCPConnectionRefArray(const TArray<FFlowGraphDiffConnection>& Connections)
	{
		TArray<FFlowMCPConnectionRef> Result;
		Result.Reserve(Connections.Num());
		for (const FFlowGraphDiffConnection& Connection : Connections)
		{
			Result.Add(ToFlowMCPConnectionRef(Connection));
		}
		return Result;
	}

	// Resolves a DiffFlowAsset Old/New argument to export text: if it looks like a raw Courier v2
	// JSON document (starts with '{' once leading whitespace is trimmed), use it as-is; otherwise
	// treat it as an asset path, load it, and export it via the same UFlowGraphExporter path
	// export_flow_asset uses.
	bool ResolveDiffInput(const FString& Input, FString& OutText, FString& OutSourceKind, FString& OutErrorMessage)
	{
		if (Input.TrimStart().StartsWith(TEXT("{")))
		{
			OutText = Input;
			OutSourceKind = TEXT("text");
			return true;
		}

		OutSourceKind = TEXT("asset_path");

		UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Input);
		if (!FlowAsset)
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to load asset at path: %s"), *Input);
			return false;
		}

		OutText = UFlowGraphExporter::ExportFlowGraphToString(FlowAsset);
		if (OutText.IsEmpty())
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to export FlowAsset at path %s - exported text is empty"), *Input);
			return false;
		}

		return true;
	}

}

FFlowMCPCheckAddonAttachmentEligibilityResult UFlowMCPToolset::CheckAddonAttachmentEligibility(
	const FFlowMCPCheckAddonAttachmentEligibilityRequest& Request)
{
	FFlowMCPCheckAddonAttachmentEligibilityResult Result;
	if (Request.ParentClassName.IsEmpty() || Request.AddonClassName.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("ParentClassName and AddonClassName must both be non-empty."));
		return Result;
	}

	UClass* ParentClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(Request.ParentClassName);
	if (!ParentClass)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("Could not resolve parent class: %s"), *Request.ParentClassName));
		return Result;
	}

	UClass* AddonClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(Request.AddonClassName);
	if (!AddonClass)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("Could not resolve addon class: %s"), *Request.AddonClassName));
		return Result;
	}

	if (!AddonClass->IsChildOf(UFlowNodeAddOn::StaticClass()))
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("%s is not a UFlowNodeAddOn subclass"), *Request.AddonClassName));
		return Result;
	}

	const UFlowNodeBase* ParentCDO = Cast<UFlowNodeBase>(ParentClass->GetDefaultObject());
	if (!ParentCDO)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("%s is not a UFlowNodeBase subclass"), *Request.ParentClassName));
		return Result;
	}

	const UFlowNodeAddOn* AddonCDO = Cast<UFlowNodeAddOn>(AddonClass->GetDefaultObject());
	if (!AddonCDO)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("Failed to get CDO for addon class: %s"), *Request.AddonClassName));
		return Result;
	}

#if WITH_EDITOR
	const TArray<UFlowNodeAddOn*> NoOtherAddOns;
	const EFlowAddOnAcceptResult AcceptResult = ParentCDO->CheckAcceptFlowNodeAddOnChild(AddonCDO, NoOtherAddOns);

	// Match FFlowGraphValidation::ValidateAddOnsRecursive's gate exactly: only an explicit Reject
	// blocks attachment; Undetermined and TentativeAccept both pass.
	Result.bEligible = (AcceptResult != EFlowAddOnAcceptResult::Reject);

	Result.Result = StaticEnum<EFlowAddOnAcceptResult>()->GetNameStringByValue(static_cast<int64>(AcceptResult));
	if (Result.Result.IsEmpty())
	{
		Result.Result = TEXT("Unknown");
	}

	Result.ParentClass = ParentClass->GetPathName();
	Result.AddonClass = AddonClass->GetPathName();
	if (!Result.bEligible)
	{
		Result.Reason = FString::Printf(
			TEXT("%s does not accept %s as a child addon (result: %s)"),
			*ParentClass->GetName(), *AddonClass->GetName(), *Result.Result);
	}

	return Result;
#else
	RaiseFlowGraphToolError(TEXT("CheckAddonAttachmentEligibility requires an editor build."));
	return Result;
#endif
}

namespace
{
	void AppendPinDescriptors(const TArray<FFlowCatalogPinRow>& PinRows, TArray<FFlowMCPPinDescriptor>& OutDescriptors)
	{
		for (const FFlowCatalogPinRow& PinRow : PinRows)
		{
			FFlowMCPPinDescriptor& PinDetail = OutDescriptors.AddDefaulted_GetRef();
			PinDetail.Name = PinRow.Name;
			PinDetail.Type = PinRow.Type;
			PinDetail.Description = PinRow.Description;
		}
	}

	FFlowMCPClassDetail CatalogRowToClassDetail(const FFlowCatalogClassRow& Row)
	{
		FFlowMCPClassDetail Detail;
		Detail.Name = Row.Stem;
		Detail.ClassPath = Row.ClassPath;
		Detail.Description = Row.Description;
		Detail.Origin = Row.Origin;
		Detail.bDeprecated = Row.bDeprecated;
		Detail.Articles = Row.Articles;

		Detail.Doc.Guidance = Row.Doc.Guidance;
		Detail.Doc.Tags = Row.Doc.Tags;

		for (const FFlowCatalogPropertyRow& PropRow : Row.Properties)
		{
			FFlowMCPPropertyDescriptor& PropDetail = Detail.Properties.AddDefaulted_GetRef();
			PropDetail.Name = PropRow.Name;
			PropDetail.Type = PropRow.Type;
			PropDetail.PinBinding = PropRow.PinBinding;
			PropDetail.Description = PropRow.Description;
			PropDetail.DeclaringClass = PropRow.DeclaringClass;
			PropDetail.Value = PropRow.Value;
		}

		AppendPinDescriptors(Row.InputPins, Detail.InputPins);
		AppendPinDescriptors(Row.OutputPins, Detail.OutputPins);

		return Detail;
	}

	void AppendFacets(const TArray<FFlowCatalogFacetRow>& Facets, TArray<FFlowMCPCatalogFacet>& OutFacets)
	{
		for (const FFlowCatalogFacetRow& Facet : Facets)
		{
			FFlowMCPCatalogFacet& MCPFacet = OutFacets.AddDefaulted_GetRef();
			MCPFacet.Value = Facet.Value;
			MCPFacet.Count = Facet.Count;
		}
	}
}

FFlowMCPListFlowAssetTypesResult UFlowMCPToolset::ListFlowAssetTypes(
	const FFlowMCPListFlowAssetTypesRequest& Request)
{
	FFlowMCPListFlowAssetTypesResult Result;

	TArray<FFlowCatalogAssetTypeRow> Rows;
	UFlowCatalogQuery::ListAssetTypes(Rows);

	for (const FFlowCatalogAssetTypeRow& Row : Rows)
	{
		FFlowMCPFlowAssetTypeSummary Summary;
		Summary.ClassName = Row.ClassName;
		Summary.ClassPath = Row.ClassPath;
		Summary.Description = Row.Description;
		Summary.Keywords = Row.Keywords;
		Summary.AllowedNodeCount = Row.AllowedNodeCount;
		Summary.AllowedAddonCount = Row.AllowedAddonCount;
		Result.AssetTypes.Add(Summary);
	}

	Result.FoundCount = Result.AssetTypes.Num();
	return Result;
}

FFlowMCPFindFlowNodeTypesResult UFlowMCPToolset::FindFlowNodeTypes(
	const FFlowMCPFindFlowNodeTypesRequest& Request)
{
	FFlowMCPFindFlowNodeTypesResult Result;

	FFlowCatalogQueryParams Params;
	Params.AssetClassName = Request.AssetClassName;
	Params.ClassNames = Request.ClassNames;
	Params.Kind = Request.Kind;
	Params.Query = Request.Query;
	Params.Keywords = Request.Keywords;
	Params.Categories = Request.Categories;
	Params.Articles = Request.Articles;
	Params.PinTypes = Request.PinTypes;
	Params.Origin = Request.Origin;
	Params.bIncludeDeprecated = Request.bIncludeDeprecated;
	Params.Limit = Request.Limit;
	Params.Offset = Request.Offset;

	FString SectionsError;
	if (!UFlowCatalogQuery::ParseSections(Request.Sections, Params.Sections, Params.bDescriptionOnly, SectionsError))
	{
		RaiseFlowGraphToolError(SectionsError);
		return Result;
	}

	FFlowCatalogQueryResult QueryResult;
	UFlowCatalogQuery::QueryCatalog(Params, QueryResult);

	if (!QueryResult.ErrorMessage.IsEmpty())
	{
		RaiseFlowGraphToolError(QueryResult.ErrorMessage);
		return Result;
	}

	for (const FFlowCatalogClassRow& Row : QueryResult.Nodes)
	{
		Result.Nodes.Add(CatalogRowToClassDetail(Row));
	}
	for (const FFlowCatalogClassRow& Row : QueryResult.Addons)
	{
		Result.Addons.Add(CatalogRowToClassDetail(Row));
	}

	Result.Shape.NodeCount = QueryResult.Shape.NodeCount;
	Result.Shape.AddonCount = QueryResult.Shape.AddonCount;
	Result.Shape.DocumentedCount = QueryResult.Shape.DocumentedCount;
	Result.Shape.DeprecatedCount = QueryResult.Shape.DeprecatedCount;
	AppendFacets(QueryResult.Shape.Categories, Result.Shape.Categories);
	AppendFacets(QueryResult.Shape.Keywords, Result.Shape.Keywords);
	AppendFacets(QueryResult.Shape.Tags, Result.Shape.Tags);
	AppendFacets(QueryResult.Shape.Articles, Result.Shape.Articles);

	for (const FFlowCatalogAmbiguousNameRow& Ambiguous : QueryResult.AmbiguousClassNames)
	{
		FFlowMCPAmbiguousClassName& MCPAmbiguous = Result.AmbiguousClassNames.AddDefaulted_GetRef();
		MCPAmbiguous.Stem = Ambiguous.Stem;
		MCPAmbiguous.Candidates = Ambiguous.Candidates;
	}

	Result.NodeCount = Result.Nodes.Num();
	Result.AddonCount = Result.Addons.Num();
	Result.TotalNodeCount = QueryResult.TotalNodeCount;
	Result.TotalAddonCount = QueryResult.TotalAddonCount;
	Result.bTruncated = QueryResult.bTruncated;
	Result.Sections = QueryResult.EffectiveSections;
	Result.UnresolvedClassNames = QueryResult.UnresolvedClassNames;
	return Result;
}

FFlowMCPSetFlowAgentDocResult UFlowMCPToolset::SetFlowAgentDoc(
	const FFlowMCPSetFlowAgentDocRequest& Request)
{
	FFlowMCPSetFlowAgentDocResult Result;

	FFlowAgentDocWriteRequest WriteRequest;
	WriteRequest.ClassName = Request.ClassName;
	WriteRequest.Guidance = Request.Guidance;
	WriteRequest.Tags = Request.Tags;
	WriteRequest.Articles = Request.Articles;

	FFlowMCPMutationReport MutationReport;
	const FFlowAgentDocWriteResult WriteResult = FFlowAgentDocWriter::Write(WriteRequest, Request.Mutation, MutationReport);

	if (!WriteResult.ErrorMessage.IsEmpty())
	{
		RaiseFlowGraphToolError(WriteResult.ErrorMessage);
		return Result;
	}

	Result.ClassPath = WriteResult.ClassPath;
	Result.Origin = WriteResult.Origin;
	Result.bPersisted = WriteResult.bPersisted;
	Result.SourceSnippet = WriteResult.SourceSnippet;
	Result.Note = WriteResult.Note;
	Result.Mutation = MutationReport;
	return Result;
}

FFlowMCPCreateFlowNodeBlueprintResult UFlowMCPToolset::CreateFlowNodeBlueprint(
	const FFlowMCPCreateFlowNodeBlueprintRequest& Request)
{
	FFlowMCPCreateFlowNodeBlueprintResult Result;

	FFlowNodeBlueprintCreateRequest CreateRequest;
	CreateRequest.PackagePath = Request.PackagePath;
	CreateRequest.AssetName = Request.AssetName;
	CreateRequest.ParentClass = Request.ParentClass;
	CreateRequest.DisplayName = Request.DisplayName;
	CreateRequest.Description = Request.Description;

	FFlowMCPMutationReport MutationReport;
	const FFlowNodeBlueprintCreateResult CreateResult =
		FFlowNodeBlueprintCreator::Create(CreateRequest, Request.Mutation, MutationReport);

	if (!CreateResult.ErrorMessage.IsEmpty())
	{
		RaiseFlowGraphToolError(CreateResult.ErrorMessage);
		return Result;
	}

	Result.AssetPath = CreateResult.AssetPath;
	Result.AssetClassPath = CreateResult.AssetClassPath;
	Result.GeneratedClassPath = CreateResult.GeneratedClassPath;
	Result.ParentClassPath = CreateResult.ParentClassPath;
	Result.bCompiled = CreateResult.bCompiled;
	Result.CompileErrors = CreateResult.CompileErrors;
	Result.bResolvedInCatalog = CreateResult.bResolvedInCatalog;
	Result.Note = CreateResult.Note;
	Result.Mutation = MutationReport;
	return Result;
}

FFlowMCPFindFlowNodeUsageResult UFlowMCPToolset::FindFlowNodeUsage(
	const FFlowMCPFindFlowNodeUsageRequest& Request)
{
	FFlowMCPFindFlowNodeUsageResult Result;

	if (Request.ClassName.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("FindFlowNodeUsage: ClassName cannot be empty."));
		return Result;
	}

	const FFlowNodeUsageResult Usage = FFlowNodeUsageIndex::Get().QueryUsage(
		Request.ClassName, Request.Limit, Request.bIncludeSnippets);

	if (!Usage.ErrorMessage.IsEmpty())
	{
		RaiseFlowGraphToolError(Usage.ErrorMessage);
		return Result;
	}

	Result.ClassPath = Usage.ClassPath;
	Result.InstanceCount = Usage.InstanceCount;
	Result.AssetCount = Usage.AssetCount;
	Result.Snippets = Usage.Snippets;

	for (const FFlowNodeUsageExample& Example : Usage.Examples)
	{
		FFlowMCPFlowNodeUsageExample& MCPExample = Result.Examples.AddDefaulted_GetRef();
		MCPExample.AssetPath = Example.AssetPath;
		MCPExample.InstanceCount = Example.InstanceCount;
		MCPExample.AssetNodeCount = Example.AssetNodeCount;
	}

	for (const FFlowNodeUsageCoAttachment& CoAttachment : Usage.CoAttachedAddOns)
	{
		FFlowMCPCatalogFacet& Facet = Result.CoAttachedAddOns.AddDefaulted_GetRef();
		Facet.Value = CoAttachment.ClassName.IsEmpty() ? CoAttachment.ClassPath : CoAttachment.ClassName;
		Facet.Count = CoAttachment.Count;
	}

	return Result;
}

FFlowMCPFindFlowNodesResult UFlowMCPToolset::FindFlowNodes(
	const FFlowMCPFindFlowNodesRequest& Request)
{
	FFlowMCPFindFlowNodesResult Result;
	if (Request.AssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("FindFlowNodes: AssetPath cannot be empty."));
		return Result;
	}

	UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.AssetPath);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("FindFlowNodes: Failed to load asset at path: %s"), *Request.AssetPath));
		return Result;
	}

	TArray<FFlowSubgraphNodeSummary> Summaries;
	FString ErrorMessage;
	if (!FFlowGraphSubgraphQuery::FindNodes(FlowAsset, Request.TitleFilter, Request.ClassFilter, Request.bEntryPointsOnly, Summaries, ErrorMessage))
	{
		RaiseFlowGraphToolError(ErrorMessage);
		return Result;
	}

	Result.AssetPath = Request.AssetPath;
	Result.Nodes.Reserve(Summaries.Num());
	for (const FFlowSubgraphNodeSummary& Summary : Summaries)
	{
		FFlowMCPNodeSummary NodeSummary;
		NodeSummary.NodeGuid = Summary.NodeGuid.ToString();
		NodeSummary.Title = Summary.Title;
		NodeSummary.ClassPath = Summary.ClassPath;
		NodeSummary.bIsEntryPoint = Summary.bIsEntryPoint;
		Result.Nodes.Add(NodeSummary);
	}

	Result.NodeCount = Result.Nodes.Num();

	return Result;
}

FFlowMCPExportFlowSubgraphResult UFlowMCPToolset::ExportFlowSubgraph(
	const FFlowMCPExportFlowSubgraphRequest& Request)
{
	FFlowMCPExportFlowSubgraphResult Result;
	if (Request.AssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("ExportFlowSubgraph: AssetPath cannot be empty."));
		return Result;
	}

	FGuid ParsedGuid;
	if (!FGuid::Parse(Request.StartNodeGuid, ParsedGuid))
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("ExportFlowSubgraph: StartNodeGuid is not a valid GUID: %s"), *Request.StartNodeGuid));
		return Result;
	}

	UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.AssetPath);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("ExportFlowSubgraph: Failed to load asset at path: %s"), *Request.AssetPath));
		return Result;
	}

	FString ExportedText;
	int32 NodeCount = 0;
	FString ErrorMessage;
	if (!FFlowGraphSubgraphQuery::ExportSubgraph(FlowAsset, ParsedGuid, ExportedText, NodeCount, ErrorMessage))
	{
		RaiseFlowGraphToolError(ErrorMessage);
		return Result;
	}

	Result.AssetPath = Request.AssetPath;
	Result.StartNodeGuid = ParsedGuid.ToString();
	Result.ExportedText = ExportedText;
	Result.NodeCount = NodeCount;
	Result.TextLength = ExportedText.Len();
	return Result;
}

FFlowMCPPlanFlowSubgraphFromSelectionResult UFlowMCPToolset::PlanFlowSubgraphFromSelection(
	const FFlowMCPPlanFlowSubgraphFromSelectionRequest& Request)
{
	FFlowMCPPlanFlowSubgraphFromSelectionResult Result;
	Result.TargetAssetPath = Request.TargetAssetPath;
	Result.NewAssetName = Request.NewAssetName;

	if (Request.TargetAssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("PlanFlowSubgraphFromSelection: TargetAssetPath cannot be empty."));
		return Result;
	}

	UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("PlanFlowSubgraphFromSelection: Failed to load asset at path: %s"),
			*Request.TargetAssetPath));
		return Result;
	}

	TArray<FGuid> SelectionGuids;
	FString ErrorMessage;
	if (!ParseSelectionGuids(Request.SelectionGuids, SelectionGuids, ErrorMessage))
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("PlanFlowSubgraphFromSelection: %s"),
			*ErrorMessage));
		return Result;
	}

	FlowSubgraphSelection::FSelectionPlanSummary PlanSummary;
	if (!FlowSubgraphSelection::PlanSelection(
		FlowAsset,
		SelectionGuids,
		Request.NewAssetName,
		PlanSummary,
		ErrorMessage))
	{
		if (!ErrorMessage.IsEmpty())
		{
			RaiseFlowGraphToolError(FString::Printf(
				TEXT("PlanFlowSubgraphFromSelection: %s"),
				*ErrorMessage));
			return Result;
		}
	}

	CopySubgraphSelectionPlan(PlanSummary, Result.Plan);
	return Result;
}

FFlowMCPCreateFlowSubgraphFromSelectionResult UFlowMCPToolset::CreateFlowSubgraphFromSelection(
	const FFlowMCPCreateFlowSubgraphFromSelectionRequest& Request)
{
	FFlowMCPCreateFlowSubgraphFromSelectionResult Result;
	Result.TargetAssetPath = Request.TargetAssetPath;
	Result.Mutation.bDryRun = Request.Mutation.bDryRun;

	if (!Request.Mutation.bUseTransaction)
	{
		RaiseFlowGraphToolError(
			TEXT("CreateFlowSubgraphFromSelection requires Mutation.bUseTransaction=true."));
		return Result;
	}

	if (GEditor != nullptr && GEditor->Trans != nullptr && GEditor->Trans->IsActive())
	{
		RaiseFlowGraphToolError(
			TEXT("CreateFlowSubgraphFromSelection cannot run inside an existing editor transaction."));
		return Result;
	}

	if (Request.TargetAssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("CreateFlowSubgraphFromSelection: TargetAssetPath cannot be empty."));
		return Result;
	}

	UFlowAsset* SourceAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
	if (!SourceAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("CreateFlowSubgraphFromSelection: Failed to load asset at path: %s"),
			*Request.TargetAssetPath));
		return Result;
	}

	TArray<FGuid> SelectionGuids;
	FString ErrorMessage;
	if (!ParseSelectionGuids(Request.SelectionGuids, SelectionGuids, ErrorMessage))
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("CreateFlowSubgraphFromSelection: %s"),
			*ErrorMessage));
		return Result;
	}

	FlowSubgraphSelection::FSelectionPlanSummary PlanSummary;
	if (!FlowSubgraphSelection::PlanSelection(
		SourceAsset,
		SelectionGuids,
		Request.NewAssetName,
		PlanSummary,
		ErrorMessage))
	{
		if (!ErrorMessage.IsEmpty())
		{
			RaiseFlowGraphToolError(FString::Printf(
				TEXT("CreateFlowSubgraphFromSelection: %s"),
				*ErrorMessage));
			return Result;
		}
	}

	CopySubgraphSelectionPlan(PlanSummary, Result.Plan);
	if (!PlanSummary.bCanApply)
	{
		return Result;
	}

	FFlowMCPMutationContext MutationContext(Request.Mutation);
	if (!BeginMutation(SourceAsset, MutationContext))
	{
		return Result;
	}

	MutationContext.Modify(SourceAsset);
	UFlowAsset* NewAsset = nullptr;
	UFlowGraphNode* SubgraphNode = nullptr;
	if (!FlowSubgraphSelection::ApplySelection(
		SourceAsset,
		SelectionGuids,
		Request.NewAssetName,
		PlanSummary,
		NewAsset,
		SubgraphNode,
		ErrorMessage))
	{
		MutationContext.Abort();
		DiscardCreatedSubgraph(NewAsset);
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("CreateFlowSubgraphFromSelection: %s"),
			*ErrorMessage));
		return Result;
	}

	CopySubgraphSelectionPlan(PlanSummary, Result.Plan);
	Result.NewAssetPath = IsValid(NewAsset) ? NewAsset->GetPathName() : FString();
	Result.SubgraphNodeGuid = IsValid(SubgraphNode) ? SubgraphNode->NodeGuid.ToString() : FString();

	MutationContext.RegisterDryRunRollback([NewAsset]()
	{
		DiscardCreatedSubgraph(NewAsset);
	});

	if (!MutationContext.PrepareAsset(NewAsset, ErrorMessage) ||
		!MutationContext.FinalizeAsset(NewAsset, nullptr, false, ErrorMessage) ||
		!MutationContext.FinalizeAsset(SourceAsset, nullptr, false, ErrorMessage) ||
		!MutationContext.Complete(Result.Mutation, ErrorMessage))
	{
		MutationContext.Abort();
		if (!Request.Mutation.bDryRun)
		{
			DiscardCreatedSubgraph(NewAsset);
		}
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("CreateFlowSubgraphFromSelection: %s"),
			*ErrorMessage));
		return Result;
	}

	Result.bApplySucceeded = true;
	return Result;
}

FFlowMCPFindOrCreateFlowNodeResult UFlowMCPToolset::FindOrCreateFlowNode(
	const FFlowMCPFindOrCreateFlowNodeRequest& Request)
{
	FFlowMCPFindOrCreateFlowNodeResult Result;
	if (Request.TargetAssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("FindOrCreateFlowNode: TargetAssetPath cannot be empty."));
		return Result;
	}

	if (Request.NodeType.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("FindOrCreateFlowNode: NodeType cannot be empty."));
		return Result;
	}

	Result.AssetPath = Request.TargetAssetPath;
	Result.Mutation.bDryRun = Request.Mutation.bDryRun;

	// Dry runs never touch the asset - the library's own bDryRun handles that, so there is
	// nothing for FFlowMCPMutationContext to check out, transact, or roll back.
	if (Request.Mutation.bDryRun)
	{
		FGuid NodeGuid;
		bool bCreated = false;
		FString ErrorMessage;
		if (!FFlowGraphReconciler::FindOrCreateNode(Request.TargetAssetPath, Request.NodeType, Request.MatchProperties, true, NodeGuid, bCreated, ErrorMessage))
		{
			RaiseFlowGraphToolError(ErrorMessage);
			return Result;
		}
		Result.bCreated = bCreated;
		Result.NodeGuid = NodeGuid.ToString();
		return Result;
	}

	UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("FindOrCreateFlowNode: Failed to load asset at path: %s"), *Request.TargetAssetPath));
		return Result;
	}

	FFlowMCPMutationContext MutationContext(Request.Mutation);
	if (!BeginMutation(FlowAsset, MutationContext))
	{
		return Result;
	}

	MutationContext.Modify(FlowAsset);

	FGuid NodeGuid;
	bool bCreated = false;
	FString ErrorMessage;
	if (!FFlowGraphReconciler::FindOrCreateNode(Request.TargetAssetPath, Request.NodeType, Request.MatchProperties, false, NodeGuid, bCreated, ErrorMessage))
	{
		MutationContext.Abort();
		RaiseFlowGraphToolError(ErrorMessage);
		return Result;
	}

	if (!CompleteMutation(FlowAsset, MutationContext, Result.Mutation))
	{
		return Result;
	}

	Result.bCreated = bCreated;
	Result.NodeGuid = NodeGuid.ToString();
	return Result;
}

FFlowMCPAutoFormatFlowGraphResult UFlowMCPToolset::AutoFormatFlowGraph(
	const FFlowMCPAutoFormatFlowGraphRequest& Request)
{
	FFlowMCPAutoFormatFlowGraphResult Result;
	if (Request.TargetAssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("AutoFormatFlowGraph: TargetAssetPath cannot be empty."));
		return Result;
	}

	UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("AutoFormatFlowGraph: Failed to load asset at path: %s"), *Request.TargetAssetPath));
		return Result;
	}

	// Empty SelectionGuids means "the whole graph"; resolve the given strings to real GUIDs that
	// exist on this asset, reporting anything unresolved rather than silently dropping it.
	TSet<FGuid> TargetGuids;
	TArray<FString> UnresolvedGuids;
	for (const FString& GuidString : Request.SelectionGuids)
	{
		FGuid ParsedGuid;
		if (FGuid::Parse(GuidString, ParsedGuid) && FlowAsset->GetNode(ParsedGuid))
		{
			TargetGuids.Add(ParsedGuid);
		}
		else
		{
			UnresolvedGuids.Add(GuidString);
		}
	}

	TMap<FGuid, FIntPoint> Positions;
	UFlowGraphEditorLayout::ComputeAutoFormatPositions(FlowAsset, TargetGuids, Positions);

	FFlowMCPMutationContext MutationContext(Request.Mutation);
	if (!BeginMutation(FlowAsset, MutationContext))
	{
		return Result;
	}

	if (!MutationContext.IsDryRun())
	{
		MutationContext.Modify(FlowAsset);
		UFlowGraphEditorLayout::ApplyPositionsToExistingGraph(FlowAsset, Positions);
	}

	if (!CompleteMutation(FlowAsset, MutationContext, Result.Mutation))
	{
		return Result;
	}

	Result.AssetPath = Request.TargetAssetPath;
	Result.NodeCount = Positions.Num();
	for (const TPair<FGuid, FIntPoint>& Pair : Positions)
	{
		Result.Positions.Add(Pair.Key.ToString(), Pair.Value);
	}

	Result.UnresolvedGuids = UnresolvedGuids;
	return Result;
}

FFlowMCPReconstructFlowGraphResult UFlowMCPToolset::ReconstructFlowGraph(
	const FFlowMCPReconstructFlowGraphRequest& Request)
{
	FFlowMCPReconstructFlowGraphResult Result;
	if (Request.TargetAssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("ReconstructFlowGraph: TargetAssetPath cannot be empty."));
		return Result;
	}

	UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
	if (!FlowAsset)
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("ReconstructFlowGraph: Failed to load asset at path: %s"), *Request.TargetAssetPath));
		return Result;
	}

	FFlowMCPMutationContext MutationContext(Request.Mutation);
	if (!BeginMutation(FlowAsset, MutationContext))
	{
		return Result;
	}

	MutationContext.Modify(FlowAsset);

	// Counted before the rebuild, which is what removes them - reported so a caller can tell a
	// repair from a no-op rebuild.
	const int32 OrphanedEditorNodesRemoved = UFlowGraphRegrapher::PruneOrphanedEditorNodes(FlowAsset);

	if (!UFlowGraphRegrapher::RegraphFlowAsset(FlowAsset))
	{
		MutationContext.Abort();
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("ReconstructFlowGraph: Failed to rebuild the editor graph for '%s'."),
			*Request.TargetAssetPath));
		return Result;
	}

#if WITH_EDITOR
	FlowAsset->RebuildCustomInterfaceLists();
#endif

	if (!CompleteMutation(FlowAsset, MutationContext, Result.Mutation))
	{
		return Result;
	}

	Result.AssetPath = Request.TargetAssetPath;
	Result.NodeCount = FlowAsset->GetNodes().Num();
	Result.OrphanedEditorNodesRemoved = OrphanedEditorNodesRemoved;
	UFlowGraphRegrapher::CollectGraphParityIssues(FlowAsset, Result.GraphIntegrityIssues);
	return Result;
}

FFlowMCPExportFlowCatalogResult UFlowMCPToolset::ExportFlowCatalog(
	const FFlowMCPExportFlowCatalogRequest& Request)
{
	FFlowMCPExportFlowCatalogResult Result;
	if (Request.OutputFilePath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("ExportFlowCatalog: OutputFilePath cannot be empty."));
		return Result;
	}

	Result.OutputFilePath = Request.OutputFilePath;
	Result.bSucceeded = UFlowCatalogQuery::ExportFlowCatalogToFile(Request.OutputFilePath, Request.AssetClassName);
	if (!Result.bSucceeded)
	{
		RaiseFlowGraphToolError(FString::Printf(
			TEXT("ExportFlowCatalog: Failed to export catalog to '%s'."),
			*Request.OutputFilePath));
	}

	return Result;
}

FFlowMCPDiffFlowAssetResult UFlowMCPToolset::DiffFlowAsset(
	const FFlowMCPDiffFlowAssetRequest& Request)
{
	FFlowMCPDiffFlowAssetResult Result;
	if (Request.Old.IsEmpty() || Request.New.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("Old and New must both be non-empty (an asset path or a raw FlowCourier text blob)."));
		return Result;
	}

	FString OldText, NewText, ErrorMessage;
	if (!ResolveDiffInput(Request.Old, OldText, Result.OldSource, ErrorMessage))
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("Old: %s"), *ErrorMessage));
		return Result;
	}

	if (!ResolveDiffInput(Request.New, NewText, Result.NewSource, ErrorMessage))
	{
		RaiseFlowGraphToolError(FString::Printf(TEXT("New: %s"), *ErrorMessage));
		return Result;
	}

	FFlowGraphDiffResult DiffResult;
	if (!UFlowGraphDiff::ComputeDiff(OldText, NewText, DiffResult, ErrorMessage))
	{
		RaiseFlowGraphToolError(ErrorMessage);
		return Result;
	}

	Result.bHasDifferences = DiffResult.HasAnyDifference();
	Result.AddedNodes = GuidArrayToStringArray(DiffResult.AddedNodeGuids);
	Result.RemovedNodes = GuidArrayToStringArray(DiffResult.RemovedNodeGuids);

	Result.ChangedNodes.Reserve(DiffResult.ChangedNodes.Num());
	for (const FFlowGraphDiffChangedNode& ChangedNode : DiffResult.ChangedNodes)
	{
		FFlowMCPChangedNode FlowMCPChangedNode;
		FlowMCPChangedNode.NodeGuid = ChangedNode.NodeGuid.ToString();
		FlowMCPChangedNode.ChangedProperties = ChangedNode.ChangedProperties;
		FlowMCPChangedNode.bPosChanged = ChangedNode.bPosChanged;
		if (ChangedNode.bPosChanged)
		{
			FlowMCPChangedNode.OldPos = FString::Printf(TEXT("%d,%d"), ChangedNode.OldPos.X, ChangedNode.OldPos.Y);
			FlowMCPChangedNode.NewPos = FString::Printf(TEXT("%d,%d"), ChangedNode.NewPos.X, ChangedNode.NewPos.Y);
		}

		FlattenAddonDiffs(ChangedNode.AddedAddOns, TEXT("added"), FlowMCPChangedNode.NodeGuid, FlowMCPChangedNode.AddonDiffs);
		FlattenAddonDiffs(ChangedNode.RemovedAddOns, TEXT("removed"), FlowMCPChangedNode.NodeGuid, FlowMCPChangedNode.AddonDiffs);
		FlattenAddonDiffs(ChangedNode.ChangedAddOns, TEXT("changed"), FlowMCPChangedNode.NodeGuid, FlowMCPChangedNode.AddonDiffs);
		Result.ChangedNodes.Add(FlowMCPChangedNode);
	}

	Result.AddedConnections = ToFlowMCPConnectionRefArray(DiffResult.AddedConnections);
	Result.RemovedConnections = ToFlowMCPConnectionRefArray(DiffResult.RemovedConnections);
	return Result;
}

FFlowMCPApplyFlowPatchResult UFlowMCPToolset::ApplyFlowPatch(
	const FFlowMCPApplyFlowPatchRequest& Request)
{
	FFlowMCPApplyFlowPatchResult Result;
	if (Request.MutationText.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("MutationText cannot be empty."));
		return Result;
	}

	if (Request.TargetAssetPath.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("TargetAssetPath cannot be empty."));
		return Result;
	}

	if (!Request.TargetAssetPath.StartsWith(TEXT("/")))
	{
		RaiseFlowGraphToolError(TEXT("TargetAssetPath must start with '/' (for example, '/Game/MyFlows/MyFlow')."));
		return Result;
	}

	Result.AssetPath = Request.TargetAssetPath;
	Result.Mutation.bDryRun = Request.Mutation.bDryRun;

	FString ComputeError;
	TSharedPtr<FFlowReconcileExecutionPlan> ExecutionPlan = FFlowGraphReconciler::ComputeReconcilePlan(Request.TargetAssetPath, Request.MutationText, ComputeError);
	if (!ExecutionPlan)
	{
		RaiseFlowGraphToolError(ComputeError.IsEmpty() ? TEXT("apply_flow_patch: failed to compute plan") : ComputeError);
		return Result;
	}

	FFlowReconcileResult ReconcileResult;
	FString ExecuteError;
	if (Request.Mutation.bDryRun)
	{
		// Dry runs only compute the plan; the asset is never loaded for mutation, so there is
		// nothing for FFlowMCPMutationContext to check out, transact, or roll back.
		ReconcileResult.Plan = ExecutionPlan->Plan;
		ReconcileResult.AliasMap = ExecutionPlan->AliasMap;
		ReconcileResult.ValidationFindings = ExecutionPlan->ValidationFindings;
		ReconcileResult.NodesTouched = ExecutionPlan->NodesTouched;
		ReconcileResult.NodesPreserved = ExecutionPlan->NodesPreserved;
		ReconcileResult.InputSizeBytes = ExecutionPlan->InputSizeBytes;
		ReconcileResult.bSuccess = FFlowGraphValidation::HasNoErrors(ReconcileResult.ValidationFindings);
	}
	else
	{
		UFlowAsset* FlowAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
		FFlowMCPMutationContext MutationContext(Request.Mutation);
		const bool bHasExistingAsset = (FlowAsset != nullptr);

		if (bHasExistingAsset)
		{
			if (!BeginMutation(FlowAsset, MutationContext))
			{
				return Result;
			}
			MutationContext.Modify(FlowAsset);
		}
		else
		{
			// New-asset creation path: ExecuteReconcilePlan below will import and regraph a
			// brand-new FlowAsset itself, so there is no pre-existing UObject to check out or
			// Modify() ahead of time - just open the transaction now and PrepareAsset/checkout
			// the newly-created object once it exists, below.
			FString BeginError;
			if (!MutationContext.Begin(BeginError))
			{
				RaiseFlowGraphToolError(BeginError);
				return Result;
			}
		}

		ReconcileResult = FFlowGraphReconciler::ExecuteReconcilePlan(*ExecutionPlan, Request.TargetAssetPath, ExecuteError);

		if (!bHasExistingAsset && ReconcileResult.bSuccess)
		{
			FlowAsset = LoadFlowAssetOrNull(Request.TargetAssetPath);
			if (!FlowAsset)
			{
				// The reconciler reports the new asset was created, but it cannot be reloaded at
				// TargetAssetPath to finalize the mutation. Failing loudly here, rather than
				// falling through to a silent Abort() below, is what stops this from surfacing as
				// a reported success that persisted nothing.
				MutationContext.Abort();
				RaiseFlowGraphToolError(FString::Printf(
					TEXT("apply_flow_patch: created the new asset but could not reload it at '%s' to finalize the mutation."),
					*Request.TargetAssetPath));
				return Result;
			}

			FString PrepareError;
			if (!MutationContext.PrepareAsset(FlowAsset, PrepareError))
			{
				MutationContext.Abort();
				RaiseFlowGraphToolError(PrepareError);
				return Result;
			}
		}

		if (ReconcileResult.bSuccess && FlowAsset)
		{
			if (!CompleteMutation(FlowAsset, MutationContext, Result.Mutation))
			{
				return Result;
			}
		}
		else
		{
			MutationContext.Abort();
		}
	}

	Result.bApplySucceeded = ReconcileResult.bSuccess;
	Result.Plan = ToFlowMCPReconcilePlan(ReconcileResult.Plan);

	for (const TPair<FString, FGuid>& AliasPair : ReconcileResult.AliasMap)
	{
		Result.AliasMap.Add(AliasPair.Key, AliasPair.Value.ToString());
	}

	Result.Findings = ReconcileResult.Findings;
	Result.ValidationFindings = ToFlowMCPValidationFindings(ReconcileResult.ValidationFindings, Result.bHasBlockingError);

	Result.Metrics.NodesTouched = ReconcileResult.NodesTouched;
	Result.Metrics.NodesPreserved = ReconcileResult.NodesPreserved;
	Result.Metrics.InputSizeBytes = ReconcileResult.InputSizeBytes;

	// A non-dry-run apply that reports success with a non-empty plan must have actually
	// modified a package. A reported success that persisted nothing is a contract violation,
	// not a no-op, and this catches the whole class of "succeeded but changed nothing" defect
	// regardless of which code path produced it.
	if (Result.bApplySucceeded && !Request.Mutation.bDryRun &&
		!ReconcileResult.Plan.IsEmpty() && Result.Mutation.ModifiedPackages.IsEmpty())
	{
		Result.bApplySucceeded = false;
		const FString PostConditionError = TEXT(
			"apply_flow_patch reported a non-empty plan but modified no package.");
		Result.Findings.Add(PostConditionError);
		RaiseFlowGraphToolError(PostConditionError);
		return Result;
	}

	// A successful apply leaves the editor graph and the runtime node map in agreement. Anything
	// reported here is damage this call either caused or failed to repair, and it must travel with
	// the result: the plan and the export both read the runtime map, so neither can show it.
	if (!Request.Mutation.bDryRun)
	{
		if (UFlowAsset* AppliedAsset = LoadFlowAssetOrNull(Request.TargetAssetPath))
		{
			TArray<FString> ParityIssues;
			UFlowGraphRegrapher::CollectGraphParityIssues(AppliedAsset, ParityIssues);
			for (const FString& ParityIssue : ParityIssues)
			{
				Result.Findings.Add(FString::Printf(TEXT("Graph integrity: %s"), *ParityIssue));
			}
		}
	}

	// On failure, the plan/findings/validation_findings above are still populated - a caller
	// needs the structured findings even when the apply was blocked. A validation block is not
	// an exceptional failure - it is the expected "here is what would have changed and why it
	// didn't" response - so it must not raise: the MCP transport converts a raised error into an
	// exception and discards this return value, which is exactly the payload the caller needs.
	// Only a genuine execution error (not surfaced as a validation finding) raises.
	if (!ReconcileResult.bSuccess && !Result.bHasBlockingError)
	{
		RaiseFlowGraphToolError(ExecuteError.IsEmpty() ? TEXT("apply_flow_patch failed") : ExecuteError);
	}

	return Result;
}

FFlowMCPReplaceFlowNodeClassResult UFlowMCPToolset::ReplaceFlowNodeClass(
	const FFlowMCPReplaceFlowNodeClassRequest& Request)
{
	FFlowMCPReplaceFlowNodeClassResult Result;
	Result.AssetPath = Request.AssetPath;
	Result.NodeGuid = Request.NodeGuid;
	Result.Mutation.bDryRun = Request.Mutation.bDryRun;

	FFlowNodeClassReplacementPlan Plan;
	if (!FFlowNodeClassReplacement::BuildPlan(Request, Plan, Result.Findings))
	{
		return Result;
	}
	Result.bCanReplace = true;
	Result.PreviousClass = Plan.Original->GetClass()->GetPathName();
	Result.NewClass = Plan.TargetClass->GetPathName();
	for (const FGuid& Guid : Plan.ReplacedAddOnGuids)
	{
		Result.ReplacedAddOnGuids.Add(Guid.ToString(EGuidFormats::Digits));
	}
	if (Request.Mutation.bDryRun || !Plan.bNeedsReplacement)
	{
		return Result;
	}
	if (!Request.Mutation.bUseTransaction || (GEditor && GEditor->Trans && GEditor->Trans->IsActive()))
	{
		Result.Findings.Add(TEXT("Class replacement requires bUseTransaction=true and no ambient transaction to guarantee rollback."));
		Result.bCanReplace = false;
		return Result;
	}
	FFlowMCPMutationContext Context(Request.Mutation);
	if (!BeginMutation(Plan.Asset, Context))
	{
		Result.bCanReplace = false;
		return Result;
	}
	Context.Modify(Plan.Asset);
	Context.Modify(Plan.Asset->GetGraph());
	Context.Modify(Plan.EditorNode);
	for (const TPair<FGuid, UFlowNode*>& Entry : Plan.Asset->GetNodes())
	{
		Context.Modify(Entry.Value);
	}
	if (!FFlowNodeClassReplacement::ApplyPlan(Plan, Result.Findings))
	{
		Context.Abort();
		Result.bCanReplace = false;
		return Result;
	}
	if (!CompleteMutation(Plan.Asset, Context, Result.Mutation))
	{
		Result.bCanReplace = false;
		return Result;
	}
	Result.bReplaced = true;
	return Result;
}

namespace
{
	FFlowMCPCourierFieldGrammar MakeCourierField(
		const FString& FieldName,
		EFlowMCPCourierFieldLegality Legality,
		const FString& Notes)
	{
		FFlowMCPCourierFieldGrammar Field;
		Field.FieldName = FieldName;
		Field.Legality = Legality;
		Field.Notes = Notes;
		return Field;
	}

	FFlowMCPCourierFieldGroup MakeExactlyOneOfGroup(std::initializer_list<FString> FieldNames, const FString& Notes)
	{
		FFlowMCPCourierFieldGroup Group;
		Group.FieldNames = FieldNames;
		Group.Notes = Notes;
		return Group;
	}

	// Identity fields (guid/newAlias) on Upsert* kinds: exactly one must be set, either is legal.
	void AppendUpsertIdentityFields(TArray<FFlowMCPCourierFieldGrammar>& Fields, const FString& ObjectNoun)
	{
		Fields.Add(MakeCourierField(TEXT("guid"), EFlowMCPCourierFieldLegality::Optional,
			FString::Printf(TEXT("Exactly one of guid/newAlias must be set; guid targets an existing %s."), *ObjectNoun)));
		Fields.Add(MakeCourierField(TEXT("newAlias"), EFlowMCPCourierFieldLegality::Optional,
			FString::Printf(TEXT("Exactly one of guid/newAlias must be set; newAlias names a %s being created in this document."), *ObjectNoun)));
	}

	// Identity fields on Delete* kinds: newAlias is illegal (bCanCreate is false), so guid is the
	// only way to satisfy the identity constraint.
	void AppendDeleteIdentityFields(TArray<FFlowMCPCourierFieldGrammar>& Fields, const FString& ObjectNoun)
	{
		Fields.Add(MakeCourierField(TEXT("guid"), EFlowMCPCourierFieldLegality::Required,
			FString::Printf(TEXT("Identifies the existing %s to delete."), *ObjectNoun)));
		Fields.Add(MakeCourierField(TEXT("newAlias"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("newAlias is only legal on UpsertNode/UpsertAddon.")));
	}

	void AppendAddonParentFields(TArray<FFlowMCPCourierFieldGrammar>& Fields)
	{
		Fields.Add(MakeCourierField(TEXT("parentGuid"), EFlowMCPCourierFieldLegality::Optional,
			TEXT("Exactly one of parentGuid/parentAlias must be set; the owning node's GUID, or the parent addon's GUID for a nested addon-of-addon.")));
		Fields.Add(MakeCourierField(TEXT("parentAlias"), EFlowMCPCourierFieldLegality::Optional,
			TEXT("Exactly one of parentGuid/parentAlias must be set; may reference another op's newAlias in the same document.")));
	}

	void AppendIllegalParentFields(TArray<FFlowMCPCourierFieldGrammar>& Fields)
	{
		Fields.Add(MakeCourierField(TEXT("parentGuid"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("parentGuid/parentAlias are only legal on UpsertAddon/DeleteAddon.")));
		Fields.Add(MakeCourierField(TEXT("parentAlias"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("parentGuid/parentAlias are only legal on UpsertAddon/DeleteAddon.")));
	}

	void AppendIllegalConnectionEndpointFields(TArray<FFlowMCPCourierFieldGrammar>& Fields)
	{
		Fields.Add(MakeCourierField(TEXT("source"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("source/target are only legal on AddConnection/RemoveConnection.")));
		Fields.Add(MakeCourierField(TEXT("target"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("source/target are only legal on AddConnection/RemoveConnection.")));
	}

	FFlowMCPCourierOpKindGrammar MakeUpsertNodeGrammar()
	{
		FFlowMCPCourierOpKindGrammar Kind;
		Kind.Kind = TEXT("UpsertNode");
		Kind.Description = TEXT("Create or update a node.");
		AppendUpsertIdentityFields(Kind.Fields, TEXT("node"));
		AppendIllegalParentFields(Kind.Fields);
		Kind.Fields.Add(MakeCourierField(TEXT("type"), EFlowMCPCourierFieldLegality::RequiredOnCreate,
			TEXT("Required when creating (newAlias set); on an update (guid set) it is optional, but if present must equal the existing class or it is a ClassMismatch error.")));
		Kind.Fields.Add(MakeCourierField(TEXT("properties"), EFlowMCPCourierFieldLegality::Optional,
			TEXT("Merge semantics: an omitted key leaves the existing value unchanged. Values are UE property export-text strings.")));
		Kind.Fields.Add(MakeCourierField(TEXT("inputPins"), EFlowMCPCourierFieldLegality::Optional, FString()));
		Kind.Fields.Add(MakeCourierField(TEXT("outputPins"), EFlowMCPCourierFieldLegality::Optional, FString()));
		Kind.Fields.Add(MakeCourierField(TEXT("bHasPosition"), EFlowMCPCourierFieldLegality::Optional,
			TEXT("Advisory editor position; pairs with position.")));
		Kind.Fields.Add(MakeCourierField(TEXT("position"), EFlowMCPCourierFieldLegality::Optional, FString()));
		Kind.Fields.Add(MakeCourierField(TEXT("comment"), EFlowMCPCourierFieldLegality::Optional, FString()));
		Kind.Fields.Add(MakeCourierField(TEXT("bReplaceAddons"), EFlowMCPCourierFieldLegality::Optional,
			TEXT("Default false is merge: unmentioned addons are left alone. True makes this op's UpsertAddon children (direct or transitive) the complete authoritative addon list; any existing addon not reached that way is deleted.")));
		AppendIllegalConnectionEndpointFields(Kind.Fields);
		Kind.ExactlyOneOfGroups.Add(MakeExactlyOneOfGroup({TEXT("guid"), TEXT("newAlias")},
			TEXT("guid targets an existing node; newAlias names a node being created in this document.")));
		return Kind;
	}

	FFlowMCPCourierOpKindGrammar MakeUpsertAddonGrammar()
	{
		FFlowMCPCourierOpKindGrammar Kind;
		Kind.Kind = TEXT("UpsertAddon");
		Kind.Description = TEXT("Create or update an addon on a node, or on another addon (addon-of-addon).");
		AppendUpsertIdentityFields(Kind.Fields, TEXT("addon"));
		AppendAddonParentFields(Kind.Fields);
		Kind.Fields.Add(MakeCourierField(TEXT("type"), EFlowMCPCourierFieldLegality::RequiredOnCreate,
			TEXT("Required when creating (newAlias set); on an update (guid set) it is optional, but if present must equal the existing class or it is a ClassMismatch error.")));
		Kind.Fields.Add(MakeCourierField(TEXT("properties"), EFlowMCPCourierFieldLegality::Optional,
			TEXT("Merge semantics: an omitted key leaves the existing value unchanged. Values are UE property export-text strings.")));
		Kind.Fields.Add(MakeCourierField(TEXT("inputPins"), EFlowMCPCourierFieldLegality::Optional, FString()));
		Kind.Fields.Add(MakeCourierField(TEXT("outputPins"), EFlowMCPCourierFieldLegality::Optional, FString()));
		Kind.Fields.Add(MakeCourierField(TEXT("bHasPosition"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		Kind.Fields.Add(MakeCourierField(TEXT("position"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		Kind.Fields.Add(MakeCourierField(TEXT("comment"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		Kind.Fields.Add(MakeCourierField(TEXT("bReplaceAddons"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		AppendIllegalConnectionEndpointFields(Kind.Fields);
		Kind.ExactlyOneOfGroups.Add(MakeExactlyOneOfGroup({TEXT("guid"), TEXT("newAlias")},
			TEXT("guid targets an existing addon; newAlias names an addon being created in this document.")));
		Kind.ExactlyOneOfGroups.Add(MakeExactlyOneOfGroup({TEXT("parentGuid"), TEXT("parentAlias")},
			TEXT("The owning node's GUID, or the parent addon's GUID for a nested addon-of-addon; parentAlias may reference another op's newAlias in the same document.")));
		return Kind;
	}

	FFlowMCPCourierOpKindGrammar MakeDeleteNodeGrammar()
	{
		FFlowMCPCourierOpKindGrammar Kind;
		Kind.Kind = TEXT("DeleteNode");
		Kind.Description = TEXT("Delete an existing node.");
		AppendDeleteIdentityFields(Kind.Fields, TEXT("node"));
		AppendIllegalParentFields(Kind.Fields);
		Kind.Fields.Add(MakeCourierField(TEXT("type"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("type is only legal on UpsertNode/UpsertAddon.")));
		Kind.Fields.Add(MakeCourierField(TEXT("properties/inputPins/outputPins"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("properties/inputPins/outputPins are only legal on UpsertNode/UpsertAddon.")));
		Kind.Fields.Add(MakeCourierField(TEXT("bHasPosition/position/comment/bReplaceAddons"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		AppendIllegalConnectionEndpointFields(Kind.Fields);
		return Kind;
	}

	FFlowMCPCourierOpKindGrammar MakeDeleteAddonGrammar()
	{
		FFlowMCPCourierOpKindGrammar Kind;
		Kind.Kind = TEXT("DeleteAddon");
		Kind.Description = TEXT("Delete an existing addon.");
		AppendDeleteIdentityFields(Kind.Fields, TEXT("addon"));
		AppendAddonParentFields(Kind.Fields);
		Kind.Fields.Add(MakeCourierField(TEXT("type"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("type is only legal on UpsertNode/UpsertAddon.")));
		Kind.Fields.Add(MakeCourierField(TEXT("properties/inputPins/outputPins"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("properties/inputPins/outputPins are only legal on UpsertNode/UpsertAddon.")));
		Kind.Fields.Add(MakeCourierField(TEXT("bHasPosition/position/comment/bReplaceAddons"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		AppendIllegalConnectionEndpointFields(Kind.Fields);
		Kind.ExactlyOneOfGroups.Add(MakeExactlyOneOfGroup({TEXT("parentGuid"), TEXT("parentAlias")},
			TEXT("The owning node's GUID, or the parent addon's GUID for a nested addon-of-addon; parentAlias may reference another op's newAlias in the same document.")));
		return Kind;
	}

	FFlowMCPCourierOpKindGrammar MakeConnectionGrammar(const FString& KindName, const FString& Description)
	{
		FFlowMCPCourierOpKindGrammar Kind;
		Kind.Kind = KindName;
		Kind.Description = Description;
		Kind.Fields.Add(MakeCourierField(TEXT("guid/newAlias"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("guid/newAlias are not legal on a connection op.")));
		AppendIllegalParentFields(Kind.Fields);
		Kind.Fields.Add(MakeCourierField(TEXT("type"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("type is only legal on UpsertNode/UpsertAddon.")));
		Kind.Fields.Add(MakeCourierField(TEXT("properties/inputPins/outputPins"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("properties/inputPins/outputPins are only legal on UpsertNode/UpsertAddon.")));
		Kind.Fields.Add(MakeCourierField(TEXT("bHasPosition/position/comment/bReplaceAddons"), EFlowMCPCourierFieldLegality::Illegal,
			TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode.")));
		Kind.Fields.Add(MakeCourierField(TEXT("source"), EFlowMCPCourierFieldLegality::Required,
			TEXT("Exactly one of source.nodeGuid/source.nodeAlias must be set; identifies the connection's origin pin.")));
		Kind.Fields.Add(MakeCourierField(TEXT("target"), EFlowMCPCourierFieldLegality::Required,
			TEXT("Exactly one of target.nodeGuid/target.nodeAlias must be set; identifies the connection's destination pin.")));
		Kind.ExactlyOneOfGroups.Add(MakeExactlyOneOfGroup({TEXT("source.nodeGuid"), TEXT("source.nodeAlias")},
			TEXT("Identifies the connection's origin node.")));
		Kind.ExactlyOneOfGroups.Add(MakeExactlyOneOfGroup({TEXT("target.nodeGuid"), TEXT("target.nodeAlias")},
			TEXT("Identifies the connection's destination node.")));
		return Kind;
	}
}

FFlowMCPDescribeCourierGrammarResult UFlowMCPToolset::DescribeCourierGrammar(
	const FFlowMCPDescribeCourierGrammarRequest& /*Request*/)
{
	FFlowMCPDescribeCourierGrammarResult Result;

	Result.Kinds.Add(MakeUpsertNodeGrammar());
	Result.Kinds.Add(MakeDeleteNodeGrammar());
	Result.Kinds.Add(MakeUpsertAddonGrammar());
	Result.Kinds.Add(MakeDeleteAddonGrammar());
	Result.Kinds.Add(MakeConnectionGrammar(TEXT("AddConnection"), TEXT("Add a connection between two node/addon pins.")));
	Result.Kinds.Add(MakeConnectionGrammar(TEXT("RemoveConnection"), TEXT("Remove a connection between two node/addon pins.")));

	Result.DocumentFields.Add(MakeCourierField(TEXT("formatVersion"), EFlowMCPCourierFieldLegality::Required,
		TEXT("Must be 2. Any other value is a hard error - there is no silent version coercion.")));
	Result.DocumentFields.Add(MakeCourierField(TEXT("mode"), EFlowMCPCourierFieldLegality::Required,
		TEXT("\"Full\" or \"Patch\". Full is authoritative over the whole asset; Patch changes only what its ops mention.")));
	Result.DocumentFields.Add(MakeCourierField(TEXT("assetClass"), EFlowMCPCourierFieldLegality::RequiredOnCreate,
		TEXT("Full class path. Required when creating an asset; on an existing asset it must match, or it is an error.")));
	Result.DocumentFields.Add(MakeCourierField(TEXT("bWorldBound"), EFlowMCPCourierFieldLegality::Optional, FString()));
	Result.DocumentFields.Add(MakeCourierField(TEXT("expectedOwnerClass"), EFlowMCPCourierFieldLegality::Optional, FString()));
	Result.DocumentFields.Add(MakeCourierField(TEXT("scopeNodeGuids"), EFlowMCPCourierFieldLegality::Optional,
		TEXT("Patch mode only: nodes this document is authoritative over. A listed node with no corresponding UpsertNode op is deleted. Illegal (ScopeWithFullMode) in Full mode, where the whole document is already authoritative.")));
	Result.DocumentFields.Add(MakeCourierField(TEXT("ops"), EFlowMCPCourierFieldLegality::Required, FString()));

	return Result;
}

FFlowMCPSearchFlowAssetsResult UFlowMCPToolset::SearchFlowAssets(
	const FFlowMCPSearchFlowAssetsRequest& Request)
{
	FFlowMCPSearchFlowAssetsResult Result;
#if WITH_EDITOR
	if (Request.Query.IsEmpty())
	{
		RaiseFlowGraphToolError(TEXT("SearchFlowAssets: Query must not be empty."));
		return Result;
	}

	EFlowSearchPinDirection ResolvedPinDirection;
	if (Request.PinDirection.Equals(TEXT("any"), ESearchCase::IgnoreCase))
	{
		ResolvedPinDirection = EFlowSearchPinDirection::Any;
	}
	else if (Request.PinDirection.Equals(TEXT("input"), ESearchCase::IgnoreCase))
	{
		ResolvedPinDirection = EFlowSearchPinDirection::Input;
	}
	else if (Request.PinDirection.Equals(TEXT("output"), ESearchCase::IgnoreCase))
	{
		ResolvedPinDirection = EFlowSearchPinDirection::Output;
	}
	else
	{
		RaiseFlowGraphToolError(TEXT("SearchFlowAssets: PinDirection must be 'any', 'input', or 'output'."));
		return Result;
	}

	EFlowSearchPinConnectionState ResolvedPinConnection;
	if (Request.PinConnection.Equals(TEXT("any"), ESearchCase::IgnoreCase))
	{
		ResolvedPinConnection = EFlowSearchPinConnectionState::Any;
	}
	else if (Request.PinConnection.Equals(TEXT("connected"), ESearchCase::IgnoreCase))
	{
		ResolvedPinConnection = EFlowSearchPinConnectionState::Connected;
	}
	else if (Request.PinConnection.Equals(TEXT("unconnected"), ESearchCase::IgnoreCase))
	{
		ResolvedPinConnection = EFlowSearchPinConnectionState::Unconnected;
	}
	else
	{
		RaiseFlowGraphToolError(TEXT("SearchFlowAssets: PinConnection must be 'any', 'connected', or 'unconnected'."));
		return Result;
	}

	FFlowSearchQuery SearchQuery;
	SearchQuery.SearchText = Request.Query;
	SearchQuery.Flags = (Request.Flags == 0)
		? EFlowSearchFlags::DefaultSearchFlags
		: static_cast<EFlowSearchFlags>(static_cast<uint32>(Request.Flags));
	SearchQuery.MaxDepth = FMath::Max(Request.MaxDepth, 1);
	SearchQuery.PinFilter.Direction = ResolvedPinDirection;
	SearchQuery.PinFilter.ConnectionState = ResolvedPinConnection;

	const EFlowSearchScope ResolvedScope = Request.Scope;
	SearchQuery.Scope = ResolvedScope;

	if (!Request.ContextAssetPath.IsEmpty())
	{
		UFlowAsset* ContextAsset = LoadFlowAssetOrNull(Request.ContextAssetPath);
		if (ContextAsset)
		{
			SearchQuery.ContextAsset = ContextAsset;
		}
		else if (ResolvedScope == EFlowSearchScope::ThisAssetOnly || ResolvedScope == EFlowSearchScope::AllOfThisType)
		{
			RaiseFlowGraphToolError(FString::Printf(
				TEXT("SearchFlowAssets: context_asset_path '%s' could not be loaded as a FlowAsset"),
				*Request.ContextAssetPath));
			return Result;
		}
	}
	else if (ResolvedScope == EFlowSearchScope::ThisAssetOnly || ResolvedScope == EFlowSearchScope::AllOfThisType)
	{
		RaiseFlowGraphToolError(TEXT("SearchFlowAssets: context_asset_path is required for ThisAssetOnly and AllOfThisType scopes."));
		return Result;
	}

	TArray<FFlowSearchResultItem> RawResults;
	FFlowSearch::Search(SearchQuery, RawResults);

	Result.Results.Reserve(RawResults.Num());
	for (const FFlowSearchResultItem& Item : RawResults)
	{
		FFlowMCPSearchResultItem FlowMCPItem;
		FlowMCPItem.AssetPath = Item.AssetPath.ToString();
		FlowMCPItem.NodeGuid = Item.NodeGuid.ToString();
		FlowMCPItem.NodeTitle = Item.NodeTitle;
		FlowMCPItem.NodeType = Item.NodeTypeName;
		FlowMCPItem.MatchedFlags = static_cast<int32>(static_cast<uint32>(Item.MatchedFlags));
		FlowMCPItem.MatchedSnippet = Item.MatchedSnippet;
		FlowMCPItem.MatchedPins.Reserve(Item.MatchedPins.Num());
		for (const FFlowSearchMatchedPin& MatchedPin : Item.MatchedPins)
		{
			FFlowMCPSearchMatchedPin FlowMCPMatchedPin;
			FlowMCPMatchedPin.PinName = MatchedPin.PinName.ToString();
			FlowMCPMatchedPin.Direction = MatchedPin.Direction == EFlowSearchPinDirection::Input
				? TEXT("input")
				: TEXT("output");
			FlowMCPMatchedPin.bConnected = MatchedPin.bConnected;
			FlowMCPItem.MatchedPins.Add(MoveTemp(FlowMCPMatchedPin));
		}
		FlowMCPItem.bIsSubgraphNode = Item.bIsSubGraphNode;
		if (Item.bIsSubGraphNode)
		{
			FlowMCPItem.SubgraphOwnerAssetPath = Item.SubgraphOwnerAssetPath.ToString();
		}

		Result.Results.Add(FlowMCPItem);
	}

	FString ScopeName = StaticEnum<EFlowSearchScope>()->GetNameStringByValue(static_cast<int64>(ResolvedScope));
	if (ScopeName.IsEmpty())
	{
		ScopeName = TEXT("Unknown");
	}

	Result.ResultCount = RawResults.Num();
	Result.Query = Request.Query;
	Result.Scope = ScopeName;
	Result.Flags = static_cast<int32>(static_cast<uint32>(SearchQuery.Flags));
	Result.PinDirection = Request.PinDirection.ToLower();
	Result.PinConnection = Request.PinConnection.ToLower();
	return Result;
#else
	RaiseFlowGraphToolError(TEXT("SearchFlowAssets requires an editor build."));
	return Result;
#endif
}
