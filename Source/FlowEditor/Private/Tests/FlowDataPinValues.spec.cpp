// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/Developer/FlowNode_Log.h"
#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "Tests/FlowDataPinValuesTestTypes.h"
#include "Types/FlowDataPinBlueprintLibrary.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Types/FlowPinTypeTemplates.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

BEGIN_DEFINE_SPEC(FFlowDataPinValuesSpec, "FlowEditor.DataPinValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UFlowAsset> Asset;
	UFlowGraph* Graph = nullptr;
	TStrongObjectPtr<UFlowDataPinValuesTestNode> NativeNode;
	TStrongObjectPtr<UFlowNode_DefineProperties> LiteralNode;

	void AddNode(UFlowNode* Node)
	{
		UFlowGraphNode* GraphNode = NewObject<UFlowGraphNode>(Graph);
		GraphNode->CreateNewGuid();
		Graph->AddNode(GraphNode, false, false);
		Asset->RegisterNode(GraphNode->NodeGuid, Node);
		GraphNode->SetNodeTemplate(Node);
		GraphNode->MarkNeedsFullReconstruction();
		GraphNode->ReconstructNode();
	}

	void CheckLiteralValue(const TCHAR* Name, const FInstancedStruct& Value)
	{
		FFlowNamedDataPinProperty Property;
		Property.Name = Name;
		Property.DataPinValue.InitializeAsScriptStruct(Value.GetScriptStruct(), Value.GetMemory());
		LiteralNode->GetMutableNamedProperties().Add(MoveTemp(Property));
		LiteralNode->GetGraphNode()->ReconstructNode();

		const FFlowDataPinResult Result = LiteralNode->TrySupplyDataPin(Name);
		TestEqual(TEXT("Literal supplies successfully"), Result.Result, EFlowDataPinResolveResult::Success);
		TestTrue(TEXT("Literal retains its concrete value type"), Result.ResultValue.GetScriptStruct() == Value.GetScriptStruct());
		TestTrue(TEXT("Literal retains its values"), Result.ResultValue ==
			LiteralNode->GetMutableNamedProperties().Last().DataPinValue);
	}

	template <typename TValue>
	void CheckLiteral(const TCHAR* Name, const TValue& Value)
	{
		CheckLiteralValue(Name, FInstancedStruct::Make<TValue>(Value));
	}
END_DEFINE_SPEC(FFlowDataPinValuesSpec)

void FFlowDataPinValuesSpec::Define()
{
	BeforeEach([this]()
	{
		Asset.Reset(NewObject<UFlowAsset>());
		UFlowGraph::CreateGraph(Asset.Get(), UFlowGraphSchema::StaticClass(), false);
		Graph = CastChecked<UFlowGraph>(Asset->GetGraph());
		NativeNode.Reset(NewObject<UFlowDataPinValuesTestNode>(Asset.Get()));
		AddNode(NativeNode.Get());
		LiteralNode.Reset(NewObject<UFlowNode_DefineProperties>(Asset.Get()));
		AddNode(LiteralNode.Get());
	});

	AfterEach([this]()
	{
		LiteralNode.Reset();
		NativeNode.Reset();
		Graph = nullptr;
		Asset.Reset();
	});

	It("resolves native scalar and array defaults through the current APIs", [this]()
	{
		int32 Integer = 0;
		TestEqual(TEXT("Scalar resolves"), NativeNode->TryResolveDataPinValue<FFlowPinType_Int>(
			TEXT("Integer"), Integer), EFlowDataPinResolveResult::Success);
		TestEqual(TEXT("Scalar default retained"), Integer, 42);

		TArray<int32> Integers;
		TestEqual(TEXT("Array resolves"), NativeNode->TryResolveDataPinValues<FFlowPinType_Int>(
			TEXT("Integers"), Integers), EFlowDataPinResolveResult::Success);
		TestTrue(TEXT("Array order and values retained"), Integers == NativeNode->Integers);
		TestEqual(TEXT("Single resolution selects last value"), NativeNode->TryResolveDataPinValue<FFlowPinType_Int>(
			TEXT("Integers"), Integer), EFlowDataPinResolveResult::Success);
		TestEqual(TEXT("Last value retained"), Integer, 11);

		FVector Vector;
		TestEqual(TEXT("Native struct resolves"), NativeNode->TryResolveDataPinValue<FFlowPinType_Vector>(
			TEXT("Vector"), Vector), EFlowDataPinResolveResult::Success);
		TestEqual(TEXT("Native struct value retained"), Vector, NativeNode->Vector);
		FGameplayTagContainer Tags;
		TestEqual(TEXT("Native tag container resolves"),
			NativeNode->TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(TEXT("Tags"), Tags),
			EFlowDataPinResolveResult::Success);
	});

	It("retains every standard value wrapper", [this]()
	{
		CheckLiteral(TEXT("Bool"), FFlowDataPinValue_Bool(true));
		CheckLiteral(TEXT("Int"), FFlowDataPinValue_Int(TArray<int32>{ 5, 9 }));
		CheckLiteral(TEXT("Int64"), FFlowDataPinValue_Int64(int64(12345678901)));
		CheckLiteral(TEXT("Float"), FFlowDataPinValue_Float(1.5f));
		CheckLiteral(TEXT("Double"), FFlowDataPinValue_Double(2.5));
		CheckLiteral(TEXT("Name"), FFlowDataPinValue_Name(FName(TEXT("Value"))));
		CheckLiteral(TEXT("String"), FFlowDataPinValue_String(FString(TEXT("Value"))));
		CheckLiteral(TEXT("Text"), FFlowDataPinValue_Text(FText::FromString(TEXT("Value"))));
		FInstancedStruct EnumValue;
		EnumValue.InitializeAs(FFlowDataPinValue_Enum::StaticStruct());
		FFlowDataPinValue_Enum& Enum = EnumValue.GetMutable<FFlowDataPinValue_Enum>();
		Enum.EnumClass = StaticEnum<EFlowDataMultiType>();
		Enum.Values.Add(TEXT("Array"));
		CheckLiteralValue(TEXT("Enum"), EnumValue);
		CheckLiteral(TEXT("Vector"), FFlowDataPinValue_Vector(FVector(1, 2, 3)));
		CheckLiteral(TEXT("Rotator"), FFlowDataPinValue_Rotator(FRotator(10, 20, 30)));
		CheckLiteral(TEXT("Transform"), FFlowDataPinValue_Transform(FTransform(FVector(1, 2, 3))));
		CheckLiteral(TEXT("Tag"), FFlowDataPinValue_GameplayTag(FGameplayTag()));
		CheckLiteral(TEXT("Tags"), FFlowDataPinValue_GameplayTagContainer(FGameplayTagContainer()));
		CheckLiteral(TEXT("Struct"), FFlowDataPinValue_InstancedStruct(FInstancedStruct::Make<FVector>(1, 2, 3)));
		CheckLiteral(TEXT("Object"), FFlowDataPinValue_Object(NativeNode.Get()));
		CheckLiteral(TEXT("Class"), FFlowDataPinValue_Class(UFlowDataPinValuesTestNode::StaticClass()));
	});

	It("retains numeric conversions and reports empty arrays and unknown pins", [this]()
	{
		double Number = 0;
		TestEqual(TEXT("Integer converts to double"), NativeNode->TryResolveDataPinValue<FFlowPinType_Double>(
			TEXT("Integer"), Number), EFlowDataPinResolveResult::Success);
		TestEqual(TEXT("Converted value retained"), Number, 42.0);
		TestEqual(TEXT("Unknown pin remains an error"), NativeNode->TrySupplyDataPin(TEXT("Missing")).Result,
			EFlowDataPinResolveResult::FailedUnknownPin);
		NativeNode->Integers.Reset();
		int32 Integer = 0;
		TestEqual(TEXT("Empty scalar extraction fails"), NativeNode->TryResolveDataPinValue<FFlowPinType_Int>(
			TEXT("Integers"), Integer), EFlowDataPinResolveResult::FailedInsufficientValues);
	});

	It("resolves the log message default without a connected data pin", [this]()
	{
		TStrongObjectPtr<UFlowNode_Log> LogNode{NewObject<UFlowNode_Log>(Asset.Get())};
		const FStrProperty* MessageProperty = FindFProperty<FStrProperty>(UFlowNode_Log::StaticClass(), TEXT("Message"));
		MessageProperty->SetPropertyValue_InContainer(LogNode.Get(), TEXT("Default message"));
		AddNode(LogNode.Get());
		FString Message;
		TestEqual(TEXT("Log message resolves"), LogNode->TryResolveDataPinValue<FFlowPinType_String>(
			TEXT("Message"), Message), EFlowDataPinResolveResult::Success);
		TestEqual(TEXT("Log message default retained"), Message, FString(TEXT("Default message")));
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
