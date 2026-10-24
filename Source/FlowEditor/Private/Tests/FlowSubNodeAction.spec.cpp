// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Graph/FlowGraphSchema_Actions.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowSubNodeActionSearchTest, "Flow.Editor.SubNodeAction.Search",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlowSubNodeActionSearchTest::RunTest(const FString& Parameters)
{
	FGraphActionListBuilderBase Actions;
	const FText Category = FText::FromString(TEXT("Flow|AddOns"));
	const FText Keywords = FText::FromString(TEXT("DistinctKeyword"));
	TSharedPtr<FFlowSchemaAction_NewSubNode> Action = FFlowSchemaAction_NewSubNode::AddNewSubNodeAction(
		Actions, Category, FText::FromString(TEXT("Test AddOn")), FText::GetEmpty(), Keywords);

	TestEqual(TEXT("The action is registered once"), Actions.GetNumActions(), 1);
	TestEqual(TEXT("The category chain is built once"), Action->GetCategoryChain().Num(), 2);
	TestTrue(TEXT("Keywords are indexed during construction"),
		Action->GetSearchKeywordsArray().Contains(TEXT("DistinctKeyword")));
	TestTrue(TEXT("Keywords are searchable"), Action->GetFullSearchText().Contains(TEXT("distinctkeyword")));
	return true;
}

#endif
