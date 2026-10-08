// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AddOns/FlowNodeAddOn.h"
#include "Engine/DataTable.h"
#include "Find/FindInFlowEnums.h"
#include "FlowAsset.h"
#include "FlowCourierDocument.h"
#include "FlowMCPToolset.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Nodes/FlowNode.h"
#include "Tests/FlowNodeClassReplacementTestTypes.h"
#include "ToolsetRegistry/ToolCallExceptionHandler.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeClassReplacementTestTypes)

namespace
{
	const auto TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ProductFilter |
		EAutomationTestFlags::CriticalPriority;

	// Courier v2 JSON document - two nodes connected by one exec connection.
	const FString ValidFlowGraphText = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Full",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-000000000001", "type": "/Script/Flow.FlowNode_Start", "outputPins": [ { "name": "Out", "type": "Exec" } ] },
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-000000000002", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-000000000001", "pin": "Out" }, "target": { "nodeGuid": "00000000-0000-0000-0000-000000000002", "pin": "In" } }
	]
}
)JSON");
}

BEGIN_DEFINE_SPEC(
	FFlowMCPToolsetSpec,
	"AI.Toolsets.FlowMCPToolset",
	TestFlags)

	TUniquePtr<UE::ToolsetRegistry::FToolCallExceptionHandler> ExceptionHandler;

	void ExpectNoException()
	{
		check(ExceptionHandler.IsValid());
		TestEqual(TEXT("No script error"), ExceptionHandler->GetException(), TEXT(""));
	}

	void ExpectExceptionContains(const FString& ExpectedText)
	{
		check(ExceptionHandler.IsValid());
		TestTrue(
			*FString::Printf(TEXT("Script error contains '%s'"), *ExpectedText),
			ExceptionHandler->GetException().Contains(ExpectedText));
	}

	FString MakeTestAssetPath()
	{
		return FString::Printf(
			TEXT("/Game/__FlowMCPToolsetTests/Flow_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FString MakeTestBlueprintName()
	{
		return FString::Printf(
			TEXT("FlowNodeBP_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

END_DEFINE_SPEC(FFlowMCPToolsetSpec)

void FFlowMCPToolsetSpec::Define()
{
	using namespace UE::ToolsetRegistry;

	BeforeEach([this]()
	{
		ExceptionHandler = MakeUnique<FToolCallExceptionHandler>();
	});

	AfterEach([this]()
	{
		ExceptionHandler.Reset();
	});

	Describe(TEXT("Registration and schemas"), [this]()
	{
		It(TEXT("Registers all typed Flow Graph operations"), [this]()
		{
			TestTrue(
				TEXT("Flow Graph toolset registered"),
				UToolsetRegistry::IsToolsetClassRegistered(
					UFlowMCPToolset::StaticClass()));

			const FString Schema = UToolsetRegistry::GetToolsetJsonSchema(
				UFlowMCPToolset::StaticClass());
			const TCHAR* ToolNames[] =
			{
				TEXT("ExportFlowAsset"),
				TEXT("ImportAndRegraphFlowAsset"),
				TEXT("ListFlowAssetTypes"),
				TEXT("FindFlowNodeTypes"),
				TEXT("SearchFlowAssets"),
				TEXT("CreateFlowNodeBlueprint"),
				TEXT("PlanFlowSubgraphFromSelection"),
				TEXT("CreateFlowSubgraphFromSelection"),
				TEXT("ReplaceFlowNodeClass"),
			};

			for (const TCHAR* ToolName : ToolNames)
			{
				TestTrue(
					*FString::Printf(TEXT("%s is discoverable"), ToolName),
					Schema.Contains(ToolName));
			}
		});
	});

	Describe(TEXT("Subgraph from selection"), [this]()
	{
		It(TEXT("Requires an owned transaction before loading the source asset"), [this]()
		{
			FFlowMCPCreateFlowSubgraphFromSelectionRequest Request;
			Request.TargetAssetPath = TEXT("/Game/DoesNotMatter");
			Request.Mutation.bUseTransaction = false;

			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::CreateFlowSubgraphFromSelection(Request);
			});

			ExpectExceptionContains(TEXT("requires Mutation.bUseTransaction=true"));
		});

		It(TEXT("Rejects an unavailable source asset before mutation"), [this]()
		{
			FFlowMCPPlanFlowSubgraphFromSelectionRequest Request;
			Request.TargetAssetPath = TEXT("/Game/DoesNotMatter");
			Request.SelectionGuids = {};
			Request.NewAssetName = TEXT("Subgraph_Test");

			FFlowMCPPlanFlowSubgraphFromSelectionResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::PlanFlowSubgraphFromSelection(Request);
			});

			ExpectExceptionContains(TEXT("Failed to load asset"));
			TestFalse(TEXT("No plan is reported for an unavailable asset"), Result.Plan.bCanApply);
		});

		It(TEXT("Rejects malformed selection GUIDs after loading the asset"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest ImportRequest;
			ImportRequest.FlowGraphText = ValidFlowGraphText;
			ImportRequest.AssetPath = MakeTestAssetPath();
			ImportRequest.Mutation.bSave = false;
			FFlowMCPImportAndRegraphFlowAssetResult ImportResult;
			ExceptionHandler->CaptureErrorsIn([&ImportRequest, &ImportResult]()
			{
				ImportResult = UFlowMCPToolset::ImportAndRegraphFlowAsset(ImportRequest);
			});
			ExpectNoException();
			if (ImportResult.AssetPath.IsEmpty())
			{
				return;
			}

			FFlowMCPPlanFlowSubgraphFromSelectionRequest Request;
			Request.TargetAssetPath = ImportResult.AssetPath;
			Request.SelectionGuids = { TEXT("not-a-guid") };
			Request.NewAssetName = TEXT("Subgraph_Test");

			FFlowMCPPlanFlowSubgraphFromSelectionResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::PlanFlowSubgraphFromSelection(Request);
			});

			ExpectExceptionContains(TEXT("invalid GUID"));
			TestFalse(TEXT("No plan is reported for an unavailable asset"), Result.Plan.bCanApply);
		});
	});

	Describe(TEXT("Export"), [this]()
	{
		It(TEXT("Rejects an export request without an asset path"), [this]()
		{
			FFlowMCPExportFlowAssetRequest Request;
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ExportFlowAsset(Request);
			});

			ExpectExceptionContains(TEXT("AssetPath is required"));
		});

		It(TEXT("Exports a transient FlowAsset through Courier"), [this]()
		{
			UFlowAsset* FlowAsset = NewObject<UFlowAsset>(GetTransientPackage());
			TestNotNull(TEXT("Transient FlowAsset"), FlowAsset);
			if (!FlowAsset)
			{
				return;
			}

			FFlowMCPExportFlowAssetRequest Request;
			Request.AssetPath = FlowAsset->GetPathName();
			FFlowMCPExportFlowAssetResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::ExportFlowAsset(Request);
			});

			ExpectNoException();
			TestTrue(
				TEXT("Courier v2 formatVersion is present"),
				Result.ExportedText.Contains(TEXT("\"formatVersion\": 2")));
			TestEqual(
				TEXT("Text length matches output"),
				Result.TextLength,
				Result.ExportedText.Len());
		});

		It(TEXT("Exports an authored SwitchCase before its runtime owner is cached"), [this]()
		{
			const FString AssetPath = MakeTestAssetPath();
			FFlowMCPImportAndRegraphFlowAssetRequest Import;
			Import.AssetPath = AssetPath;
			Import.FlowGraphText = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/Flow.FlowAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000003","parentGuid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNodeAddOn_SwitchCase","properties":{"CaseName":"CaseA"}},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNode_Finish","inputPins":[{"name":"In","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}}]})JSON");
			Import.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Import]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Import);
			});
			ExpectNoException();

			FGuid RootGuid;
			FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), RootGuid);
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *AssetPath);
			UFlowNode* Root = IsValid(Asset) ? Asset->GetNode(RootGuid) : nullptr;
			UFlowNodeAddOn* Case = IsValid(Root) && !Root->GetFlowNodeAddOnChildren().IsEmpty()
				? Root->GetFlowNodeAddOnChildren()[0] : nullptr;
			if (!TestNotNull(TEXT("Imported SwitchCase addon"), Case))
			{
				return;
			}
			if (!TestTrue(TEXT("Move addon outside the Flow node Outer chain"), Case->Rename(nullptr, Asset, REN_DoNotDirty)))
			{
				return;
			}
			TestNull(TEXT("Authored addon has no Flow node Outer"), Case->FindOwningFlowNode());
			Case->SetFlowNodeForEditor(nullptr);
			TestNull(TEXT("Authored addon has no runtime owner cache"), Case->GetFlowNodeSelfOrOwner());

			FFlowMCPExportFlowAssetRequest Request;
			Request.AssetPath = AssetPath;
			FFlowMCPExportFlowAssetResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::ExportFlowAsset(Request);
			});
			ExpectNoException();
			TestTrue(TEXT("SwitchCase template exports without graph-integrity errors"), Result.GraphIntegrityIssues.IsEmpty());
			TestTrue(TEXT("SwitchCase addon survives export"), Result.ExportedText.Contains(TEXT("FlowNodeAddOn_SwitchCase")));
		});
	});

	Describe(TEXT("Import"), [this]()
	{
		It(TEXT("Rejects import without Flow Courier text"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.AssetPath = MakeTestAssetPath();
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});

			ExpectExceptionContains(TEXT("FlowGraphText is required"));
		});

		It(TEXT("Rejects an import target that is not a long object path"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.FlowGraphText = ValidFlowGraphText;
			Request.AssetPath = TEXT("FlowWithoutRoot");
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});

			ExpectExceptionContains(TEXT("must start with '/'"));
		});

		It(TEXT("Rejects an import dry run"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.FlowGraphText = ValidFlowGraphText;
			Request.AssetPath = MakeTestAssetPath();
			Request.Mutation.bDryRun = true;
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});

			ExpectExceptionContains(TEXT("does not support dry runs"));
		});

		It(TEXT("Imports and regraphs a FlowAsset without saving it"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.FlowGraphText = ValidFlowGraphText;
			Request.AssetPath = MakeTestAssetPath();
			Request.Mutation.bSave = false;
			FFlowMCPImportAndRegraphFlowAssetResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});

			ExpectNoException();
			TestTrue(
				TEXT("Imported graph contains the two Flow Courier nodes"),
				Result.NodeCount >= 2);
			TestFalse(TEXT("Import was not saved"), Result.Mutation.bSaved);
			TestTrue(TEXT("Imported asset has a path"), !Result.AssetPath.IsEmpty());
			TestTrue(
				TEXT("An unsaved import does not check out a package"),
				Result.Mutation.CheckedOutPackages.IsEmpty());

			FFlowMCPExportFlowAssetRequest ExportRequest;
			ExportRequest.AssetPath = Result.AssetPath;
			const FString Before = UFlowMCPToolset::ExportFlowAsset(ExportRequest).ExportedText;
			FFlowMCPImportAndRegraphFlowAssetRequest DuplicateRequest = Request;
			DuplicateRequest.FlowGraphText = TEXT("{\"formatVersion\":2,\"mode\":\"Full\",\"ops\":[]}");
			ExceptionHandler->CaptureErrorsIn([&DuplicateRequest]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(DuplicateRequest);
			});
			ExpectExceptionContains(TEXT("already exists at AssetPath"));
			TestEqual(TEXT("A rejected import preserves the existing graph"),
				UFlowMCPToolset::ExportFlowAsset(ExportRequest).ExportedText, Before);
		});

		It(TEXT("Rejects a path occupied by another asset class"), [this]()
		{
			const FString AssetPath = MakeTestAssetPath();
			const FString PackageName = FPackageName::ObjectPathToPackageName(AssetPath);
			UPackage* Package = CreatePackage(*PackageName);
			UDataTable* Occupant = NewObject<UDataTable>(Package, *FPackageName::GetShortName(PackageName));
			if (!TestNotNull(TEXT("Existing object"), Occupant))
			{
				return;
			}

			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.FlowGraphText = ValidFlowGraphText;
			Request.AssetPath = AssetPath;
			Request.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});
			ExpectExceptionContains(TEXT("already exists at AssetPath"));
			TestTrue(TEXT("The existing object is preserved"),
				FindObject<UObject>(Package, *FPackageName::GetShortName(PackageName)) == Occupant);
		});

		It(TEXT("Releases the asset path after an import failure"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.AssetPath = MakeTestAssetPath();
			Request.FlowGraphText = ValidFlowGraphText.Replace(
				TEXT("/Script/Flow.FlowNode_Start"), TEXT("/Script/Flow.MissingNode"));
			Request.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});
			ExpectExceptionContains(TEXT("Failed to import and rebuild FlowAsset"));

			Request.FlowGraphText = ValidFlowGraphText;
			ExceptionHandler = MakeUnique<UE::ToolsetRegistry::FToolCallExceptionHandler>();
			FFlowMCPImportAndRegraphFlowAssetResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});
			ExpectNoException();
			TestTrue(TEXT("The corrected import can use the same path"), Result.NodeCount >= 2);
		});

		It(TEXT("Rejects an unresolved asset class instead of creating a base FlowAsset"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest Request;
			Request.AssetPath = MakeTestAssetPath();
			Request.FlowGraphText = ValidFlowGraphText.Replace(
				TEXT("/Script/Flow.FlowAsset"), TEXT("/Script/Flow.MissingAsset"));
			Request.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Request);
			});
			ExpectExceptionContains(TEXT("AssetClass is not a resolvable FlowAsset class"));
			UPackage* Package = FindPackage(nullptr, *FPackageName::ObjectPathToPackageName(Request.AssetPath));
			TestNull(TEXT("No base FlowAsset occupies the requested path"),
				Package ? FindObject<UFlowAsset>(Package, *FPackageName::GetShortName(Request.AssetPath)) : nullptr);
		});
	});

	Describe(TEXT("Apply"), [this]()
	{
		It(TEXT("Creates a new asset and reports it as modified, not a silent no-op success"), [this]()
		{
			const FString TargetAssetPath = MakeTestAssetPath();

			FFlowMCPApplyFlowPatchRequest Request;
			Request.TargetAssetPath = TargetAssetPath;
			Request.MutationText = ValidFlowGraphText;
			Request.Mutation.bSave = false;

			FFlowMCPApplyFlowPatchResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::ApplyFlowPatch(Request);
			});

			ExpectNoException();
			TestTrue(TEXT("Apply reports success"), Result.bApplySucceeded);
			TestFalse(TEXT("Apply is not falsely blocked"), Result.bHasBlockingError);
			TestFalse(TEXT("Plan is non-empty"), Result.Plan.bIsEmpty);
			TestTrue(
				TEXT("A reported success actually modified a package"),
				Result.Mutation.ModifiedPackages.Num() > 0);
			TestTrue(
				TEXT("An unsaved apply does not check out a package"),
				Result.Mutation.CheckedOutPackages.IsEmpty());

			FFlowMCPExportFlowAssetRequest ExportRequest;
			ExportRequest.AssetPath = TargetAssetPath;
			FFlowMCPExportFlowAssetResult ExportResult;
			ExceptionHandler->CaptureErrorsIn([&ExportRequest, &ExportResult]()
			{
				ExportResult = UFlowMCPToolset::ExportFlowAsset(ExportRequest);
			});

			ExpectNoException();
			TestTrue(
				TEXT("The created asset is actually findable afterward"),
				!ExportResult.ExportedText.IsEmpty());
		});

		It(TEXT("Dry-runs a new-asset apply without persisting or reporting a modified package"), [this]()
		{
			FFlowMCPApplyFlowPatchRequest Request;
			Request.TargetAssetPath = MakeTestAssetPath();
			Request.MutationText = ValidFlowGraphText;
			Request.Mutation.bDryRun = true;

			FFlowMCPApplyFlowPatchResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::ApplyFlowPatch(Request);
			});

			ExpectNoException();
			TestTrue(TEXT("Dry run still reports the plan as succeeding"), Result.bApplySucceeded);
			TestFalse(TEXT("Dry run never persists anything"), Result.Mutation.bSaved);
			TestTrue(
				TEXT("Dry run does not report a modified package"),
				Result.Mutation.ModifiedPackages.IsEmpty());
		});
	});

	Describe(TEXT("Replace Flow node class"), [this]()
	{
		It(TEXT("preserves GUIDs, nested addons, properties, and connections across an explicit class swap"), [this]()
		{
			const FString TargetPath = MakeTestAssetPath();
			const FString Fixture = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/Flow.FlowAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNode_Timer","properties":{"CompletionTime":"5.0","StepTime":"1.0"},"inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"},{"name":"Step","type":"Exec"}]},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000004","parentGuid":"00000000-0000-0000-0000-000000000002","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOn","properties":{"Value":"7"}},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000005","parentGuid":"00000000-0000-0000-0000-000000000004","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOn","properties":{"Value":"9"}},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000003","type":"/Script/Flow.FlowNode_Finish","inputPins":[{"name":"In","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"Completed"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000003","pin":"In"}}]})JSON");
			FFlowMCPImportAndRegraphFlowAssetRequest ImportRequest;
			ImportRequest.AssetPath = TargetPath;
			ImportRequest.FlowGraphText = Fixture;
			ImportRequest.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(ImportRequest);
			});
			ExpectNoException();
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *TargetPath);
			TestNotNull(TEXT("Unsaved Flow fixture loaded"), Asset);
			if (!Asset)
			{
				return;
			}
			const FGuid TimerGuid(0, 0, 0, 2);
			const FGuid AddOnGuid(0, 0, 0, 4);
			const FGuid ChildGuid(0, 0, 0, 5);
			UFlowNode* Original = Asset->GetNode(TimerGuid);
			TestNotNull(TEXT("Timer exists before replacement"), Original);
			if (!Original)
			{
				return;
			}

			FFlowMCPReplaceFlowNodeClassRequest Request;
			Request.AssetPath = TargetPath;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestTimer");
			Request.AddOnClassMappings.Add(AddOnGuid.ToString(EGuidFormats::Digits),
				TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOnV2"));
			Request.AddOnClassMappings.Add(ChildGuid.ToString(EGuidFormats::Digits),
				TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOnV2"));
			Request.Mutation.bDryRun = true;
			Request.Mutation.bSave = false;
			FFlowMCPReplaceFlowNodeClassResult Preview;
			ExceptionHandler->CaptureErrorsIn([&]()
			{
				Preview = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			});
			ExpectNoException();
			TestTrue(TEXT("Replacement preflight succeeds"), Preview.bCanReplace);
			TestTrue(TEXT("Dry run does not replace the UObject"), Asset->GetNode(TimerGuid) == Original);
			if (!Preview.bCanReplace)
			{
				for (const FString& Finding : Preview.Findings)
				{
					AddError(Finding);
				}
				return;
			}

			Request.Mutation.bDryRun = false;
			FFlowMCPReplaceFlowNodeClassResult Applied;
			ExceptionHandler->CaptureErrorsIn([&]()
			{
				Applied = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			});
			ExpectNoException();
			TestTrue(TEXT("Class replacement applied"), Applied.bReplaced);
			TestTrue(TEXT("The root node has a new UObject but the same GUID"),
				Asset->GetNode(TimerGuid) != Original
				&& Asset->GetNode(TimerGuid)->GetClass() == UFlowNodeClassReplacementTestTimer::StaticClass());
			UFlowNode* Replacement = Asset->GetNode(TimerGuid);
			if (!Applied.bReplaced || !Replacement)
			{
				for (const FString& Finding : Applied.Findings)
				{
					AddError(Finding);
				}
				return;
			}
			TestEqual(TEXT("One root addon survives"), Replacement->GetFlowNodeAddOnChildren().Num(), 1);
			if (!Replacement->GetFlowNodeAddOnChildren().IsEmpty())
			{
				UFlowNodeAddOn* AddOn = Replacement->GetFlowNodeAddOnChildren()[0];
				TestTrue(TEXT("Addon class and GUID preserved by mapping"),
					AddOn->GetGuid() == AddOnGuid
					&& AddOn->GetClass() == UFlowNodeClassReplacementTestAddOnV2::StaticClass());
				TestEqual(TEXT("Mapped addon retains its value"),
					CastChecked<UFlowNodeClassReplacementTestAddOn>(AddOn)->Value, 7);
				TestEqual(TEXT("Nested addon survives"), AddOn->GetFlowNodeAddOnChildren().Num(), 1);
				if (!AddOn->GetFlowNodeAddOnChildren().IsEmpty())
				{
					UFlowNodeAddOn* Child = AddOn->GetFlowNodeAddOnChildren()[0];
					TestTrue(TEXT("Nested addon class changes without losing its GUID"),
						Child->GetGuid() == ChildGuid
						&& Child->GetClass() == UFlowNodeClassReplacementTestAddOnV2::StaticClass());
					TestEqual(TEXT("Nested addon value survives"),
						CastChecked<UFlowNodeClassReplacementTestAddOn>(Child)->Value, 9);
				}
			}
			FFlowMCPExportFlowAssetRequest ExportRequest;
			ExportRequest.AssetPath = TargetPath;
			const FFlowMCPExportFlowAssetResult Exported = UFlowMCPToolset::ExportFlowAsset(ExportRequest);
			TestTrue(TEXT("Editor and runtime graphs agree"), Exported.GraphIntegrityIssues.IsEmpty());

			FFlowMCPReplaceFlowNodeClassResult Repeated = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			if (!Repeated.bCanReplace || Repeated.bReplaced)
			{
				AddError(FString::Printf(TEXT("Unexpected repeat status: canReplace=%d replaced=%d"),
					Repeated.bCanReplace, Repeated.bReplaced));
				for (const FString& Finding : Repeated.Findings)
				{
					AddError(Finding);
				}
			}
			TestTrue(TEXT("Class replacement is idempotent"), Repeated.bCanReplace && !Repeated.bReplaced);
			Request.PropertyMappings.Add(TEXT("NotAnAuthoredField"), TEXT("OtherField"));
			Request.Mutation.bDryRun = true;
			FFlowMCPReplaceFlowNodeClassResult Blocked = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Unknown property mapping is blocked without mutation"), Blocked.bCanReplace);
			TestTrue(TEXT("Blocked replacement leaves the node intact"), Asset->GetNode(TimerGuid) == Replacement);
		});

		It(TEXT("rejects unknown pin mappings and mutation without a rollback transaction"), [this]()
		{
			const FString TargetPath = MakeTestAssetPath();
			const FString Fixture = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/Flow.FlowAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNode_Timer","inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"},{"name":"Step","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}}]})JSON");
			FFlowMCPImportAndRegraphFlowAssetRequest Import;
			Import.AssetPath = TargetPath;
			Import.FlowGraphText = Fixture;
			Import.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Import]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Import);
			});
			ExpectNoException();
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *TargetPath);
			const FGuid TimerGuid(0, 0, 0, 2);
			UFlowNode* Original = IsValid(Asset) ? Asset->GetNode(TimerGuid) : nullptr;
			if (!TestNotNull(TEXT("Original timer exists"), Original))
			{
				return;
			}

			FFlowMCPReplaceFlowNodeClassRequest Request;
			Request.AssetPath = TargetPath;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestTimer");
			Request.Mutation.bDryRun = true;
			Request.Mutation.bSave = false;
			Request.PinMappings.Add(TEXT("MissingPin"), TEXT("Step"));
			const FFlowMCPReplaceFlowNodeClassResult UnknownPin = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Unknown pin mapping blocks preflight"), UnknownPin.bCanReplace);
			TestTrue(TEXT("Unknown mapping identifies the missing source"), UnknownPin.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("MissingPin"));
			}));
			TestTrue(TEXT("Unknown mapping leaves the node intact"), Asset->GetNode(TimerGuid) == Original);

			Request.PinMappings.Reset();
			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestAmbiguousTimer");
			const FFlowMCPReplaceFlowNodeClassResult AmbiguousPins = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Duplicate input/output pin names block replacement"), AmbiguousPins.bCanReplace);
			TestTrue(TEXT("Finding identifies the ambiguous pin"), AmbiguousPins.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("In")) && Finding.Contains(TEXT("duplicate pin"));
			}));
			TestTrue(TEXT("Ambiguous target leaves the node intact"), Asset->GetNode(TimerGuid) == Original);

			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestTimer");
			Request.Mutation.bDryRun = false;
			Request.Mutation.bUseTransaction = false;
			const FFlowMCPReplaceFlowNodeClassResult NoRollback = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Replacement without a rollback transaction is blocked"), NoRollback.bCanReplace);
			TestTrue(TEXT("Transaction finding explains the requirement"), NoRollback.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("bUseTransaction=true"));
			}));
			TestTrue(TEXT("No-transaction request leaves the node intact"), Asset->GetNode(TimerGuid) == Original);
		});

		It(TEXT("rejects a mapped addon class denied by the asset"), [this]()
		{
			const FString TargetPath = MakeTestAssetPath();
			const FString Fixture = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNode_Timer","inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"}]},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000004","parentGuid":"00000000-0000-0000-0000-000000000002","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOn"},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000003","type":"/Script/Flow.FlowNode_Finish","inputPins":[{"name":"In","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"Completed"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000003","pin":"In"}}]})JSON");
			FFlowMCPImportAndRegraphFlowAssetRequest Import;
			Import.AssetPath = TargetPath;
			Import.FlowGraphText = Fixture;
			Import.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Import]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Import);
			});
			ExpectNoException();
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *TargetPath);
			if (!TestNotNull(TEXT("Fixture asset exists"), Asset))
			{
				return;
			}
			const FGuid TimerGuid(0, 0, 0, 2);
			UFlowNode* Original = Asset->GetNode(TimerGuid);
			if (!TestNotNull(TEXT("Original timer exists"), Original))
			{
				return;
			}
			TestFalse(TEXT("Mapped addon class is denied by this asset"),
				Asset->IsNodeOrAddOnClassAllowed(UFlowNodeClassReplacementTestAddOnV2::StaticClass()));

			FFlowMCPReplaceFlowNodeClassRequest Request;
			Request.AssetPath = TargetPath;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestTimer");
			Request.AddOnClassMappings.Add(FGuid(0, 0, 0, 4).ToString(EGuidFormats::Digits),
				TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOnV2"));
			Request.Mutation.bDryRun = true;
			Request.Mutation.bSave = false;
			const FFlowMCPReplaceFlowNodeClassResult Preview = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Denied addon class blocks preflight"), Preview.bCanReplace);
			TestTrue(TEXT("Finding identifies the denied addon class"), Preview.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("not allowed"));
			}));
			TestTrue(TEXT("Denied dry run leaves the original node intact"), Asset->GetNode(TimerGuid) == Original);
		});

		It(TEXT("rejects a mapped predicate parent that would have two children"), [this]()
		{
			const FString TargetPath = MakeTestAssetPath();
			const FString Fixture = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/Flow.FlowAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNode_Timer","inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"}]},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000004","parentGuid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNodeAddOn_PredicateAND"},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000005","parentGuid":"00000000-0000-0000-0000-000000000004","type":"/Script/Flow.FlowNodeAddOn_PredicateNOT"},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000006","parentGuid":"00000000-0000-0000-0000-000000000004","type":"/Script/Flow.FlowNodeAddOn_PredicateNOT"},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000003","type":"/Script/Flow.FlowNode_Finish","inputPins":[{"name":"In","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"Completed"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000003","pin":"In"}}]})JSON");
			FFlowMCPImportAndRegraphFlowAssetRequest Import;
			Import.AssetPath = TargetPath;
			Import.FlowGraphText = Fixture;
			Import.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Import]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Import);
			});
			ExpectNoException();
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *TargetPath);
			if (!TestNotNull(TEXT("Predicate fixture exists"), Asset))
			{
				return;
			}
			const FGuid TimerGuid(0, 0, 0, 2);
			UFlowNode* Original = Asset->GetNode(TimerGuid);
			if (!TestNotNull(TEXT("Original timer exists"), Original))
			{
				return;
			}
			FFlowMCPReplaceFlowNodeClassRequest Request;
			Request.AssetPath = TargetPath;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			Request.NewNodeClass = TEXT("/Script/Flow.FlowNode_Timer");
			Request.AddOnClassMappings.Add(FGuid(0, 0, 0, 4).ToString(EGuidFormats::Digits),
				TEXT("/Script/Flow.FlowNodeAddOn_PredicateNOT"));
			Request.Mutation.bDryRun = true;
			Request.Mutation.bSave = false;
			const FFlowMCPReplaceFlowNodeClassResult Preview = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("PredicateNOT rejects its second child during preflight"), Preview.bCanReplace);
			TestTrue(TEXT("Finding identifies sibling-dependent parent rejection"), Preview.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("Replacement parent rejects addon"));
			}));
			TestTrue(TEXT("Rejected dry run preserves the original predicate tree"),
				Asset->GetNode(TimerGuid) == Original && Original->GetFlowNodeAddOnChildren().Num() == 1
				&& Original->GetFlowNodeAddOnChildren()[0]->GetFlowNodeAddOnChildren().Num() == 2);
		});

		It(TEXT("blocks a class swap when two attached addons share a GUID"), [this]()
		{
			const FString TargetPath = MakeTestAssetPath();
			const FString Fixture = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/Flow.FlowAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/Flow.FlowNode_Timer","inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000006","type":"/Script/Flow.FlowNode_Timer","inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"}]},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000004","parentGuid":"00000000-0000-0000-0000-000000000002","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOn"},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000007","parentGuid":"00000000-0000-0000-0000-000000000004","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOn"},
{"kind":"UpsertAddon","guid":"00000000-0000-0000-0000-000000000005","parentGuid":"00000000-0000-0000-0000-000000000002","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestAddOn"},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000003","type":"/Script/Flow.FlowNode_Finish","inputPins":[{"name":"In","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"Completed"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000003","pin":"In"}}]})JSON");
			FFlowMCPImportAndRegraphFlowAssetRequest Import;
			Import.AssetPath = TargetPath;
			Import.FlowGraphText = Fixture;
			Import.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&Import]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(Import);
			});
			ExpectNoException();
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *TargetPath);
			const FGuid TimerGuid(0, 0, 0, 2);
			UFlowNode* Timer = IsValid(Asset) ? Asset->GetNode(TimerGuid) : nullptr;
			if (!TestNotNull(TEXT("Imported Timer node"), Timer)
				|| !TestEqual(TEXT("Imported addon count"), Timer->GetFlowNodeAddOnChildren().Num(), 2))
			{
				return;
			}
			const TArray<UFlowNodeAddOn*>& AddOns = Timer->GetFlowNodeAddOnChildren();
			if (!TestEqual(TEXT("Nested addon count"), AddOns[0]->GetFlowNodeAddOnChildren().Num(), 1))
			{
				return;
			}
			UFlowNodeAddOn* Nested = AddOns[0]->GetFlowNodeAddOnChildren()[0];
			Nested->SetGuid(AddOns[0]->GetGuid());
			AddOns[1]->SetGuid(AddOns[0]->GetGuid());

			FFlowMCPReplaceFlowNodeClassRequest Request;
			Request.AssetPath = TargetPath;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestTimer");
			Request.Mutation.bDryRun = true;
			Request.Mutation.bSave = false;
			const FFlowMCPReplaceFlowNodeClassResult Preview = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Duplicate addon identity blocks replacement"), Preview.bCanReplace);
			TestTrue(TEXT("Diagnostic names the duplicate GUID"), Preview.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("duplicate addon GUID"));
			}));
			TestTrue(TEXT("Dry run retains both authored addons"), Asset->GetNode(TimerGuid) == Timer
				&& Timer->GetFlowNodeAddOnChildren().Num() == 2);
			UFlowGraphNode* AddOnEditor = Cast<UFlowGraphNode>(AddOns[0]->GetGraphNode());
			if (!TestNotNull(TEXT("Unrelated addon has an editor subnode"), AddOnEditor))
			{
				return;
			}
			UFlowNodeClassReplacementTestAddOn* EditorCopy = NewObject<UFlowNodeClassReplacementTestAddOn>(Asset);
			EditorCopy->SetGuid(FGuid(0, 0, 0, 8));
			AddOnEditor->SetNodeTemplate(EditorCopy);

			const FGuid OtherGuid(0, 0, 0, 6);
			Request.NodeGuid = OtherGuid.ToString(EGuidFormats::Digits);
			UFlowNode* UnrelatedOriginal = Asset->GetNode(OtherGuid);
			const FFlowMCPReplaceFlowNodeClassResult BlockedOtherNode = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Unrelated duplicate addon GUIDs require explicit approval"), BlockedOtherNode.bCanReplace);
			TestTrue(TEXT("Preflight identifies the save-time repair opt-in"), BlockedOtherNode.Findings.ContainsByPredicate([](const FString& Finding)
			{
				return Finding.Contains(TEXT("bAllowDuplicateAddonGuidRepair=true"));
			}));
			TestTrue(TEXT("Blocked preview retains the unrelated source node"), Asset->GetNode(OtherGuid) == UnrelatedOriginal);
			Request.bAllowDuplicateAddonGuidRepair = true;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			const FFlowMCPReplaceFlowNodeClassResult StillBlocked = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestFalse(TEXT("Approval cannot bypass duplicate GUIDs on the target node"), StillBlocked.bCanReplace);
			Request.NodeGuid = OtherGuid.ToString(EGuidFormats::Digits);
			const FFlowMCPReplaceFlowNodeClassResult OtherNode = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestTrue(TEXT("Approved unrelated duplicates do not block replacement"), OtherNode.bCanReplace);
			if (!OtherNode.bCanReplace)
			{
				for (const FString& Finding : OtherNode.Findings)
				{
					AddError(Finding);
				}
				return;
			}
			Request.Mutation.bDryRun = false;
			const FFlowMCPReplaceFlowNodeClassResult Applied = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			TestTrue(TEXT("Unrelated node is replaced at its original GUID"), Applied.bReplaced
				&& Asset->GetNode(OtherGuid)->GetClass() == UFlowNodeClassReplacementTestTimer::StaticClass());
			TestTrue(TEXT("Ambiguous addon instances remain untouched"), Asset->GetNode(TimerGuid) == Timer
				&& Timer->GetFlowNodeAddOnChildren().Num() == 2
				&& IsValid(AddOns[0]) && IsValid(AddOns[1]) && IsValid(Nested)
				&& AddOns[0] != EditorCopy
				&& AddOns[0]->GetFlowNodeAddOnChildren().Num() == 1
				&& AddOns[0]->GetGuid() == AddOns[1]->GetGuid()
				&& Nested->GetGuid() == AddOns[0]->GetGuid());
		});

		It(TEXT("re-instances an object nested in an authored struct instead of retaining the old node"), [this]()
		{
			const FString TargetPath = MakeTestAssetPath();
			const FString Fixture = TEXT(R"JSON(
{"formatVersion":2,"mode":"Full","assetClass":"/Script/Flow.FlowAsset","bWorldBound":true,
"ops":[
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000001","type":"/Script/Flow.FlowNode_Start","outputPins":[{"name":"Out","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000002","type":"/Script/FlowGraphCourier.FlowNodeClassReplacementTestOwnedTimer","inputPins":[{"name":"In","type":"Exec"}],"outputPins":[{"name":"Completed","type":"Exec"},{"name":"Step","type":"Exec"}]},
{"kind":"UpsertNode","guid":"00000000-0000-0000-0000-000000000003","type":"/Script/Flow.FlowNode_Finish","inputPins":[{"name":"In","type":"Exec"}]},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000001","pin":"Out"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"In"}},
{"kind":"AddConnection","source":{"nodeGuid":"00000000-0000-0000-0000-000000000002","pin":"Completed"},"target":{"nodeGuid":"00000000-0000-0000-0000-000000000003","pin":"In"}}]})JSON");
			FFlowMCPImportAndRegraphFlowAssetRequest ImportRequest;
			ImportRequest.AssetPath = TargetPath;
			ImportRequest.FlowGraphText = Fixture;
			ImportRequest.Mutation.bSave = false;
			ExceptionHandler->CaptureErrorsIn([&]()
			{
				UFlowMCPToolset::ImportAndRegraphFlowAsset(ImportRequest);
			});
			ExpectNoException();
			UFlowAsset* Asset = LoadObject<UFlowAsset>(nullptr, *TargetPath);
			const FGuid TimerGuid(0, 0, 0, 2);
			UFlowNodeClassReplacementTestOwnedTimer* Original = Asset
				? Cast<UFlowNodeClassReplacementTestOwnedTimer>(Asset->GetNode(TimerGuid)) : nullptr;
			TestNotNull(TEXT("Owned-object source node exists"), Original);
			if (!Original)
			{
				return;
			}
			Original->SetTestOwnedValue(42);
			UFlowNodeClassReplacementTestAddOn* SourceObject = Original->GetTestOwnedObject();
			const FString SourcePath = SourceObject->GetPathName();
			FFlowMCPExportFlowAssetRequest ExportRequest;
			ExportRequest.AssetPath = TargetPath;
			const FFlowMCPExportFlowAssetResult Before = UFlowMCPToolset::ExportFlowAsset(ExportRequest);
			TestTrue(TEXT("Fixture exports the old owned object reference"), Before.ExportedText.Contains(SourcePath));

			FFlowMCPReplaceFlowNodeClassRequest Request;
			Request.AssetPath = TargetPath;
			Request.NodeGuid = TimerGuid.ToString(EGuidFormats::Digits);
			Request.NewNodeClass = TEXT("/Script/FlowGraphCourier.FlowNodeClassReplacementTestOwnedTimerV2");
			Request.Mutation.bDryRun = true;
			Request.Mutation.bSave = false;
			FFlowMCPReplaceFlowNodeClassResult Preview = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			for (const FString& Finding : Preview.Findings)
			{
			AddError(Finding);
			}
			TestTrue(TEXT("Owned subobject passes read-only preflight"), Preview.bCanReplace);
			TestTrue(TEXT("Preview did not move the source object"), SourceObject->GetOuter() == Original);
			if (!Preview.bCanReplace)
			{
				return;
			}

			Request.Mutation.bDryRun = false;
			FFlowMCPReplaceFlowNodeClassResult Applied = UFlowMCPToolset::ReplaceFlowNodeClass(Request);
			for (const FString& Finding : Applied.Findings)
			{
			AddError(Finding);
			}
			TestTrue(TEXT("Owned-object replacement succeeds"), Applied.bReplaced);
			UFlowNodeClassReplacementTestOwnedTimerV2* Replacement =
				Cast<UFlowNodeClassReplacementTestOwnedTimerV2>(Asset->GetNode(TimerGuid));
			if (!Applied.bReplaced || !Replacement)
			{
				return;
			}
			UFlowNodeClassReplacementTestAddOn* OwnedObject = Replacement->GetTestOwnedObject();
			TestTrue(TEXT("Authored instance is duplicated under the replacement node"),
				IsValid(OwnedObject) && OwnedObject != SourceObject && OwnedObject->GetOuter() == Replacement);
			TestEqual(TEXT("Nested authored value survives"), IsValid(OwnedObject) ? OwnedObject->Value : INDEX_NONE, 42);
			const FFlowMCPExportFlowAssetResult Exported = UFlowMCPToolset::ExportFlowAsset(ExportRequest);
			TestFalse(TEXT("Export no longer refers to the old subobject"), Exported.ExportedText.Contains(SourcePath));
			TestTrue(TEXT("Editor and runtime graphs still agree"), Exported.GraphIntegrityIssues.IsEmpty());
		});
	});

	Describe(TEXT("Create Flow node Blueprint"), [this]()
	{
		It(TEXT("Rejects a request with no package path"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Flow.FlowNode");
			Request.Mutation.bSave = false;

			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectExceptionContains(TEXT("PackagePath is required"));
		});

		It(TEXT("Rejects a request with no asset name"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.ParentClass = TEXT("/Script/Flow.FlowNode");
			Request.Mutation.bSave = false;

			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectExceptionContains(TEXT("AssetName is required"));
		});

		It(TEXT("Rejects a parent class that does not resolve"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("ThisClassDoesNotExistAnywhere");
			Request.Mutation.bSave = false;

			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectExceptionContains(TEXT("ThisClassDoesNotExistAnywhere"));
		});

		// The factory guards this case with FMessageDialog::Open, which would block forever with no
		// user present. The op has to reject a non-Flow parent itself, before the factory ever runs.
		It(TEXT("Rejects a non-Flow parent class without reaching the factory's modal dialog"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Engine.Actor");
			Request.Mutation.bSave = false;

			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectExceptionContains(TEXT("FlowNode"));
		});

		// UFlowNodeBase is the shared base of both subtrees, so it names neither factory and the op
		// cannot infer which asset class was meant.
		It(TEXT("Rejects a parent that is neither a node nor an add-on"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Flow.FlowNodeBase");
			Request.Mutation.bSave = false;

			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectExceptionContains(TEXT("FlowNodeAddOn"));
		});

		It(TEXT("Creates a node Blueprint whose asset class is FlowNodeBlueprint, not Blueprint"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Flow.FlowNode");
			Request.DisplayName = TEXT("Spec Node");
			Request.Description = TEXT("Created by FlowMCPToolset spec.");
			Request.Mutation.bSave = false;

			FFlowMCPCreateFlowNodeBlueprintResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectNoException();
			TestEqual(
				TEXT("Asset class is the Flow node Blueprint class"),
				Result.AssetClassPath,
				TEXT("/Script/Flow.FlowNodeBlueprint"));
			TestTrue(TEXT("Blueprint compiled cleanly"), Result.bCompiled);
			TestTrue(TEXT("No compile errors reported"), Result.CompileErrors.IsEmpty());
			TestTrue(
				TEXT("Generated class resolves through the Flow catalog"),
				Result.bResolvedInCatalog);
		});

		It(TEXT("Creates an add-on Blueprint whose asset class is FlowNodeAddOnBlueprint"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Flow.FlowNodeAddOn");
			Request.DisplayName = TEXT("Spec AddOn");
			Request.Description = TEXT("Created by FlowMCPToolset spec.");
			Request.Mutation.bSave = false;

			FFlowMCPCreateFlowNodeBlueprintResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectNoException();
			TestEqual(
				TEXT("Asset class is the Flow add-on Blueprint class"),
				Result.AssetClassPath,
				TEXT("/Script/Flow.FlowNodeAddOnBlueprint"));
			TestTrue(TEXT("Blueprint compiled cleanly"), Result.bCompiled);
			TestTrue(
				TEXT("Generated class resolves through the Flow catalog"),
				Result.bResolvedInCatalog);
		});

		// Absent display name and description are legal - UFlowNodeBase falls back to the class name
		// and the class tooltip - so the op notes the fallback instead of failing.
		It(TEXT("Notes, rather than rejects, a missing display name and description"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Flow.FlowNode");
			Request.Mutation.bSave = false;

			FFlowMCPCreateFlowNodeBlueprintResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectNoException();
			TestTrue(TEXT("Blueprint still compiled"), Result.bCompiled);
			TestFalse(TEXT("A fallback note is reported"), Result.Note.IsEmpty());
		});

		It(TEXT("Dry-runs a creation without persisting anything"), [this]()
		{
			FFlowMCPCreateFlowNodeBlueprintRequest Request;
			Request.PackagePath = TEXT("/Game/__FlowMCPToolsetTests");
			Request.AssetName = MakeTestBlueprintName();
			Request.ParentClass = TEXT("/Script/Flow.FlowNode");
			Request.Mutation.bDryRun = true;

			FFlowMCPCreateFlowNodeBlueprintResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::CreateFlowNodeBlueprint(Request);
			});

			ExpectNoException();
			TestTrue(TEXT("Dry run is reported as such"), Result.Mutation.bDryRun);
			TestFalse(TEXT("Dry run never persists anything"), Result.Mutation.bSaved);
		});
	});

	Describe(TEXT("Courier Grammar"), [this]()
	{
		It(TEXT("Describes exactly the Kind values EFlowCourierOpKind declares, with no drift"), [this]()
		{
			FFlowMCPDescribeCourierGrammarResult Result;
			ExceptionHandler->CaptureErrorsIn([&Result]()
			{
				Result = UFlowMCPToolset::DescribeCourierGrammar(FFlowMCPDescribeCourierGrammarRequest());
			});

			ExpectNoException();

			const UEnum* OpKindEnum = StaticEnum<EFlowCourierOpKind>();
			TArray<FString> DeclaredKindNames;
			for (int32 EnumIndex = 0; EnumIndex < OpKindEnum->NumEnums(); ++EnumIndex)
			{
				const FString EnumValueName = OpKindEnum->GetNameStringByIndex(EnumIndex);
				// Skip the hidden _MAX sentinel some UHT-generated enums carry alongside the
				// declared values; it is never a real Courier op Kind.
				if (EnumValueName.EndsWith(TEXT("_MAX")))
				{
					continue;
				}
				DeclaredKindNames.Add(EnumValueName);
			}

			TestEqual(TEXT("Grammar covers every EFlowCourierOpKind value"), Result.Kinds.Num(), DeclaredKindNames.Num());

			for (const FString& EnumValueName : DeclaredKindNames)
			{
				const bool bDescribed = Result.Kinds.ContainsByPredicate(
					[&EnumValueName](const FFlowMCPCourierOpKindGrammar& KindGrammar)
					{
						return KindGrammar.Kind == EnumValueName;
					});
				TestTrue(
					*FString::Printf(TEXT("Grammar describes Kind '%s'"), *EnumValueName),
					bDescribed);
			}
		});

		It(TEXT("Reports field legality matching the reconciler's validation rules"), [this]()
		{
			FFlowMCPDescribeCourierGrammarResult Result;
			ExceptionHandler->CaptureErrorsIn([&Result]()
			{
				Result = UFlowMCPToolset::DescribeCourierGrammar(FFlowMCPDescribeCourierGrammarRequest());
			});

			ExpectNoException();

			auto FindKind = [&Result](const FString& KindName) -> const FFlowMCPCourierOpKindGrammar*
			{
				return Result.Kinds.FindByPredicate(
					[&KindName](const FFlowMCPCourierOpKindGrammar& KindGrammar)
					{
						return KindGrammar.Kind == KindName;
					});
			};
			auto FindField = [](const FFlowMCPCourierOpKindGrammar& KindGrammar, const FString& FieldName) -> const FFlowMCPCourierFieldGrammar*
			{
				return KindGrammar.Fields.FindByPredicate(
					[&FieldName](const FFlowMCPCourierFieldGrammar& FieldGrammar)
					{
						return FieldGrammar.FieldName == FieldName;
					});
			};

			const FFlowMCPCourierOpKindGrammar* UpsertNodeKind = FindKind(TEXT("UpsertNode"));
			if (TestNotNull(TEXT("UpsertNode is described"), UpsertNodeKind))
			{
				const FFlowMCPCourierFieldGrammar* TypeField = FindField(*UpsertNodeKind, TEXT("type"));
				if (TestNotNull(TEXT("UpsertNode.type is described"), TypeField))
				{
					TestEqual(TEXT("UpsertNode.type is required only on create"), static_cast<uint8>(TypeField->Legality), static_cast<uint8>(EFlowMCPCourierFieldLegality::RequiredOnCreate));
				}
			}

			const FFlowMCPCourierOpKindGrammar* DeleteNodeKind = FindKind(TEXT("DeleteNode"));
			if (TestNotNull(TEXT("DeleteNode is described"), DeleteNodeKind))
			{
				const FFlowMCPCourierFieldGrammar* NewAliasField = FindField(*DeleteNodeKind, TEXT("newAlias"));
				if (TestNotNull(TEXT("DeleteNode.newAlias is described"), NewAliasField))
				{
					TestEqual(TEXT("DeleteNode.newAlias is illegal"), static_cast<uint8>(NewAliasField->Legality), static_cast<uint8>(EFlowMCPCourierFieldLegality::Illegal));
				}
			}

			const FFlowMCPCourierOpKindGrammar* AddConnectionKind = FindKind(TEXT("AddConnection"));
			if (TestNotNull(TEXT("AddConnection is described"), AddConnectionKind))
			{
				const FFlowMCPCourierFieldGrammar* SourceField = FindField(*AddConnectionKind, TEXT("source"));
				if (TestNotNull(TEXT("AddConnection.source is described"), SourceField))
				{
					TestEqual(TEXT("AddConnection.source is required"), static_cast<uint8>(SourceField->Legality), static_cast<uint8>(EFlowMCPCourierFieldLegality::Required));
				}

				const bool bHasSourceEndpointGroup = AddConnectionKind->ExactlyOneOfGroups.ContainsByPredicate(
					[](const FFlowMCPCourierFieldGroup& Group)
					{
						return Group.FieldNames.Contains(TEXT("source.nodeGuid")) && Group.FieldNames.Contains(TEXT("source.nodeAlias"));
					});
				TestTrue(TEXT("AddConnection declares an exactly-one-of group for source.nodeGuid/source.nodeAlias"), bHasSourceEndpointGroup);
			}

			// A flat per-field legality of Optional cannot, by itself, distinguish "this field is
			// genuinely optional" from "one of this field and its sibling is required". The
			// ExactlyOneOfGroups array is what carries that constraint, so DeleteAddon's
			// parentGuid/parentAlias pair - each individually Optional - must appear there.
			const FFlowMCPCourierOpKindGrammar* DeleteAddonKind = FindKind(TEXT("DeleteAddon"));
			if (TestNotNull(TEXT("DeleteAddon is described"), DeleteAddonKind))
			{
				const bool bHasParentGroup = DeleteAddonKind->ExactlyOneOfGroups.ContainsByPredicate(
					[](const FFlowMCPCourierFieldGroup& Group)
					{
						return Group.FieldNames.Contains(TEXT("parentGuid")) && Group.FieldNames.Contains(TEXT("parentAlias"));
					});
				TestTrue(TEXT("DeleteAddon declares an exactly-one-of group for parentGuid/parentAlias"), bHasParentGroup);
			}
		});
	});

	Describe(TEXT("Catalog"), [this]()
	{
		It(TEXT("Lists FlowAsset types with counts only"), [this]()
		{
			FFlowMCPListFlowAssetTypesResult Result;
			ExceptionHandler->CaptureErrorsIn([&Result]()
			{
				Result = UFlowMCPToolset::ListFlowAssetTypes(FFlowMCPListFlowAssetTypesRequest());
			});

			ExpectNoException();
			TestTrue(TEXT("At least one FlowAsset subclass found"), Result.FoundCount > 0);
			TestEqual(TEXT("FoundCount matches array length"), Result.FoundCount, Result.AssetTypes.Num());
		});

		It(TEXT("FindFlowNodeTypes defaults to the shape section, returning histograms and no rows"), [this]()
		{
			FFlowMCPFindFlowNodeTypesRequest Request;
			Request.Kind = EFlowCatalogKind::Node;
			FFlowMCPFindFlowNodeTypesResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeTypes(Request);
			});

			ExpectNoException();
			TestEqual(TEXT("No rows emitted"), Result.Nodes.Num(), 0);
			TestTrue(TEXT("Shape still reports a node count"), Result.Shape.NodeCount > 0);
			TestTrue(TEXT("Shape reports categories"), Result.Shape.Categories.Num() > 0);
		});

		It(TEXT("FindFlowNodeTypes names section emits a stem plus the authoritative class path"), [this]()
		{
			FFlowMCPFindFlowNodeTypesRequest Request;
			Request.ClassNames = { TEXT("FlowNode_Start") };
			Request.Sections = TEXT("names");
			FFlowMCPFindFlowNodeTypesResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeTypes(Request);
			});

			ExpectNoException();
			TestEqual(TEXT("Exactly one node resolved"), Result.Nodes.Num(), 1);
			if (Result.Nodes.Num() == 1)
			{
				TestEqual(TEXT("Stem emitted rather than the full class name"), Result.Nodes[0].Name, FString(TEXT("Start")));
				TestFalse(TEXT("Class path is always present"), Result.Nodes[0].ClassPath.IsEmpty());
				TestTrue(TEXT("Description not gathered for a names-only request"), Result.Nodes[0].Description.IsEmpty());
			}
		});

		It(TEXT("FindFlowNodeTypes full section returns own properties and pins"), [this]()
		{
			FFlowMCPFindFlowNodeTypesRequest Request;
			Request.ClassNames = { TEXT("FlowNode_Start") };
			Request.Sections = TEXT("full");
			FFlowMCPFindFlowNodeTypesResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeTypes(Request);
			});

			ExpectNoException();
			TestEqual(TEXT("Exactly one node resolved"), Result.Nodes.Num(), 1);
			if (Result.Nodes.Num() == 1)
			{
				TestTrue(TEXT("Origin reported as native"), Result.Nodes[0].Origin == EFlowClassOrigin::Native);
				TestTrue(TEXT("Output pins reported"), Result.Nodes[0].OutputPins.Num() > 0);
			}
		});

		It(TEXT("FindFlowNodeTypes reports unresolved class names instead of silently dropping them"), [this]()
		{
			FFlowMCPFindFlowNodeTypesRequest Request;
			Request.ClassNames = { TEXT("FlowNode_Start"), TEXT("FlowNode_DoesNotExist_Bogus") };
			FFlowMCPFindFlowNodeTypesResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeTypes(Request);
			});

			ExpectNoException();
			TestEqual(TEXT("Only the valid name resolved"), Result.Nodes.Num(), 1);
			TestTrue(
				TEXT("Bogus name reported as unresolved"),
				Result.UnresolvedClassNames.Contains(TEXT("FlowNode_DoesNotExist_Bogus")));
		});

		It(TEXT("FindFlowNodeTypes rejects an unknown section rather than silently returning less"), [this]()
		{
			FFlowMCPFindFlowNodeTypesRequest Request;
			Request.ClassNames = { TEXT("FlowNode_Start") };
			Request.Sections = TEXT("pinz");
			FFlowMCPFindFlowNodeTypesResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeTypes(Request);
			});

			ExpectExceptionContains(TEXT("Unknown section"));
			TestEqual(TEXT("No rows returned on error"), Result.Nodes.Num(), 0);
		});

		It(TEXT("SetFlowAgentDoc refuses a doc with no guidance"), [this]()
		{
			FFlowMCPSetFlowAgentDocRequest Request;
			Request.ClassName = TEXT("FlowNode_Start");
			FFlowMCPSetFlowAgentDocResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::SetFlowAgentDoc(Request);
			});

			ExpectExceptionContains(TEXT("Guidance is required"));
		});

		It(TEXT("SetFlowAgentDoc reports a compiled-in target as not persisted and hands back source to paste"), [this]()
		{
			// A runtime write to a native class's compiled-in default is lost on restart, so reporting
			// success here would look like it worked and then silently vanish.
			FFlowMCPSetFlowAgentDocRequest Request;
			Request.ClassName = TEXT("FlowNode_Start");
			Request.Guidance = TEXT("Every graph needs exactly one; a second start point is a graph authoring error.");
			Request.Tags = { TEXT("entry"), TEXT("start") };
			FFlowMCPSetFlowAgentDocResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::SetFlowAgentDoc(Request);
			});

			ExpectNoException();
			TestTrue(TEXT("Origin reported as native"), Result.Origin == EFlowClassOrigin::Native);
			TestFalse(TEXT("Not reported as persisted"), Result.bPersisted);
			TestFalse(TEXT("Source snippet supplied instead"), Result.SourceSnippet.IsEmpty());
			TestTrue(TEXT("Snippet sets the guidance default"), Result.SourceSnippet.Contains(TEXT("Every graph needs exactly one")));
			TestTrue(TEXT("Snippet declares the accessor override"), Result.SourceSnippet.Contains(TEXT("GetAgentDoc")));
		});

		It(TEXT("SetFlowAgentDoc hands a script class AngelScript, not C++"), [this]()
		{
			// Script classes receive an editor-only source snippet rather than a C++ override.
			// The EDITOR guard keeps AgentDoc out of runtime script compilation.
			FFlowMCPFindFlowNodeTypesRequest FindRequest;
			FindRequest.Kind = EFlowCatalogKind::Any;
			FindRequest.Sections = TEXT("names,origin");
			FindRequest.Origin = EFlowClassOrigin::AngelScript;
			FindRequest.Limit = 1;
			FFlowMCPFindFlowNodeTypesResult FindResult;
			ExceptionHandler->CaptureErrorsIn([&FindRequest, &FindResult]()
			{
				FindResult = UFlowMCPToolset::FindFlowNodeTypes(FindRequest);
			});
			if (FindResult.Nodes.Num() + FindResult.Addons.Num() == 0)
			{
				// No script-authored Flow class in this project; nothing to assert about the snippet.
				return;
			}

			const FString ScriptClassPath = FindResult.Nodes.Num() > 0
				? FindResult.Nodes[0].ClassPath
				: FindResult.Addons[0].ClassPath;

			FFlowMCPSetFlowAgentDocRequest Request;
			Request.ClassName = ScriptClassPath;
			Request.Guidance = TEXT("Placeholder guidance for the script snippet test, long enough to be real.");
			Request.Tags = { TEXT("script"), TEXT("test") };

			FFlowMCPSetFlowAgentDocResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::SetFlowAgentDoc(Request);
			});

			ExpectNoException();
			TestTrue(TEXT("Origin reported as angelscript"), Result.Origin == EFlowClassOrigin::AngelScript);
			TestFalse(TEXT("Not persisted"), Result.bPersisted);
			TestTrue(TEXT("Snippet sets the guidance default"), Result.SourceSnippet.Contains(TEXT("Placeholder guidance for the script snippet test")));
			TestTrue(TEXT("Snippet is guarded for the editor"), Result.SourceSnippet.Contains(TEXT("#if EDITOR")));
			TestTrue(TEXT("Snippet sets the inherited guidance default"), Result.SourceSnippet.Contains(TEXT("default AgentDoc.Guidance")));
			TestTrue(TEXT("Tags go through Add with a name literal"), Result.SourceSnippet.Contains(TEXT("Tags.Add(n\"script\")")));
			TestFalse(TEXT("No C++ override is offered for a class with no header"),
				Result.SourceSnippet.Contains(TEXT("const FFlowAgentDoc&")));
		});

		It(TEXT("FindFlowNodeUsage answers zero usage without treating it as an error"), [this]()
		{
			FFlowMCPFindFlowNodeUsageRequest Request;
			Request.ClassName = TEXT("FlowNode_Start");
			FFlowMCPFindFlowNodeUsageResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeUsage(Request);
			});

			ExpectNoException();
			TestFalse(TEXT("Class path resolved"), Result.ClassPath.IsEmpty());
			TestTrue(TEXT("Instance count is never negative"), Result.InstanceCount >= 0);
			TestEqual(TEXT("Snippets are opt-in only"), Result.Snippets.Num(), 0);
		});

		It(TEXT("FindFlowNodeUsage rejects an empty class name"), [this]()
		{
			FFlowMCPFindFlowNodeUsageRequest Request;
			FFlowMCPFindFlowNodeUsageResult Result;
			ExceptionHandler->CaptureErrorsIn([&Request, &Result]()
			{
				Result = UFlowMCPToolset::FindFlowNodeUsage(Request);
			});

			ExpectExceptionContains(TEXT("ClassName cannot be empty"));
		});
	});

	Describe(TEXT("Search"), [this]()
	{
		It(TEXT("Rejects an invalid pin direction"), [this]()
		{
			FFlowMCPSearchFlowAssetsRequest Request;
			Request.Query = TEXT("Out");
			Request.PinDirection = TEXT("sideways");
			ExceptionHandler->CaptureErrorsIn([&Request]()
			{
				UFlowMCPToolset::SearchFlowAssets(Request);
			});

			ExpectExceptionContains(TEXT("PinDirection must be"));
		});

		It(TEXT("Uses the main query for connected output pins"), [this]()
		{
			FFlowMCPImportAndRegraphFlowAssetRequest ImportRequest;
			ImportRequest.FlowGraphText = ValidFlowGraphText;
			ImportRequest.AssetPath = MakeTestAssetPath();
			ImportRequest.Mutation.bSave = false;
			FFlowMCPImportAndRegraphFlowAssetResult ImportResult;
			ExceptionHandler->CaptureErrorsIn([&ImportRequest, &ImportResult]()
			{
				ImportResult = UFlowMCPToolset::ImportAndRegraphFlowAsset(ImportRequest);
			});
			ExpectNoException();
			if (ImportResult.AssetPath.IsEmpty())
			{
				return;
			}

			FFlowMCPSearchFlowAssetsRequest SearchRequest;
			SearchRequest.Query = TEXT("Out");
			SearchRequest.Flags = static_cast<int32>(EFlowSearchFlags::PinNames);
			SearchRequest.Scope = EFlowSearchScope::ThisAssetOnly;
			SearchRequest.ContextAssetPath = ImportResult.AssetPath;
			SearchRequest.PinDirection = TEXT("output");
			SearchRequest.PinConnection = TEXT("connected");
			FFlowMCPSearchFlowAssetsResult SearchResult;
			ExceptionHandler->CaptureErrorsIn([&SearchRequest, &SearchResult]()
			{
				SearchResult = UFlowMCPToolset::SearchFlowAssets(SearchRequest);
			});

			ExpectNoException();
			if (TestEqual(TEXT("One connected output matches"), SearchResult.ResultCount, 1))
			{
				TestEqual(TEXT("One matched pin is returned"), SearchResult.Results[0].MatchedPins.Num(), 1);
				TestEqual(TEXT("Matched output pin name"), SearchResult.Results[0].MatchedPins[0].PinName, TEXT("Out"));
				TestEqual(TEXT("Matched output direction"), SearchResult.Results[0].MatchedPins[0].Direction, TEXT("output"));
				TestTrue(TEXT("Matched output is connected"), SearchResult.Results[0].MatchedPins[0].bConnected);
			}
		});
	});
}

#endif
