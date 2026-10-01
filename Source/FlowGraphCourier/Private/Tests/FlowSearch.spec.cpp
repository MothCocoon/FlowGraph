// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Find/FlowSearch.h"

#include "FlowAsset.h"
#include "FlowGraphImporter.h"
#include "FlowGraphRegrapher.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "Graph/FlowGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_Reroute.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Nodes/Actor/FlowNode_ExecuteComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UnrealType.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Exercises FFlowSearch without a Slate widget: token AND-matching, per-flag categories,
// search scope, subgraph recursion and cycle detection, and the maximum depth cutoff.
BEGIN_DEFINE_SPEC(FFlowSearchSpec, "FlowGraphCourier.EditorGame.Search", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TArray<UFlowAsset*> TestAssets;

	UFlowAsset* ImportAsset(const FString& PackagePath, const FString& Json)
	{
		// ImportFlowGraphFromDocument alone only builds runtime UFlowNodes; FFlowSearch iterates the
		// editor UEdGraph (Asset->GetGraph()->Nodes) and reads UEdGraphNode::NodeComment, so the
		// import must also regraph and apply parsed Comment/Pos data - ImportAndRegraphFromDocument
		// does both in one call.
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		FString ParseError;
		if (!FFlowCourierConverter::ParseDocument(Json, Document, Issues, ParseError))
		{
			return nullptr;
		}
		TArray<FFlowGraphParsedNode> ParsedNodes;
		TArray<FFlowGraphParsedConnection> ParsedConnections;
		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);
		UFlowAsset* Asset = UFlowGraphRegrapher::ImportAndRegraphFromDocument(PackagePath, Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections);
		if (Asset)
		{
			// A never-saved test package is otherwise treated as "not installed" by the IoStore
			// loader when something later resolves a soft reference into it (e.g. a FlowNode_SubGraph
			// node's Asset TSoftObjectPtr), which skips before ever checking for the in-memory object.
			if (UPackage* Package = Asset->GetOutermost())
			{
				Package->MarkAsFullyLoaded();
			}
			TestAssets.Add(Asset);
		}
		return Asset;
	}
END_DEFINE_SPEC(FFlowSearchSpec)

void FFlowSearchSpec::Define()
{
	Describe("FlowSearch", [this]()
	{
		AfterEach([this]()
		{
			for (UFlowAsset* Asset : TestAssets)
			{
				if (Asset)
				{
					Asset->ClearFlags(RF_Standalone);
					Asset->MarkAsGarbage();
				}
			}
			TestAssets.Empty();
		});

		// A single node carrying a distinct value in each searchable category: a comment
		// (Comments), its class name (Classes), and an editable string property (PropertyValues).
		const FString SingleNodeDoc = TEXT(
			"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
			"\"ops\":["
			"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Log\","
			"\"comment\":\"Hello World Marker\","
			"\"properties\":{\"Message\":\"SecretPropertyValue123\"},"
			"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
			"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
			"]}"
		);

		Describe("Token AND matching", [this, SingleNodeDoc]()
		{
			It("finds a match when every whitespace-delimited token is present", [this, SingleNodeDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchTokenAnd1"), SingleNodeDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("Hello World");
				Query.Flags = EFlowSearchFlags::Comments;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;

				TArray<FFlowSearchResultItem> Results;
				TestTrue("Search should find the comment when both tokens match", FFlowSearch::Search(Query, Results));
				TestEqual("Exactly one node should match", Results.Num(), 1);
			});

			It("finds no match when one token is absent (AND, not OR)", [this, SingleNodeDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchTokenAnd2"), SingleNodeDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("Hello Missing");
				Query.Flags = EFlowSearchFlags::Comments;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;

				TArray<FFlowSearchResultItem> Results;
				TestFalse("A token absent from the comment should fail the whole AND-match", FFlowSearch::Search(Query, Results));
				TestTrue("No results should be returned", Results.IsEmpty());
			});
		});

		Describe("Per-flag category matching", [this, SingleNodeDoc]()
		{
			It("matches the node comment only when Comments is enabled", [this, SingleNodeDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchFlagComment"), SingleNodeDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("Hello");
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;

				Query.Flags = EFlowSearchFlags::Comments;
				TArray<FFlowSearchResultItem> HitResults;
				TestTrue("Comments flag enabled should find the comment token", FFlowSearch::Search(Query, HitResults));

				Query.Flags = EFlowSearchFlags::Classes;
				TArray<FFlowSearchResultItem> MissResults;
				TestFalse("Comments-only text should not match when only Classes is enabled", FFlowSearch::Search(Query, MissResults));
			});

			It("matches the node's class name only when Classes is enabled", [this, SingleNodeDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchFlagClass"), SingleNodeDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("FlowNode_Log");
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;

				Query.Flags = EFlowSearchFlags::Classes;
				TArray<FFlowSearchResultItem> HitResults;
				TestTrue("Classes flag enabled should find the class name", FFlowSearch::Search(Query, HitResults));
				if (HitResults.Num() == 1)
				{
					TestTrue("Matched flags should report Classes", EnumHasAnyFlags(HitResults[0].MatchedFlags, EFlowSearchFlags::Classes));
				}

				Query.Flags = EFlowSearchFlags::Comments;
				TArray<FFlowSearchResultItem> MissResults;
				TestFalse("Class-name text should not match when only Comments is enabled", FFlowSearch::Search(Query, MissResults));
			});

			It("matches an editable property value only when PropertyValues is enabled", [this, SingleNodeDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchFlagProperty"), SingleNodeDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("SecretPropertyValue123");
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;

				Query.Flags = EFlowSearchFlags::PropertyValues;
				TArray<FFlowSearchResultItem> HitResults;
				TestTrue("PropertyValues flag enabled should find the Message property value", FFlowSearch::Search(Query, HitResults));

				Query.Flags = EFlowSearchFlags::Comments;
				TArray<FFlowSearchResultItem> MissResults;
				TestFalse("Property value text should not match when only Comments is enabled", FFlowSearch::Search(Query, MissResults));
			});
		});

		Describe("Scope", [this]()
		{
			It("ThisAssetOnly restricts results to the context asset, ignoring other loaded assets", [this]()
			{
				const FString DocA = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000011\",\"type\":\"/Script/Flow.FlowNode_Log\","
					"\"comment\":\"AssetAOnlyMarker\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
					"]}"
				);
				const FString DocB = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000012\",\"type\":\"/Script/Flow.FlowNode_Log\","
					"\"comment\":\"AssetBOnlyMarker\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
					"]}"
				);

				UFlowAsset* AssetA = ImportAsset(TEXT("/Game/Test/FlowSearchScopeA"), DocA);
				UFlowAsset* AssetB = ImportAsset(TEXT("/Game/Test/FlowSearchScopeB"), DocB);
				if (!TestNotNull("Asset A should import", AssetA) || !TestNotNull("Asset B should import", AssetB)) { return; }

				FFlowSearchQuery Query;
				Query.Flags = EFlowSearchFlags::Comments;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = AssetA;

				Query.SearchText = TEXT("AssetAOnlyMarker");
				TArray<FFlowSearchResultItem> ResultsForOwnMarker;
				TestTrue("Searching for A's own marker with A as context should find it", FFlowSearch::Search(Query, ResultsForOwnMarker));

				Query.SearchText = TEXT("AssetBOnlyMarker");
				TArray<FFlowSearchResultItem> ResultsForOtherMarker;
				TestFalse("Searching for B's marker with A as context (ThisAssetOnly) should find nothing", FFlowSearch::Search(Query, ResultsForOtherMarker));
			});
		});

		Describe("Subgraph recursion", [this]()
		{
			// Builds a fresh UFlowAsset with a single node carrying CommentMarker as its editor comment.
			auto BuildAssetWithMarkerNode = [this](const FString& CommentMarker) -> UFlowAsset*
			{
				UFlowAsset* Asset = NewObject<UFlowAsset>(GetTransientPackage(), UFlowAsset::StaticClass());
				TestAssets.Add(Asset);
				UFlowGraph::CreateGraph(Asset);

				UFlowGraph* FlowGraph = Cast<UFlowGraph>(Asset->GetGraph());
				if (!FlowGraph) { return Asset; }

				UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(FlowGraph);
				GraphNode->CreateNewGuid();
				GraphNode->NodeComment = CommentMarker;
				UFlowNode* FlowNode = Asset->CreateNode(UFlowNode_Reroute::StaticClass(), GraphNode);
				GraphNode->SetNodeTemplate(FlowNode);
				GraphNode->AllocateDefaultPins();
				FlowGraph->AddNode(GraphNode, false, false);
				FlowGraph->NotifyGraphChanged();

				return Asset;
			};

			// Adds a FlowNode_SubGraph node to OwnerAsset whose Asset reference is wired directly to
			// ChildAsset's live pointer via reflection - not a soft-path string. A path-based soft
			// reference into a purely in-memory, never-disk-loaded test package gets skipped by the
			// IoStore loader ("package does not exist on disk or in the loader") before FindObject is
			// ever tried, so LoadSynchronous() would fail even though the object is alive; assigning
			// the live pointer directly caches the resolved weak pointer and sidesteps that entirely.
			auto AddSubGraphNodeReferencing = [this](UFlowAsset* OwnerAsset, UFlowAsset* ChildAsset)
			{
				UFlowGraph* FlowGraph = Cast<UFlowGraph>(OwnerAsset->GetGraph());
				if (!FlowGraph) { return; }

				UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(FlowGraph);
				GraphNode->CreateNewGuid();
				UFlowNode* FlowNode = OwnerAsset->CreateNode(UFlowNode_SubGraph::StaticClass(), GraphNode);
				GraphNode->SetNodeTemplate(FlowNode);
				GraphNode->AllocateDefaultPins();
				FlowGraph->AddNode(GraphNode, false, false);
				FlowGraph->NotifyGraphChanged();

				FProperty* AssetProp = FlowNode->GetClass()->FindPropertyByName(TEXT("Asset"));
				if (FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(AssetProp))
				{
					SoftObjectProp->SetObjectPropertyValue(SoftObjectProp->ContainerPtrToValuePtr<void>(FlowNode), ChildAsset);
				}
			};

			It("finds a match inside a referenced subgraph when Subgraphs is enabled", [this, BuildAssetWithMarkerNode, AddSubGraphNodeReferencing]()
			{
				UFlowAsset* Child = BuildAssetWithMarkerNode(TEXT("ChildSecretMarker"));
				if (!TestNotNull("Child asset should be built", Child)) { return; }

				UFlowAsset* Parent = BuildAssetWithMarkerNode(TEXT(""));
				if (!TestNotNull("Parent asset should be built", Parent)) { return; }
				AddSubGraphNodeReferencing(Parent, Child);

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("ChildSecretMarker");
				Query.Flags = EFlowSearchFlags::Comments | EFlowSearchFlags::Subgraphs;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Parent;

				TArray<FFlowSearchResultItem> Results;
				const bool bFound = FFlowSearch::Search(Query, Results);
				TestTrue("Search from the parent should find the marker inside the child subgraph", bFound);
				if (bFound && TestEqual("Exactly one match expected", Results.Num(), 1))
				{
					TestTrue("Result should be flagged as coming from a subgraph", Results[0].bIsSubGraphNode);
					TestEqual("Subgraph owner asset path should be the parent", Results[0].SubgraphOwnerAssetPath, FSoftObjectPath(Parent));
				}
			});

			It("does not recurse into a subgraph when Subgraphs is disabled", [this, BuildAssetWithMarkerNode, AddSubGraphNodeReferencing]()
			{
				UFlowAsset* Child = BuildAssetWithMarkerNode(TEXT("ChildSecretMarkerTwo"));
				if (!TestNotNull("Child asset should be built", Child)) { return; }

				UFlowAsset* Parent = BuildAssetWithMarkerNode(TEXT(""));
				if (!TestNotNull("Parent asset should be built", Parent)) { return; }
				AddSubGraphNodeReferencing(Parent, Child);

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("ChildSecretMarkerTwo");
				Query.Flags = EFlowSearchFlags::Comments; // Subgraphs deliberately omitted.
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Parent;

				TArray<FFlowSearchResultItem> Results;
				TestFalse("Search should not recurse into the child subgraph without the Subgraphs flag", FFlowSearch::Search(Query, Results));
			});

			It("terminates and still finds the match on an A -> B -> A subgraph cycle", [this, BuildAssetWithMarkerNode, AddSubGraphNodeReferencing]()
			{
				UFlowAsset* Child = BuildAssetWithMarkerNode(TEXT("CycleSecretMarker"));
				if (!TestNotNull("Child asset should be built", Child)) { return; }

				UFlowAsset* Parent = BuildAssetWithMarkerNode(TEXT(""));
				if (!TestNotNull("Parent asset should be built", Parent)) { return; }

				AddSubGraphNodeReferencing(Parent, Child);
				AddSubGraphNodeReferencing(Child, Parent); // Closes the A -> B -> A cycle.

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("CycleSecretMarker");
				Query.Flags = EFlowSearchFlags::Comments | EFlowSearchFlags::Subgraphs;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Parent;

				TArray<FFlowSearchResultItem> Results;
				const bool bFound = FFlowSearch::Search(Query, Results);
				TestTrue("Search should terminate (cycle guard) and still find the marker via the child", bFound);
				TestEqual("The marker should be reported exactly once, not duplicated by the cycle", Results.Num(), 1);
			});
		});

		Describe("Pin constraints", [this]()
		{
			const FString ConnectedPinsDoc = TEXT(
				"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":["
				"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000021\",\"type\":\"/Script/Flow.FlowNode_Log\","
				"\"comment\":\"SourceMarker\","
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
				"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000022\",\"type\":\"/Script/Flow.FlowNode_Log\","
				"\"comment\":\"TargetMarker\","
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
				"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000021\",\"pin\":\"Out\"},"
				"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000022\",\"pin\":\"In\"}}"
				"]}"
			);

			It("uses the main query for pin names", [this, ConnectedPinsDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchPinOnly"), ConnectedPinsDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("Out");
				Query.Flags = EFlowSearchFlags::PinNames;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;
				Query.PinFilter.Direction = EFlowSearchPinDirection::Output;

				TArray<FFlowSearchResultItem> Results;
				TestTrue("The Pin Names category should use the main query", FFlowSearch::Search(Query, Results));
				TestEqual("Both nodes expose the Out pin", Results.Num(), 2);
				if (Results.Num() == 2)
				{
					TestEqual("Each result should report one matched pin", Results[0].MatchedPins.Num(), 1);
					TestEqual("The matched pin should be Out", Results[0].MatchedPins[0].PinName, FName(TEXT("Out")));
					TestEqual("The matched pin should be an output", Results[0].MatchedPins[0].Direction, EFlowSearchPinDirection::Output);
				}
			});

			It("finds connected output pins", [this, ConnectedPinsDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchConnectedOutput"), ConnectedPinsDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("Out");
				Query.Flags = EFlowSearchFlags::PinNames;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;
				Query.PinFilter.Direction = EFlowSearchPinDirection::Output;
				Query.PinFilter.ConnectionState = EFlowSearchPinConnectionState::Connected;

				TArray<FFlowSearchResultItem> Results;
				TestTrue("The connected source output should match", FFlowSearch::Search(Query, Results));
				if (TestEqual("Only the connected output should match", Results.Num(), 1))
				{
					TestEqual("The source node should match", Results[0].NodeGuid, FGuid(0, 0, 0, 0x21));
					TestTrue("Matched pin metadata should report connected", Results[0].MatchedPins[0].bConnected);
				}
			});

			It("finds connected input pins", [this, ConnectedPinsDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchConnectedInput"), ConnectedPinsDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("In");
				Query.Flags = EFlowSearchFlags::PinNames;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;
				Query.PinFilter.Direction = EFlowSearchPinDirection::Input;
				Query.PinFilter.ConnectionState = EFlowSearchPinConnectionState::Connected;

				TArray<FFlowSearchResultItem> Results;
				TestTrue("The connected target input should match", FFlowSearch::Search(Query, Results));
				if (TestEqual("Only the connected input should match", Results.Num(), 1))
				{
					TestEqual("The target node should match", Results[0].NodeGuid, FGuid(0, 0, 0, 0x22));
					TestEqual("Matched pin metadata should report input", Results[0].MatchedPins[0].Direction, EFlowSearchPinDirection::Input);
				}
			});

			It("finds unconnected output pins", [this, ConnectedPinsDoc]()
			{
				UFlowAsset* Asset = ImportAsset(TEXT("/Game/Test/FlowSearchUnconnectedOutput"), ConnectedPinsDoc);
				if (!TestNotNull("Asset should import", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("Out");
				Query.Flags = EFlowSearchFlags::PinNames;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;
				Query.PinFilter.Direction = EFlowSearchPinDirection::Output;
				Query.PinFilter.ConnectionState = EFlowSearchPinConnectionState::Unconnected;

				TArray<FFlowSearchResultItem> Results;
				TestTrue("The unconnected target output should match", FFlowSearch::Search(Query, Results));
				if (TestEqual("Only the unconnected output should match", Results.Num(), 1))
				{
					TestEqual("The target node should match", Results[0].NodeGuid, FGuid(0, 0, 0, 0x22));
					TestFalse("Matched pin metadata should report unconnected", Results[0].MatchedPins[0].bConnected);
				}
			});
		});

		Describe("Max depth cutoff", [this]()
		{
			// Builds a FlowNode_ExecuteComponent whose ComponentTemplate (a plain hard object
			// reference, one inline-object recursion level) is a USceneComponent carrying a
			// distinctive ComponentTags entry. The tag is only reachable one recursion level
			// below the node itself, so it isolates AppendPropertyValues' MaxDepth cutoff.
			auto BuildNodeWithNestedProperty = [this](const FString& AssetPath) -> UFlowAsset*
			{
				UFlowAsset* Asset = NewObject<UFlowAsset>(GetTransientPackage(), UFlowAsset::StaticClass(), *AssetPath);
				TestAssets.Add(Asset);
				UFlowGraph::CreateGraph(Asset);

				UFlowGraph* FlowGraph = Cast<UFlowGraph>(Asset->GetGraph());
				if (!FlowGraph) { return Asset; }

				UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(FlowGraph);
				GraphNode->CreateNewGuid();
				UFlowNode* FlowNode = Asset->CreateNode(UFlowNode_ExecuteComponent::StaticClass(), GraphNode);
				GraphNode->SetNodeTemplate(FlowNode);
				GraphNode->AllocateDefaultPins();
				FlowGraph->AddNode(GraphNode, false, false);
				FlowGraph->NotifyGraphChanged();

				USceneComponent* NestedComponent = NewObject<USceneComponent>(FlowNode, USceneComponent::StaticClass());
				NestedComponent->ComponentTags.Add(FName(TEXT("DeepMarkerXYZ")));

				FProperty* ComponentTemplateProp = FlowNode->GetClass()->FindPropertyByName(TEXT("ComponentTemplate"));
				if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(ComponentTemplateProp))
				{
					ObjectProp->SetObjectPropertyValue_InContainer(FlowNode, NestedComponent);
				}

				return Asset;
			};

			It("does not find a property one level beyond MaxDepth", [this, BuildNodeWithNestedProperty]()
			{
				UFlowAsset* Asset = BuildNodeWithNestedProperty(TEXT("FlowSearchMaxDepthAsset1"));
				if (!TestNotNull("Asset should be built", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("DeepMarkerXYZ");
				Query.Flags = EFlowSearchFlags::PropertyValues;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;
				Query.MaxDepth = 1;

				TArray<FFlowSearchResultItem> Results;
				TestFalse("MaxDepth 1 should not reach the nested component's own properties", FFlowSearch::Search(Query, Results));
			});

			It("finds a property within MaxDepth", [this, BuildNodeWithNestedProperty]()
			{
				UFlowAsset* Asset = BuildNodeWithNestedProperty(TEXT("FlowSearchMaxDepthAsset2"));
				if (!TestNotNull("Asset should be built", Asset)) { return; }

				FFlowSearchQuery Query;
				Query.SearchText = TEXT("DeepMarkerXYZ");
				Query.Flags = EFlowSearchFlags::PropertyValues;
				Query.Scope = EFlowSearchScope::ThisAssetOnly;
				Query.ContextAsset = Asset;
				Query.MaxDepth = 2;

				TArray<FFlowSearchResultItem> Results;
				TestTrue("MaxDepth 2 should reach the nested component's own properties", FFlowSearch::Search(Query, Results));
			});
		});
	});
}

#endif
