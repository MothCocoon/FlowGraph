// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AddOns/FlowNodeAddOn.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/ScopeExit.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

// Access the protected context without initializing unrelated game-instance subsystems.
struct FFlowInstanceSafetyGameInstanceAccess : UGameInstance
{
	static void SetWorldContext(UGameInstance& Instance, FWorldContext* Context)
	{
		FWorldContext* UGameInstance::* ContextMember = &FFlowInstanceSafetyGameInstanceAccess::WorldContext;
		Instance.*ContextMember = Context;
	}
};

BEGIN_DEFINE_SPEC(FFlowInstanceSafetySpec, "FlowEditor.InstanceSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UFlowAsset> Asset;
	TStrongObjectPtr<UFlowSubsystem> Subsystem;
	FGuid NodeGuid;
	FNativeFlowAssetEvent PreviousAdded;
	FNativeFlowAssetEvent PreviousRemoved;
	bool bPreviousRejectInvalidNodes = false;

	TMap<FGuid, TObjectPtr<UFlowNode>>& MutableNodes()
	{
		const FMapProperty* Property = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
		return *Property->ContainerPtrToValuePtr<TMap<FGuid, TObjectPtr<UFlowNode>>>(Asset.Get());
	}

	bool IsTemplateRegistered() const
	{
		const FArrayProperty* Property = FindFProperty<FArrayProperty>(UFlowSubsystem::StaticClass(), TEXT("InstancedTemplates"));
		return Property->ContainerPtrToValuePtr<TArray<TObjectPtr<UFlowAsset>>>(Subsystem.Get())->Contains(Asset.Get());
	}
END_DEFINE_SPEC(FFlowInstanceSafetySpec)

void FFlowInstanceSafetySpec::Define()
{
	BeforeEach([this]()
	{
		PreviousAdded = UFlowSubsystem::OnInstancedTemplateAdded;
		PreviousRemoved = UFlowSubsystem::OnInstancedTemplateRemoved;
		UFlowSubsystem::OnInstancedTemplateAdded.Unbind();
		UFlowSubsystem::OnInstancedTemplateRemoved.Unbind();
		bPreviousRejectInvalidNodes = GetDefault<UFlowSettings>()->bRejectFlowInstancesWithInvalidNodes;
		GetMutableDefault<UFlowSettings>()->bRejectFlowInstancesWithInvalidNodes = true;
		GameInstance.Reset(NewObject<UGameInstance>(GEngine));
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		World->SetGameInstance(GameInstance.Get());
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.OwningGameInstance = GameInstance.Get();
		Context.SetCurrentWorld(World);
		FFlowInstanceSafetyGameInstanceAccess::SetWorldContext(*GameInstance.Get(), &Context);
		// Isolate the base subsystem from project-specific overrides and external instances.
		Subsystem.Reset(NewObject<UFlowSubsystem>(GameInstance.Get()));
		Asset.Reset(NewObject<UFlowAsset>());
		NodeGuid = FGuid::NewGuid();
		MutableNodes().Add(NodeGuid, NewObject<UFlowNode_Start>(Asset.Get()));
	});

	AfterEach([this]()
	{
		Subsystem->AbortActiveFlows();
		UFlowSubsystem::OnInstancedTemplateAdded = PreviousAdded;
		UFlowSubsystem::OnInstancedTemplateRemoved = PreviousRemoved;
		GetMutableDefault<UFlowSettings>()->bRejectFlowInstancesWithInvalidNodes = bPreviousRejectInvalidNodes;
		UWorld* World = GameInstance->GetWorld();
		FFlowInstanceSafetyGameInstanceAccess::SetWorldContext(*GameInstance.Get(), nullptr);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		Asset.Reset();
		GameInstance.Reset();
		Subsystem.Reset();
	});

	It("Creates and registers a healthy graph without editor graph parity", [this]()
	{
		UFlowAsset* Instance = Subsystem->CreateRootFlow(GameInstance.Get(), Asset.Get());
		if (!TestNotNull(TEXT("Healthy instance"), Instance))
		{
			return;
		}
		TestTrue(TEXT("Initialized"), Instance->IsInstanceInitialized());
		TestEqual(TEXT("Template instance count"), Asset->GetInstancesNum(), 1);
		TestEqual(TEXT("Root instance count"), Subsystem->GetRootInstances().Num(), 1);
		TestTrue(TEXT("Nodes are individually instanced"), Instance->GetNodes()[NodeGuid] != Asset->GetNodes()[NodeGuid]);
	});

	It("Rejects a null node before registering or initializing a graph", [this]()
	{
		MutableNodes()[NodeGuid] = nullptr;
		AddExpectedError(TEXT("Flow instance creation rejected an invalid node"), EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Rejected instance"), Subsystem->CreateRootFlow(GameInstance.Get(), Asset.Get()));
		TestEqual(TEXT("No template instances"), Asset->GetInstancesNum(), 0);
		TestEqual(TEXT("No root instances"), Subsystem->GetRootInstances().Num(), 0);
		TestFalse(TEXT("No template registration"), IsTemplateRegistered());

		FFlowMessageLog Messages;
		TestEqual(TEXT("Null-node validation"), Asset->ValidateAsset(Messages), EDataValidationResult::Invalid);
	});

	It("Preserves an existing template and instance when another creation is rejected", [this]()
	{
		UFlowAsset* Instance = Subsystem->CreateRootFlow(GameInstance.Get(), Asset.Get());
		if (!TestNotNull(TEXT("First instance"), Instance))
		{
			return;
		}
		UFlowNode* TemplateNode = MutableNodes()[NodeGuid];
		MutableNodes()[NodeGuid] = nullptr;
		AddExpectedError(TEXT("Flow instance creation rejected an invalid node"), EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Second instance rejected"), Subsystem->CreateFlowInstance(GameInstance.Get(), Asset.Get()));
		MutableNodes()[NodeGuid] = TemplateNode;
		TestTrue(TEXT("Existing template registration survives"), IsTemplateRegistered());
		TestEqual(TEXT("Existing template instance survives"), Asset->GetInstancesNum(), 1);
		TestEqual(TEXT("Existing root survives"), Subsystem->GetRootInstances().Num(), 1);
		TestTrue(TEXT("Existing instance remains initialized"), Instance->IsInstanceInitialized());
	});

	It("Removes a new template registration if the cloned graph contains a null node", [this]()
	{
		UFlowNode* TemplateNode = MutableNodes()[NodeGuid];
		UFlowSubsystem::OnInstancedTemplateAdded.BindLambda([this](UFlowAsset*)
		{
			// Simulate a node becoming unavailable between template preflight and cloning.
			MutableNodes()[NodeGuid] = nullptr;
		});
		AddExpectedError(TEXT("Flow instance creation rejected an invalid node"), EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Clone rejected"), Subsystem->CreateRootFlow(GameInstance.Get(), Asset.Get()));
		MutableNodes()[NodeGuid] = TemplateNode;
		TestFalse(TEXT("New registration removed"), IsTemplateRegistered());
		TestEqual(TEXT("No template instances"), Asset->GetInstancesNum(), 0);
		TestEqual(TEXT("No root instances"), Subsystem->GetRootInstances().Num(), 0);
	});

	It("Rejects unsaveable classes while allowing palette-only deprecation", [this]()
	{
		const FString PackageName = TEXT("/Game/Test/FlowInstanceSafety_") + FGuid::NewGuid().ToString();
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(UFlowNode::StaticClass(),
			CreatePackage(*PackageName), TEXT("TestNode"), BPTYPE_Normal,
			UFlowNodeBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		UClass* NodeClass = Blueprint->GeneratedClass;
		if (!TestNotNull(TEXT("Generated class"), NodeClass))
		{
			return;
		}
		UFlowNode* Node = NewObject<UFlowNode>(Asset.Get(), NodeClass);
		MutableNodes().Add(NodeGuid, Node);
		const FBoolProperty* DeprecatedProperty = FindFProperty<FBoolProperty>(UFlowNode::StaticClass(), TEXT("bNodeDeprecated"));
		DeprecatedProperty->SetPropertyValue_InContainer(Node, true);
		TestTrue(TEXT("Palette-only deprecation is saveable"), Asset->IsNodeOrAddOnClassAllowed(NodeClass));
		for (EClassFlags Flag : {CLASS_Abstract, CLASS_Deprecated, CLASS_NewerVersionExists})
		{
			NodeClass->ClassFlags |= Flag;
			ON_SCOPE_EXIT { NodeClass->ClassFlags &= ~Flag; };
			FText Reason;
			TestFalse(TEXT("Unsaveable node rejected"), Asset->IsNodeOrAddOnClassAllowed(NodeClass, &Reason));
			TestTrue(TEXT("Actionable reason"), Reason.ToString().Contains(TEXT("cannot be saved")));
			FFlowMessageLog Messages;
			TestEqual(TEXT("Asset validation rejects unsaveable node"), Asset->ValidateAsset(Messages),
				EDataValidationResult::Invalid);
		}
		TestFalse(TEXT("Abstract add-on class rejected"), Asset->IsNodeOrAddOnClassAllowed(UFlowNodeAddOn::StaticClass()));
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
