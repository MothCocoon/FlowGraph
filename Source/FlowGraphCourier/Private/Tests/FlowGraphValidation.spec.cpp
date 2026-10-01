// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowGraphReconciler.h"
#include "FlowGraphValidation.h"
#include "FlowGraphImporter.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "Json/FlowGraphValidationFixtures.h"

// Tests for the agnostic-core pre-commit validator: class resolution, duplicate-GUID integrity,
// connection fan-out, palette allowedness, and attachment eligibility. Tests run end-to-end through
// ComputeReconcilePlan + ExecuteReconcilePlan, since validation runs on the existing-asset path.

BEGIN_DEFINE_SPEC(FFlowGraphValidationSpec, "FlowGraphCourier.EditorGame.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UFlowAsset* TestFlowAsset = nullptr;
	TArray<UBlueprint*> TestBlueprints;

	// Creates and compiles a Blueprint whose asset class is BlueprintClass and whose generated class
	// derives from ParentClass. Passing UBlueprint::StaticClass() reproduces exactly what the generic
	// create_blueprint path emits - the malformed shape these tests exist to reject.
	UBlueprint* MakeBlueprint(UClass* BlueprintClass, UClass* ParentClass, const FString& Name)
	{
		UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Game/Test/%s"), *Name));
		if (!Package)
		{
			return nullptr;
		}

		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
			ParentClass, Package, FName(*Name), BPTYPE_Normal,
			BlueprintClass, UBlueprintGeneratedClass::StaticClass());
		if (!Blueprint)
		{
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		TestBlueprints.Add(Blueprint);
		return Blueprint;
	}

	static bool HasErrorCode(const FFlowReconcileResult& Result, const FString& Code)
	{
		for (const FFlowValidationFinding& Finding : Result.ValidationFindings)
		{
			if (Finding.Severity == EFlowValidationSeverity::Error && Finding.Code == Code)
			{
				return true;
			}
		}
		return false;
	}

	static FString DescribeReconcileFailure(const FFlowReconcileResult& Result, const FString& ErrorMessage)
	{
		FString Description = ErrorMessage;
		for (const FFlowValidationFinding& Finding : Result.ValidationFindings)
		{
			Description += FString::Printf(TEXT(" [%s] %s"), *Finding.Code, *Finding.Message);
		}
		for (const FString& Finding : Result.Findings)
		{
			Description += FString::Printf(TEXT(" %s"), *Finding);
		}
		return Description;
	}

	static FFlowReconcileResult ApplyMutationText(const FString& AssetPath, const FString& MutationText, bool bDryRun, FString& OutErrorMessage)
	{
		TSharedPtr<FFlowReconcileExecutionPlan> Plan = FFlowGraphReconciler::ComputeReconcilePlan(AssetPath, MutationText, OutErrorMessage);
		if (!Plan)
		{
			return FFlowReconcileResult{};
		}
		if (bDryRun)
		{
			FFlowReconcileResult DryResult;
			DryResult.bSuccess = true;
			DryResult.Plan = Plan->Plan;
			DryResult.AliasMap = Plan->AliasMap;
			DryResult.ValidationFindings = Plan->ValidationFindings;
			DryResult.NodesTouched = Plan->NodesTouched;
			DryResult.NodesPreserved = Plan->NodesPreserved;
			DryResult.InputSizeBytes = Plan->InputSizeBytes;
			return DryResult;
		}
		return FFlowGraphReconciler::ExecuteReconcilePlan(*Plan, AssetPath, OutErrorMessage);
	}

	// Imports a Courier v2 JSON fixture document into a brand-new asset - the document-based
	// equivalent of the deleted UFlowGraphImporter::ImportFlowGraphFromText.
	static UFlowAsset* ImportFixtureDocument(const FString& TargetAssetPath, const FString& Json, FString& OutErrorMessage)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		if (!FFlowCourierConverter::ParseDocument(Json, Document, Issues, OutErrorMessage))
		{
			return nullptr;
		}
		TArray<FFlowGraphParsedNode> ParsedNodes;
		TArray<FFlowGraphParsedConnection> ParsedConnections;
		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);
		return UFlowGraphImporter::ImportFlowGraphFromDocument(TargetAssetPath, Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections, OutErrorMessage);
	}

	// A minimal two-node fixture (Start "A" -> Finish "B") used as the pre-existing asset that
	// each validation test then applies a (deliberately-invalid) mutation against.
	static FString MakeFixtureText(const FString& AGuid, const FString& BGuid)
	{
		return FString::Printf(TEXT(
			"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
			"\"ops\":["
			"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
			"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
			"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
			"]}"
		), *AGuid, *BGuid, *AGuid, *BGuid);
	}

	static bool HasError(const FFlowReconcileResult& Result, const FString& MessageSubstring)
	{
		for (const FFlowValidationFinding& Finding : Result.ValidationFindings)
		{
			if (Finding.Severity == EFlowValidationSeverity::Error && Finding.Message.Contains(MessageSubstring))
			{
				return true;
			}
		}
		return false;
	}

	static bool HasWarning(const FFlowReconcileResult& Result, const FString& MessageSubstring)
	{
		for (const FFlowValidationFinding& Finding : Result.ValidationFindings)
		{
			if (Finding.Severity == EFlowValidationSeverity::Warning && Finding.Message.Contains(MessageSubstring))
			{
				return true;
			}
		}
		return false;
	}
END_DEFINE_SPEC(FFlowGraphValidationSpec)

void FFlowGraphValidationSpec::Define()
{
	Describe("FlowGraphValidation", [this]()
	{
		BeforeEach([this]() { TestFlowAsset = nullptr; });

		AfterEach([this]()
		{
			if (TestFlowAsset)
			{
				TestFlowAsset->ClearFlags(RF_Standalone);
				TestFlowAsset->MarkAsGarbage();
				TestFlowAsset = nullptr;
			}
		});

		const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A0");
		const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B0");

		It("rejects connection fan-out from one output pin and does not mutate the asset", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationFanOut");
			const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C0");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

			// Add node C, then fan A.Out out to both B.In and C.In - illegal (one target per pin).
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_FanOut, false, ReconcileError);
			TestFalse("Apply should fail on the fan-out", Result.bSuccess);
			TestTrue("Should report a fan-out validation Error", HasError(Result, TEXT("Fan-out")));
			TestEqual("Asset must be unchanged (no node added) when validation blocks apply", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});

		It("rejects a duplicate node GUID in the document", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationDupGuid");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_DuplicateNodeGuid, false, ReconcileError);
			TestFalse("Apply should fail on the duplicate GUID", Result.bSuccess);
			TestTrue("Should report a duplicate-node-GUID validation Error", HasError(Result, TEXT("Duplicate node GUID")));
		});

		It("rejects an unresolvable node class", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationBadClass");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_UnresolvableClass, false, ReconcileError);
			TestFalse("Apply should fail on the unresolvable class", Result.bSuccess);
			TestTrue("Should report a class-resolution validation Error", HasError(Result, TEXT("does not resolve")));
		});

		It("rejects a connection targeting a node that does not exist after the plan applies", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationDanglingNode");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

			// GValidationJson_DanglingNode's guid "...AA" is never declared as a node anywhere in
			// that document or on the asset.
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_DanglingNode, false, ReconcileError);
			TestFalse("Apply should fail on the dangling node reference", Result.bSuccess);
			TestTrue("Should report a dangling-node validation Error", HasError(Result, TEXT("does not exist after the plan applies")));
			TestEqual("Asset must be unchanged when validation blocks apply", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});

		It("rejects a connection referencing a pin that does not exist on the target node", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationDanglingPin");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

			// Node B (Finish) is real, but it has no "NoSuchPin" input pin.
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_DanglingPin, false, ReconcileError);
			TestFalse("Apply should fail on the dangling pin reference", Result.bSuccess);
			TestTrue("Should report a dangling-pin validation Error", HasError(Result, TEXT("pin that doesn't exist")));
			TestEqual("Asset must be unchanged when validation blocks apply", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});

		It("accepts a legal addon attachment (Switch + SwitchCase)", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationAttachOK");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			// Add a Switch node carrying a SwitchCase addon - SwitchCase is eligible under Switch
			// (TentativeAccept), so no attachment Error should be produced.
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_LegalAddonAttachment, false, ReconcileError);
			TestFalse("Should NOT report an attachment Error for a legal pairing", HasError(Result, TEXT("cannot attach")));
		});

		It("rejects an illegal addon attachment (SwitchCase under SwitchCase) and does not mutate the asset", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationAttachReject");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

			// A SwitchCase addon nested under another SwitchCase: the parent SwitchCase only accepts
			// IFlowPredicateInterface children and explicitly Rejects everything else, so the inner
			// SwitchCase (which implements IFlowSwitchCaseInterface, not IFlowPredicateInterface) is
			// an illegal attachment.
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_IllegalAddonAttachment, false, ReconcileError);
			TestFalse("Apply should fail on the illegal attachment", Result.bSuccess);
			TestTrue("Should report an attachment Error", HasError(Result, TEXT("cannot attach")));
			TestEqual("Asset must be unchanged when validation blocks apply", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});

		It("invokes a registered per-asset-class hook and blocks on its Error", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationHook");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			// The registry is a process-global static shared with every other test in this module -
			// register for the exact duration of this test only, and always unregister before
			// returning (even on an early-out), so no other test ever sees this hook.
			bool bHookWasCalled = false;
			FFlowValidationHookDelegate FakeHook = FFlowValidationHookDelegate::CreateLambda(
				[&bHookWasCalled](const UFlowAsset*, const TArray<FFlowGraphParsedNode>&, const TArray<FFlowGraphParsedConnection>&, TArray<FFlowValidationFinding>& OutFindings)
				{
					bHookWasCalled = true;
					FFlowValidationFinding& FakeFinding = OutFindings.AddDefaulted_GetRef();
					FakeFinding.Severity = EFlowValidationSeverity::Error;
					FakeFinding.Message = TEXT("Fake hook always rejects");
				});
			FFlowGraphValidation::RegisterHook(UFlowAsset::StaticClass(), FakeHook);

			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_TouchNodeB, false, ReconcileError);

			FFlowGraphValidation::UnregisterHooksForClass(UFlowAsset::StaticClass());

			TestTrue("The hook should have been invoked", bHookWasCalled);
			TestFalse("Apply should fail on the hook's Error", Result.bSuccess);
			TestTrue("Should report the hook's finding", HasError(Result, TEXT("Fake hook always rejects")));
		});

		It("accepts a valid property-edit mutation with no Error findings", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationValid");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			// A benign update: touch node B's block with no type (pure merge), no new classes.
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_TouchNodeB, false, ReconcileError);
			TestTrue("Apply should succeed on a valid document", Result.bSuccess);
			TestTrue("Should have no Error-severity findings", FFlowGraphValidation::HasNoErrors(Result.ValidationFindings));
		});

		// Pin type compatibility blocks invalid connections before mutation.
		It("rejects an exec-to-data cross connection as an Error and does not mutate the asset", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationExecDataCross");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

			// New FormatText node C1; wire its Text output into B's exec input pin (data -> exec).
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_ExecDataCross, false, ReconcileError);
			TestFalse("Apply should fail on the exec/data cross connection", Result.bSuccess);
			TestTrue("Should report an exec-to-data cross Error", HasError(Result, TEXT("exec pin to a data pin")));
			TestEqual("Asset must be unchanged when validation blocks apply", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});

		It("accepts a compatible data-to-data connection with no type findings", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationDataCompatible");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			// Two FormatText nodes; Text output -> Text input is compatible under any standard policy.
			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_CompatibleDataConnection, false, ReconcileError);
			TestTrue(*FString::Printf(TEXT("Apply should succeed on a compatible data connection: %s"),
				*DescribeReconcileFailure(Result, ReconcileError)), Result.bSuccess);
			TestFalse("Should NOT report an exec/data cross Error", HasError(Result, TEXT("exec pin to a data pin")));
			TestFalse("Should NOT report a data type mismatch Warning", HasWarning(Result, TEXT("Data pin type mismatch")));
		});

		It("rejects an incompatible data-to-data connection before mutation", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationDataIncompatible");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			// The Vector output and Bool input exist, but the asset's connection policy rejects
			// their pairing before the editor attempts to wire them.
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();
			FString ReconcileError;
			const FFlowReconcileResult Preview = ApplyMutationText(AssetPath, GValidationJson_IncompatibleDataConnection, true, ReconcileError);
			TestTrue("Dry run reports an incompatible data-pin Error", HasErrorCode(Preview, TEXT("DataPinTypeMismatch")));
			TestEqual("Dry run does not add nodes", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
			const FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_IncompatibleDataConnection, false, ReconcileError);
			TestFalse("Apply should reject the incompatible data connection", Result.bSuccess);
			TestTrue("Apply reports the blocking type mismatch", HasErrorCode(Result, TEXT("DataPinTypeMismatch")));
			TestEqual("Rejected patch does not add nodes", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});

		// Fan-out depends on pin type: an exec output drives one target, while a data output may
		// feed several inputs. The exec case is covered by the separate fan-out rejection test.
		It("allows fan-out from a data output pin to several data inputs", [this, GuidA, GuidB]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationDataFanOut");

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(GuidA, GuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, GValidationJson_DataFanOutIsLegal, false, ReconcileError);
			TestTrue(*FString::Printf(TEXT("Apply should succeed - data fan-out is legal: %s"),
				*DescribeReconcileFailure(Result, ReconcileError)), Result.bSuccess);
			TestFalse("Must NOT report a fan-out Error for a data pin", HasError(Result, TEXT("Fan-out")));
			TestTrue("Should have no Error-severity findings at all", FFlowGraphValidation::HasNoErrors(Result.ValidationFindings));
		});

		// Empirical, read-only check: IsNodeOrAddOnClassAllowed must not report a class that is
		// already present on a real shipped asset as disallowed - that would mean the
		// Error-severity palette check could block a legitimate apply. Uses only base
		// UFlowAsset/UFlowNodeBase API; no project-specific symbols in this module. RealAssetPaths
		// is intentionally empty here (this is a generic, portable plugin) - a consuming project
		// can extend this list locally with a few of its own real, complex shipped FlowAsset
		// paths for extra confidence that the palette validator has no false positives against
		// its actual content; the test no-ops cleanly with zero entries.
		It("does not report a real shipped asset's own existing node or addon classes as disallowed", [this]()
		{
			const TArray<FString> RealAssetPaths = {
			};

			int32 AssetsChecked = 0;
			int32 ClassesChecked = 0;

			TFunction<void(const UFlowAsset*, const UFlowNodeBase*)> CheckAddOnsRecursive =
				[this, &ClassesChecked, &CheckAddOnsRecursive](const UFlowAsset* Asset, const UFlowNodeBase* Owner)
			{
				for (const UFlowNodeAddOn* AddOn : Owner->GetFlowNodeAddOnChildren())
				{
					if (!IsValid(AddOn)) { continue; }
					ClassesChecked++;
					FText FailureReason;
					const bool bAllowed = Asset->IsNodeOrAddOnClassAllowed(AddOn->GetClass(), &FailureReason);
					TestTrue(FString::Printf(TEXT("Existing addon class '%s' on '%s' must be allowed (real shipped content): %s"),
						*AddOn->GetClass()->GetName(), *Asset->GetName(), *FailureReason.ToString()), bAllowed);
					CheckAddOnsRecursive(Asset, AddOn);
				}
			};

			for (const FString& AssetPath : RealAssetPaths)
			{
				UFlowAsset* RealAsset = LoadObject<UFlowAsset>(nullptr, *AssetPath);
				if (!RealAsset)
				{
					AddWarning(FString::Printf(TEXT("Skipping unavailable real-content fixture (not synced/moved?): %s"), *AssetPath));
					continue;
				}
				AssetsChecked++;

				for (const TPair<FGuid, UFlowNode*>& NodePair : RealAsset->GetNodes())
				{
					const UFlowNode* Node = NodePair.Value;
					if (!IsValid(Node)) { continue; }
					ClassesChecked++;
					FText FailureReason;
					const bool bAllowed = RealAsset->IsNodeOrAddOnClassAllowed(Node->GetClass(), &FailureReason);
					TestTrue(FString::Printf(TEXT("Existing node class '%s' on '%s' must be allowed (real shipped content): %s"),
						*Node->GetClass()->GetName(), *RealAsset->GetName(), *FailureReason.ToString()), bAllowed);
					CheckAddOnsRecursive(RealAsset, Node);
				}
			}

			if (AssetsChecked == 0)
			{
				AddInfo(TEXT("RealAssetPaths is empty (or none were available) - no-op pass. Add real shipped FlowAsset paths locally to exercise this check."));
			}
			else
			{
				AddInfo(FString::Printf(TEXT("Checked %d node/addon classes across %d real assets"), ClassesChecked, AssetsChecked));
			}
		});
	});

	// A Blueprint-generated node class is usable in a Flow graph only when its *asset* is a
	// UFlowNodeBlueprint or UFlowNodeAddOnBlueprint. A plain UBlueprint parented to UFlowNode
	// compiles and yields a UFlowNode-derived generated class, so every IsChildOf check passes -
	// yet the palette and the Courier catalog gather by asset class and never see it.
	Describe("NonFlowBlueprintRejection", [this]()
	{
		AfterEach([this]()
		{
			for (UBlueprint* Blueprint : TestBlueprints)
			{
				if (IsValid(Blueprint))
				{
					Blueprint->ClearFlags(RF_Standalone);
					Blueprint->MarkAsGarbage();
				}
			}
			TestBlueprints.Reset();

			if (TestFlowAsset)
			{
				TestFlowAsset->ClearFlags(RF_Standalone);
				TestFlowAsset->MarkAsGarbage();
				TestFlowAsset = nullptr;
			}
		});

		It("treats a native Flow node class as placeable", [this]()
		{
			UClass* NativeClass = UFlowGraphImporter::ResolveNodeClassForQuery(TEXT("/Script/Flow.FlowNode_Start"));
			TestNotNull("FlowNode_Start should resolve", NativeClass);

			FString Reason;
			TestFalse("A native class has no generating Blueprint and must never be rejected",
				UFlowGraphImporter::IsClassFromNonFlowBlueprint(NativeClass, Reason));
			TestTrue("An accepted class must carry no rejection reason", Reason.IsEmpty());
		});

		It("fails open on a null class rather than inventing a rejection", [this]()
		{
			FString Reason;
			TestFalse("Null must not be reported as a non-Flow Blueprint class",
				UFlowGraphImporter::IsClassFromNonFlowBlueprint(nullptr, Reason));
		});

		It("accepts a class generated by a FlowNodeBlueprint", [this]()
		{
			UBlueprint* Blueprint = MakeBlueprint(UFlowNodeBlueprint::StaticClass(), UFlowNode::StaticClass(), TEXT("BP_WellFormedFlowNode"));
			TestNotNull("Blueprint creation should succeed", Blueprint);
			if (!Blueprint) { return; }

			FString Reason;
			TestFalse("A FlowNodeBlueprint-generated class is the correct shape and must be accepted",
				UFlowGraphImporter::IsClassFromNonFlowBlueprint(Blueprint->GeneratedClass, Reason));
		});

		// Guards the sibling trap: UFlowNodeBlueprint and UFlowNodeAddOnBlueprint both derive directly
		// from UBlueprint, so a predicate testing only the node form would silently reject every addon
		// Blueprint in the project.
		It("accepts a class generated by a FlowNodeAddOnBlueprint", [this]()
		{
			UBlueprint* Blueprint = MakeBlueprint(UFlowNodeAddOnBlueprint::StaticClass(), UFlowNodeAddOn::StaticClass(), TEXT("BP_WellFormedFlowAddOn"));
			TestNotNull("Blueprint creation should succeed", Blueprint);
			if (!Blueprint) { return; }

			FString Reason;
			TestFalse("A FlowNodeAddOnBlueprint-generated class must be accepted",
				UFlowGraphImporter::IsClassFromNonFlowBlueprint(Blueprint->GeneratedClass, Reason));
		});

		It("rejects a class generated by a plain Blueprint and explains why", [this]()
		{
			UBlueprint* Blueprint = MakeBlueprint(UBlueprint::StaticClass(), UFlowNode::StaticClass(), TEXT("BP_MalformedFlowNode"));
			TestNotNull("Blueprint creation should succeed", Blueprint);
			if (!Blueprint) { return; }

			TestTrue("The malformed Blueprint must still derive from UFlowNode - that is what makes it deceptive",
				Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UFlowNode::StaticClass()));

			FString Reason;
			TestTrue("A plain-Blueprint-generated Flow node class must be rejected",
				UFlowGraphImporter::IsClassFromNonFlowBlueprint(Blueprint->GeneratedClass, Reason));
			TestTrue("The rejection must name the op that produces the correct asset",
				Reason.Contains(TEXT("CreateFlowNodeBlueprint")));
		});

		It("hides the malformed class from placement but keeps it findable for repair", [this]()
		{
			UBlueprint* Blueprint = MakeBlueprint(UBlueprint::StaticClass(), UFlowNode::StaticClass(), TEXT("BP_MalformedForResolve"));
			TestNotNull("Blueprint creation should succeed", Blueprint);
			if (!Blueprint || !Blueprint->GeneratedClass) { return; }

			const FString ClassPath = Blueprint->GeneratedClass->GetPathName();

			TestNull("ResolveNodeClass must fail closed so the class can never be placed",
				UFlowGraphImporter::ResolveNodeClass(ClassPath));
			TestNotNull("ResolveNodeClassForQuery must still find it, or FindNodes could not locate bad nodes",
				UFlowGraphImporter::ResolveNodeClassForQuery(ClassPath));
		});

		It("blocks a document that places a malformed node class, naming the real cause", [this]()
		{
			const FString AssetPath = TEXT("/Game/Test/TestValidationNonFlowBlueprint");
			const FString FixtureGuidA = TEXT("00000000-0000-0000-0000-0000000000A0");
			const FString FixtureGuidB = TEXT("00000000-0000-0000-0000-0000000000B0");

			UBlueprint* Blueprint = MakeBlueprint(UBlueprint::StaticClass(), UFlowNode::StaticClass(), TEXT("BP_MalformedForValidation"));
			TestNotNull("Blueprint creation should succeed", Blueprint);
			if (!Blueprint || !Blueprint->GeneratedClass) { return; }

			FString ImportError;
			TestFlowAsset = ImportFixtureDocument(AssetPath, MakeFixtureText(FixtureGuidA, FixtureGuidB), ImportError);
			TestNotNull("Fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }
			const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

			const FString MutationText = FString::Printf(TEXT(
				"{\"formatVersion\":2,\"mode\":\"Patch\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-0000000000E0\",\"type\":\"%s\"}]}"
			), *Blueprint->GeneratedClass->GetPathName());

			FString ReconcileError;
			FFlowReconcileResult Result = ApplyMutationText(AssetPath, MutationText, false, ReconcileError);

			TestFalse("Apply must fail on the malformed node class", Result.bSuccess);
			TestTrue("Should report the dedicated ClassFromNonFlowBlueprint code, not a generic resolution failure",
				HasErrorCode(Result, TEXT("ClassFromNonFlowBlueprint")));
			TestFalse("Must not mislead the author into thinking the class does not exist",
				HasErrorCode(Result, TEXT("ClassNotResolved")));
			TestEqual("Asset must be unchanged when validation blocks apply",
				TestFlowAsset->GetNodes().Num(), NodeCountBefore);
		});
	});
}

#endif
