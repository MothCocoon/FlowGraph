// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Interfaces/FlowPredicateInterface.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/ScopeExit.h"
#include "Nodes/Developer/FlowNode_Log.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/Route/FlowNode_Branch.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace FlowNodeDiscoveryTests
{
/** Registers an unsaved Blueprint fixture and removes its registry entry on destruction. */
struct FBlueprintFixture
{
	TStrongObjectPtr<UBlueprint> Blueprint;

	FBlueprintFixture(UClass* ParentClass, UClass* BlueprintClass)
	{
		const FString PackageName = TEXT("/Game/Test/FlowNodeDiscovery_") + FGuid::NewGuid().ToString();
		Blueprint.Reset(FKismetEditorUtilities::CreateBlueprint(ParentClass, CreatePackage(*PackageName),
			TEXT("TestNode"), BPTYPE_Normal, BlueprintClass, UBlueprintGeneratedClass::StaticClass()));
		if (ParentClass->IsChildOf(UFlowNodeAddOn::StaticClass()))
		{
			FBlueprintEditorUtils::ImplementNewInterface(Blueprint.Get(),
				UFlowPredicateInterface::StaticClass()->GetClassPathName());
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint.Get());
		FAssetRegistryModule::AssetCreated(Blueprint.Get());
	}

	~FBlueprintFixture()
	{
		FAssetRegistryModule::AssetDeleted(Blueprint.Get());
		Blueprint->GetOutermost()->SetDirtyFlag(false);
		Blueprint->ClearFlags(RF_Public | RF_Standalone);
	}
};

/** Builds the public node and AddOn menus without opening an asset editor or saving content. */
struct FMenuFixture
{
	TStrongObjectPtr<UFlowAsset> Asset{NewObject<UFlowAsset>()};
	UFlowGraph* Graph = nullptr;
	UFlowGraphNode* Parent = nullptr;

	FMenuFixture()
	{
		UFlowGraph::CreateGraph(Asset.Get(), UFlowGraphSchema::StaticClass(), /*bCreateDefaultNodes=*/false);
		Graph = CastChecked<UFlowGraph>(Asset->GetGraph());
		Parent = NewObject<UFlowGraphNode>(Graph);
		Parent->SetNodeTemplate(NewObject<UFlowNode_Branch>(Asset.Get()));
		Graph->AddNode(Parent, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	}

	TArray<UClass*> GetNodeClasses() const
	{
		FGraphActionMenuBuilder Menu;
		UFlowGraphSchema::GetPaletteActions(Menu, Asset.Get(), FString());
		TArray<UClass*> Classes;
		for (int32 ActionIndex = 0; ActionIndex < Menu.GetNumActions(); ++ActionIndex)
		{
			const TSharedPtr<FEdGraphSchemaAction>& Action = Menu.GetSchemaAction(ActionIndex);
			if (Action->GetTypeId() == FFlowGraphSchemaAction_NewNode::StaticGetTypeId())
			{
				Classes.Add(StaticCastSharedPtr<FFlowGraphSchemaAction_NewNode>(Action)->NodeClass);
			}
		}
		return Classes;
	}

	TArray<UClass*> GetAddOnClasses() const
	{
		FGraphContextMenuBuilder Menu(Graph);
		Menu.SelectedObjects.Add(Parent);
		GetDefault<UFlowGraphSchema>()->GetGraphNodeContextActions(Menu, /*SubNodeFlags=*/0);
		TArray<UClass*> Classes;
		for (int32 ActionIndex = 0; ActionIndex < Menu.GetNumActions(); ++ActionIndex)
		{
			const TSharedPtr<FFlowSchemaAction_NewSubNode> Action =
				StaticCastSharedPtr<FFlowSchemaAction_NewSubNode>(Menu.GetSchemaAction(ActionIndex));
			Classes.Add(Action->NodeTemplate->NodeInstanceClass.Get());
		}
		return Classes;
	}
};

static int32 CountClass(const TArray<UClass*>& Classes, const UClass* ExpectedClass)
{
	return Classes.FilterByPredicate([ExpectedClass](const UClass* Class) { return Class == ExpectedClass; }).Num();
}
}

BEGIN_DEFINE_SPEC(FFlowNodeDiscoverySpec, "FlowEditor.NodeDiscovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FFlowNodeDiscoverySpec)

void FFlowNodeDiscoverySpec::Define()
{
	using namespace FlowNodeDiscoveryTests;

	AfterEach([]()
	{
		// Drop any fixture classes from the native caches before a later test collects garbage.
		UFlowGraphSchema::RefreshNativeNodeCache();
	});

	It("KeepsNativeNodesAndAddOnsAvailableAfterRefresh", [this]()
	{
		FMenuFixture Menus;
		UFlowGraphSchema::RefreshNativeNodeCache();
		TestEqual(TEXT("The native Branch node appears once"),
			CountClass(Menus.GetNodeClasses(), UFlowNode_Branch::StaticClass()), 1);
		TestEqual(TEXT("The native AND AddOn appears once"),
			CountClass(Menus.GetAddOnClasses(), UFlowNodeAddOn_PredicateAND::StaticClass()), 1);
	});

	It("ListsALoadedBlueprintNodeExactlyOnceAfterRepeatedRefreshes", [this]()
	{
		FBlueprintFixture Node(UFlowNode::StaticClass(), UFlowNodeBlueprint::StaticClass());
		if (!TestNotNull(TEXT("The node Blueprint has a generated class"), Node.Blueprint->GeneratedClass.Get()))
		{
			return;
		}
		FMenuFixture Menus;
		TestTrue(TEXT("The node is allowed in the fixture asset"),
			Menus.Asset->IsNodeOrAddOnClassAllowed(Node.Blueprint->GeneratedClass));
		for (int32 RefreshIndex = 0; RefreshIndex < 2; ++RefreshIndex)
		{
			UFlowGraphSchema::RefreshNativeNodeCache();
			TestEqual(TEXT("The loaded Blueprint node appears once"),
				CountClass(Menus.GetNodeClasses(), Node.Blueprint->GeneratedClass), 1);
		}
	});

	It("ListsALoadedBlueprintAddOnExactlyOnceAfterRepeatedRefreshes", [this]()
	{
		FBlueprintFixture AddOn(UFlowNodeAddOn::StaticClass(), UFlowNodeAddOnBlueprint::StaticClass());
		if (!TestNotNull(TEXT("The AddOn Blueprint has a generated class"), AddOn.Blueprint->GeneratedClass.Get()))
		{
			return;
		}
		FMenuFixture Menus;
		TestTrue(TEXT("The AddOn is allowed in the fixture asset"),
			Menus.Asset->IsNodeOrAddOnClassAllowed(AddOn.Blueprint->GeneratedClass));
		TestTrue(TEXT("The Branch accepts the fixture AddOn"),
			UFlowGraphSchema::IsAddOnAllowedForSelectedObjects({Menus.Parent},
				AddOn.Blueprint->GeneratedClass->GetDefaultObject<UFlowNodeAddOn>()));
		for (int32 RefreshIndex = 0; RefreshIndex < 2; ++RefreshIndex)
		{
			UFlowGraphSchema::RefreshNativeNodeCache();
			TestEqual(TEXT("The loaded Blueprint AddOn appears once"),
				CountClass(Menus.GetAddOnClasses(), AddOn.Blueprint->GeneratedClass), 1);
		}
	});

	It("KeepsCategoriesAndMenusUsableAfterBlueprintRecompilationAndGarbageCollection", [this]()
	{
		FBlueprintFixture Node(UFlowNode::StaticClass(), UFlowNodeBlueprint::StaticClass());
		FBlueprintFixture AddOn(UFlowNodeAddOn::StaticClass(), UFlowNodeAddOnBlueprint::StaticClass());
		FMenuFixture Menus;
		UFlowGraphSchema::RefreshNativeNodeCache();
		for (int32 CompileIndex = 0; CompileIndex < 3; ++CompileIndex)
		{
			FKismetEditorUtilities::CompileBlueprint(Node.Blueprint.Get());
			FKismetEditorUtilities::CompileBlueprint(AddOn.Blueprint.Get());
			if (!TestTrue(TEXT("Both fixture Blueprints compiled successfully"),
				Node.Blueprint->Status == BS_UpToDate && AddOn.Blueprint->Status == BS_UpToDate))
			{
				return;
			}
			CollectGarbage(RF_NoFlags);
			TestFalse(TEXT("Category enumeration still succeeds after recompilation and collection"),
				UFlowGraphSchema::GetFlowNodeCategories().IsEmpty());
			TestEqual(TEXT("Only the current node class is listed once"),
				CountClass(Menus.GetNodeClasses(), Node.Blueprint->GeneratedClass), 1);
			TestEqual(TEXT("Only the current AddOn class is listed once"),
				CountClass(Menus.GetAddOnClasses(), AddOn.Blueprint->GeneratedClass), 1);
		}
	});

	It("ExcludesReplacedNodeAndAddOnClassesFromCategories", [this]()
	{
		UClass* NodeClass = UFlowNode_Log::StaticClass();
		UClass* AddOnClass = UFlowNodeAddOn_PredicateAND::StaticClass();
		const EClassFlags PreviousNodeFlags = NodeClass->ClassFlags;
		const EClassFlags PreviousAddOnFlags = AddOnClass->ClassFlags;
		UFlowGraphSettings* Settings = GetMutableDefault<UFlowGraphSettings>();
		const auto PreviousCategories = Settings->OverridenNodeCategories;
		ON_SCOPE_EXIT
		{
			NodeClass->ClassFlags = PreviousNodeFlags;
			AddOnClass->ClassFlags = PreviousAddOnFlags;
			Settings->OverridenNodeCategories = PreviousCategories;
			UFlowGraphSchema::RefreshNativeNodeCache();
		};
		const FString NodeCategory = TEXT("FlowNodeDiscovery_ReplacedNode_") + FGuid::NewGuid().ToString();
		const FString AddOnCategory = TEXT("FlowNodeDiscovery_ReplacedAddOn_") + FGuid::NewGuid().ToString();
		Settings->OverridenNodeCategories.Add(NodeClass, NodeCategory);
		Settings->OverridenNodeCategories.Add(AddOnClass, AddOnCategory);
		// AngelScript reload leaves replaced UClasses loaded with this flag set.
		NodeClass->ClassFlags |= CLASS_NewerVersionExists;
		AddOnClass->ClassFlags |= CLASS_NewerVersionExists;
		UFlowGraphSchema::RefreshNativeNodeCache();
		const TArray<TSharedPtr<FString>> Categories = UFlowGraphSchema::GetFlowNodeCategories();
		TestFalse(TEXT("A replaced node contributes no category"), Categories.ContainsByPredicate(
			[&NodeCategory](const TSharedPtr<FString>& Category) { return *Category == NodeCategory; }));
		TestFalse(TEXT("A replaced AddOn contributes no category"), Categories.ContainsByPredicate(
			[&AddOnCategory](const TSharedPtr<FString>& Category) { return *Category == AddOnCategory; }));
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
