// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Find/FindInFlow.h"
#include "Asset/FlowAssetEditor.h"
#include "Find/SFindInFlowFilterPopup.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowAsset.h"
#include "FlowEditorModule.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBase.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Views/ITypedTableView.h"
#include "GraphEditor.h"
#include "HAL/PlatformMath.h"
#include "Input/Events.h"
#include "Internationalization/Internationalization.h"
#include "Layout/Children.h"
#include "Layout/WidgetPath.h"
#include "Math/Color.h"
#include "Misc/Attribute.h"
#include "Misc/EnumRange.h"
#include "Misc/ScopedSlowTask.h"
#include "SlotBase.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Templates/Casts.h"
#include "Types/SlateStructs.h"
#include "UObject/Class.h"
#include "UObject/ObjectPtr.h"
#include "UObject/TopLevelAssetPath.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "FindInFlow"

//////////////////////////////////////////////////////////////////////////
// FFindInFlowCache

TMap<TWeakObjectPtr<UEdGraphNode>, TMap<EFlowSearchFlags, TSet<FString>>> FFindInFlowCache::CategoryStringCache;

void FFindInFlowCache::OnFlowAssetChanged(UFlowAsset& ChangedFlowAsset)
{
	TArray<TWeakObjectPtr<UEdGraphNode>> EntriesToRemove;

	for (const auto& KV : CategoryStringCache)
	{
		const TWeakObjectPtr<UEdGraphNode>& EdNodePtr = KV.Key;

		UEdGraphNode* EdNode = EdNodePtr.Get();

		if (!IsValid(EdNode))
		{
			EntriesToRemove.Add(EdNodePtr);

			continue;
		}

		UEdGraph* EdGraph = ChangedFlowAsset.GetGraph();
		if (EdGraph->Nodes.Contains(EdNode))
		{
			EntriesToRemove.Add(EdNodePtr);
		}
	}

	for (const TWeakObjectPtr<UEdGraphNode>& EdNodePtr : EntriesToRemove)
	{
		CategoryStringCache.Remove(EdNodePtr);
	}
}

//////////////////////////////////////////////////////////////////////////
// FFindInFlowResult

FFindInFlowResult::FFindInFlowResult(const FString& InValue, UFlowAsset* InOwningFlowAsset)
	: Value(InValue)
	, OwningFlowAsset(InOwningFlowAsset)
{
}

FFindInFlowResult::FFindInFlowResult(const FString& InValue, TSharedPtr<FFindInFlowResult> InParent, UEdGraphNode* InNode, bool bInIsSubGraphNode, UFlowAsset* InOwningFlowAsset)
	: Value(InValue)
	, GraphNode(InNode)
	, OwningFlowAsset(InOwningFlowAsset)
	, Parent(InParent)
	, bIsSubGraphNode(bInIsSubGraphNode)
{
}

TSharedRef<SWidget> FFindInFlowResult::CreateIcon() const
{
	const FSlateColor IconColor = FSlateColor::UseForeground();
	const FSlateBrush* Brush = FAppStyle::GetBrush(TEXT("GraphEditor.FIB_Event"));

	return SNew(SImage)
		.Image(Brush)
		.ColorAndOpacity(IconColor);
}

FReply FFindInFlowResult::OnClick(TWeakPtr<class FFlowAssetEditor> FlowAssetEditorPtr)
{
	if (GraphNode.IsValid())
	{
		if (UEdGraph* Graph = GraphNode->GetGraph())
		{
			if (UFlowAsset* Asset = Cast<UFlowAsset>(Graph->GetOuter()))
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
				if (TSharedPtr<FFlowAssetEditor> Editor = FFlowGraphUtils::GetFlowAssetEditor(Graph))
				{
					Editor->JumpToNode(GraphNode.Get());
				}
			}
		}
	}
	else if (OwningFlowAsset.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(OwningFlowAsset.Get());
	}
	return FReply::Handled();
}

FReply FFindInFlowResult::OnDoubleClick() const
{
	if (bIsSubGraphNode && Parent.IsValid())
	{
		if (const UFlowGraphNode* ParentNode = Cast<UFlowGraphNode>(Parent.Pin()->GraphNode.Get()))
		{
			if (UFlowNode_SubGraph* SubGraph = Cast<UFlowNode_SubGraph>(ParentNode->GetFlowNodeBase()))
			{
				if (UObject* Target = SubGraph->GetAssetToEdit())
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Target);
					if (TSharedPtr<FFlowAssetEditor> Editor = FFlowGraphUtils::GetFlowAssetEditor(GraphNode->GetGraph()))
					{
						Editor->JumpToNode(GraphNode.Get());
					}
				}
			}
		}
	}

	return FReply::Handled();
}

FString FFindInFlowResult::GetDescriptionText() const
{
	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(GraphNode.Get()))
	{
		return FlowGraphNode->GetNodeDescription();
	}

	return FString();
}

FString FFindInFlowResult::GetCommentText() const
{
	return GraphNode.IsValid() ? GraphNode->NodeComment : FString();
}

FString FFindInFlowResult::GetNodeTypeText() const
{
	if (!GraphNode.IsValid())
	{
		return FString();
	}

	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(GraphNode.Get()))
	{
		if (UFlowNodeBase* Base = FlowGraphNode->GetFlowNodeBase())
		{
			return Base->GetClass()->GetDisplayNameText().ToString();
		}
	}

	return GraphNode->GetClass()->GetDisplayNameText().ToString();
}

FText FFindInFlowResult::GetToolTipText() const
{
	FString Tip = GetNodeTypeText() + TEXT("\n") + GetDescriptionText();

	if (!GetCommentText().IsEmpty())
	{
		Tip += TEXT("\n") + GetCommentText();
	}

	if (!MatchedPropertySnippet.IsEmpty())
	{
		Tip += TEXT("\n\nMatched: ") + MatchedPropertySnippet;
	}

	return FText::FromString(Tip);
}

FText FFindInFlowResult::GetMatchedSnippet() const
{
	return FText::FromString(MatchedPropertySnippet);
}

FText FFindInFlowResult::GetMatchedCategoriesText() const
{
	if (MatchedFlags == EFlowSearchFlags::None)
	{
		return FText::GetEmpty();
	}

	TArray<FText> DisplayNames;

	for (EFlowSearchFlags Flag : MakeFlagsRange(EFlowSearchFlags::All))
	{
		if (EnumHasAnyFlags(MatchedFlags, Flag))
		{
			FText DisplayName = UEnum::GetDisplayValueAsText(Flag);
			if (!DisplayName.IsEmpty())
			{
				DisplayNames.Add(DisplayName);
			}
		}
	}

	if (DisplayNames.Num() == 0)
	{
		return FText::GetEmpty();
	}

	return FText::Join(FText::FromString(TEXT(", ")), DisplayNames);
}

//////////////////////////////////////////////////////////////////////////
// SFindInFlow

void SFindInFlow::Construct(const FArguments& InArgs, TSharedPtr<class FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditorPtr = InFlowAssetEditor;
	SearchResults.Setup();

	// Load INI settings
	const UFlowGraphEditorSettings* Settings = UFlowGraphEditorSettings::Get();
	if (ensure(Settings))
	{
		MaxSearchDepth = Settings->DefaultMaxSearchDepth;
		SearchFlags = static_cast<EFlowSearchFlags>(Settings->DefaultSearchFlags);
	}

	// Populate scope options
	FLOW_ASSERT_ENUM_MAX(EFlowSearchScope, 3);
	for (EFlowSearchScope Scope : TEnumRange<EFlowSearchScope>())
	{
		if (FlowEnum::IsValidEnumValue(Scope))
		{
			ScopeOptionList.Add(MakeShareable(new EFlowSearchScope(Scope)));
		}
	}
	SelectedScopeOption = ScopeOptionList[0];

	SAssignNew(SearchTextField, SSearchBox)
		.OnTextCommitted(this, &SFindInFlow::OnSearchTextCommitted);

	SAssignNew(SearchButton, SButton)
		.Text(LOCTEXT("SearchButton", "Search"))
		.OnClicked(this, &SFindInFlow::OnSearchButtonClicked);

	SAssignNew(MaxDepthSpinBox, SSpinBox<int32>)
		.MinValue(0)
		.MaxValue(10)
		.Value(MaxSearchDepth)
		.OnValueChanged(this, &SFindInFlow::OnMaxDepthChanged)
		.ToolTipText(LOCTEXT("MaxDepthTooltip", "Maximum recursion depth when searching inside objects"));

	SAssignNew(TreeView, STreeViewType)
		.TreeItemsSource(&SearchResults.ItemsFound)
		.OnGenerateRow(this, &SFindInFlow::OnGenerateRow)
		.OnGetChildren(this, &SFindInFlow::OnGetChildren)
		.OnSelectionChanged(this, &SFindInFlow::OnTreeSelectionChanged)
		.OnMouseButtonDoubleClick(this, &SFindInFlow::OnTreeSelectionDoubleClicked);

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SearchTextField.ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SearchButton.ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("FiltersLabel", "Filters:"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
								.ToolTipText(LOCTEXT("EditFiltersTooltip", "Edit search filters"))
								.OnClicked_Lambda([this]()
									{
										FFindInFlowApplyDelegate OnSaveAsDefault = FFindInFlowApplyDelegate::CreateLambda([this](EFlowSearchFlags Flags)
											{
												if (UFlowGraphEditorSettings* Settings = UFlowGraphEditorSettings::Get())
												{
													Settings->DefaultSearchFlags = static_cast<uint32>(Flags);
													Settings->SaveConfig();
												}
											});

										TSharedRef<SFindInFlowFilterPopup> FilterPopup = SNew(SFindInFlowFilterPopup)
											.OnApply(FFindInFlowApplyDelegate::CreateLambda([this](EFlowSearchFlags NewSearchFlags)
												{
													SearchFlags = NewSearchFlags;

													InitiateSearch();
												}))
											.OnSaveAsDefault(OnSaveAsDefault)
											.InitialFlags(SearchFlags);

										FSlateApplication::Get().PushMenu(
											AsShared(),
											FWidgetPath(),
											FilterPopup,
											FSlateApplication::Get().GetCursorPos(),
											FPopupTransitionEffect::ContextMenu);

										return FReply::Handled();
									})
								[
									SNew(STextBlock)
										.Text_Lambda([this]()
											{
												int32 ActiveCount = FMath::CountBits(static_cast<uint32>(SearchFlags));
												return FText::Format(LOCTEXT("ActiveFilters", "{0} Active"), FText::AsNumber(ActiveCount));
											})
								]
						]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(SComboBox<TSharedPtr<EFlowSearchScope>>)
								.OptionsSource(&ScopeOptionList)
								.OnGenerateWidget(this, &SFindInFlow::GenerateScopeWidget)
								.OnSelectionChanged(this, &SFindInFlow::OnScopeChanged)
								[
									SNew(STextBlock).Text(this, &SFindInFlow::GetCurrentScopeText)
								]
						]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("MaxDepthLabel", "Max Depth:"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MaxDepthSpinBox.ToSharedRef()
						]
				]
			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							TreeView.ToSharedRef()
						]
				]
		];
}

void SFindInFlow::FocusForUse() const
{
	if (SearchTextField.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(SearchTextField.ToSharedRef());
		SearchTextField->SelectAllText();
	}
}

void SFindInFlow::OnSearchTextChanged(const FText& Text)
{
	SearchValue = Text.ToString();
}

void SFindInFlow::OnSearchTextCommitted(const FText& Text, ETextCommit::Type)
{
	SearchValue = Text.ToString();

	InitiateSearch();
}

FReply SFindInFlow::OnSearchButtonClicked()
{
	InitiateSearch();

	return FReply::Handled();
}

void SFindInFlow::OnScopeChanged(TSharedPtr<EFlowSearchScope> NewSelection, ESelectInfo::Type)
{
	SelectedScopeOption = NewSelection;
	SearchScope = *NewSelection;
}

void SFindInFlow::OnMaxDepthChanged(int32 NewDepth)
{
	MaxSearchDepth = NewDepth;

	// Save to INI
	if (UFlowGraphEditorSettings* Settings = UFlowGraphEditorSettings::Get())
	{
		Settings->DefaultMaxSearchDepth = NewDepth;
		Settings->SaveConfig();
	}
}

TSharedRef<SWidget> SFindInFlow::GenerateScopeWidget(TSharedPtr<EFlowSearchScope> Item) const
{
	return SNew(STextBlock)
		.Text(UEnum::GetDisplayValueAsText(*Item.Get()));
}

FText SFindInFlow::GetCurrentScopeText() const
{
	return UEnum::GetDisplayValueAsText(*SelectedScopeOption.Get());
}

void SFindInFlow::InitiateSearch()
{
	FFlowEditorModule* FlowEditorModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
	if (ensure(FlowEditorModule))
	{
		FlowEditorModule->RegisterForAssetChanges();
	}

	SearchResults.Reset();

	HighlightText = FText::FromString(SearchValue);
	TreeView->RequestTreeRefresh();

	if (SearchValue.IsEmpty())
	{
		return;
	}

	TArray<FString> Tokens;
	SearchValue.ParseIntoArray(Tokens, TEXT(" "), true);
	for (FString& Token : Tokens)
	{
		Token = Token.ToUpper();
	}

	TSharedPtr<FFlowAssetEditor> Editor = FlowAssetEditorPtr.Pin();
	if (!Editor.IsValid())
	{
		return;
	}

	UFlowAsset* CurrentAsset = Editor->GetFlowAsset();
	if (!CurrentAsset || !CurrentAsset->GetGraph())
	{
		return;
	}

	const TSubclassOf<UFlowAsset> CurrentAssetClass = CurrentAsset->GetClass();

	constexpr int32 Depth = 0;

	switch (SearchScope)
	{
	case EFlowSearchScope::ThisAssetOnly:
		{
			FSearchResult AssetRoot = MakeShareable(new FFindInFlowResult(CurrentAsset->GetName(), CurrentAsset));
			ProcessAsset(CurrentAsset, AssetRoot, Tokens, Depth);

			if (AssetRoot->Children.Num() > 0)
			{
				SearchResults.ItemsFound.Add(AssetRoot);

				// Auto-expand the current asset's results
				TreeView->SetItemExpansion(AssetRoot, true);
			}
		}
		break;

	case EFlowSearchScope::AllOfThisType:
	case EFlowSearchScope::AllFlowAssets:
		{
			FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			TArray<FAssetData> Assets;
			FARFilter Filter;
			Filter.bRecursiveClasses = true;

			if (SearchScope == EFlowSearchScope::AllFlowAssets)
			{
				Filter.ClassPaths.Add(FTopLevelAssetPath(UFlowAsset::StaticClass()->GetClassPathName()));
			}
			else
			{
				Filter.ClassPaths.Add(FTopLevelAssetPath(CurrentAsset->GetClass()->GetClassPathName()));
			}

			Registry.Get().GetAssets(Filter, Assets);

			FScopedSlowTask Task(Assets.Num(), LOCTEXT("SearchingAssets", "Searching Flow Assets..."));
			Task.MakeDialog();

			int32 CurrentAssetIndex = 0;

			for (const FAssetData& Data : Assets)
			{
				UFlowAsset* Asset = Cast<UFlowAsset>(Data.GetAsset());
				if (!IsValid(Asset))
				{
					continue;
				}

				CurrentAssetIndex++;

				Task.EnterProgressFrame(1, FText::Format(LOCTEXT("SearchingAsset", "Searching {0}/{1}: {2}..."), CurrentAssetIndex, Assets.Num(), FText::FromString(Asset->GetName())));

				FSearchResult AssetRoot = MakeShareable(new FFindInFlowResult(Asset->GetName(), Asset));
				ProcessAsset(Asset, AssetRoot, Tokens, Depth);

				if (AssetRoot->Children.Num() > 0)
				{
					SearchResults.ItemsFound.Add(AssetRoot);

					// Auto-expand only the current asset
					if (Asset == CurrentAsset)
					{
						TreeView->SetItemExpansion(AssetRoot, true);
					}
				}
			}
		}
		break;

	default:
		checkNoEntry();
		break;
	}

	// Add "No results" placeholder if nothing found
	if (SearchResults.ItemsFound.IsEmpty())
	{
		FSearchResult NoResults = MakeShareable(new FFindInFlowResult(TEXT("No results found")));
		SearchResults.ItemsFound.Add(NoResults);
	}

	TreeView->RequestTreeRefresh();
}

bool SFindInFlow::ProcessAsset(UFlowAsset* Asset, FSearchResult ParentResult, const TArray<FString>& Tokens, int32 Depth)
{
	if (!Asset || !Asset->GetGraph() || Depth >= MaxSearchDepth || SearchResults.VisitedAssets.Contains(Asset))
	{
		return false;
	}

	SearchResults.VisitedAssets.Add(Asset);
	
	bool bAnyMatches = false;

	for (UEdGraphNode* EdNode : Asset->GetGraph()->Nodes)
	{
		const TMap<EFlowSearchFlags, TSet<FString>>* CategoryStrings = BuildCategoryStrings(EdNode, Depth);

		if (!CategoryStrings)
		{
			continue;
		}

		EFlowSearchFlags NodeMatchedFlags = EFlowSearchFlags::None;

		for (const TPair<EFlowSearchFlags, TSet<FString>>& Pair : *CategoryStrings)
		{
			const TSet<FString>& StringSet = Pair.Value;
			if (EnumHasAnyFlags(SearchFlags, Pair.Key) && StringSetMatchesSearchTokens(Tokens, StringSet))
			{
				EnumAddFlags(NodeMatchedFlags, Pair.Key);
			}
		}

		if (NodeMatchedFlags != EFlowSearchFlags::None)
		{
			FString Title = EdNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
			if (Title.IsEmpty())
			{
				Title = EdNode->GetClass()->GetName();
			}

			FSearchResult Result = MakeShareable(new FFindInFlowResult(Title, ParentResult, EdNode, Depth > 0, Asset));
			Result->MatchedFlags = NodeMatchedFlags;
			ParentResult->Children.Add(Result);

			bAnyMatches = true;
		}

		bAnyMatches |= RecurseIntoSubgraphsIfEnabled(EdNode, ParentResult, Tokens, Depth);
	}

	return bAnyMatches;
}

bool SFindInFlow::RecurseIntoSubgraphsIfEnabled(UEdGraphNode* EdNode, FSearchResult ParentResult, const TArray<FString>& Tokens, int32 Depth)
{
	if (!EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Subgraphs))
	{
		return false;
	}

	UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
	if (!FlowGraphNode || !FlowGraphNode->GetFlowNodeBase())
	{
		return false;
	}

	UFlowNode_SubGraph* SubGraph = Cast<UFlowNode_SubGraph>(FlowGraphNode->GetFlowNodeBase());
	if (!SubGraph)
	{
		return false;
	}

	UFlowAsset* SubAsset = Cast<UFlowAsset>(SubGraph->GetAssetToEdit());
	if (!SubAsset)
	{
		return false;
	}

	const FString SubgraphStr =
		SearchResults.VisitedAssets.Contains(SubAsset) ?
		TEXT(" (repeat subgraph)") :
		TEXT(" (Subgraph)");

	const FString SubTitle = SubAsset->GetName() + SubgraphStr;
	FSearchResult SubResult = MakeShareable(new FFindInFlowResult(SubTitle, ParentResult, EdNode, true, SubAsset));

	// Subgraphs don't count against depth
	if (ProcessAsset(SubAsset, SubResult, Tokens, Depth))
	{
		ParentResult->Children.Add(SubResult);

		return true;
	}

	return false;
}

const TMap<EFlowSearchFlags, TSet<FString>>* SFindInFlow::BuildCategoryStrings(UEdGraphNode* EdNode, int32 Depth) const
{
	if (!IsValid(EdNode))
	{
		return nullptr;
	}

	// Check cache first
	if (const TMap<EFlowSearchFlags, TSet<FString>>* Cached = FFindInFlowCache::CategoryStringCache.Find(EdNode))
	{
		return Cached;
	}

	TMap<EFlowSearchFlags, TSet<FString>> NewResultMap;

	UpdateSearchFlagToStringMapForEdGraphNode(*EdNode, NewResultMap, Depth);

	UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
	if (IsValid(FlowGraphNode))
	{
		UFlowNodeBase* FlowNodeBase = FlowGraphNode->GetFlowNodeBase();
		if (IsValid(FlowNodeBase))
		{
			UpdateSearchFlagToStringMapForFlowNodeBase(*FlowNodeBase, NewResultMap, Depth);
		}
	}

	// Now add the new map to the search cache
	const TMap<EFlowSearchFlags, TSet<FString>>* AddedResultMap = &FFindInFlowCache::CategoryStringCache.Add(EdNode, NewResultMap);
	return AddedResultMap;
}

void SFindInFlow::UpdateSearchFlagToStringMapForEdGraphNode(const UEdGraphNode& EdGraphNode, TMap<EFlowSearchFlags, TSet<FString>>& SearchFlagToStringMap, int32 Depth) const
{
	// Comments
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Comments))
	{
		TSet<FString>& CommentsSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::Comments);
		CommentsSet.Add(EdGraphNode.NodeComment);
	}
}

void SFindInFlow::UpdateSearchFlagToStringMapForFlowNodeBase(const UFlowNodeBase& FlowNodeBase, TMap<EFlowSearchFlags, TSet<FString>>& SearchFlagToStringMap, int32 Depth) const
{
	// Node Titles
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Titles))
	{
		TSet<FString>& TitlesSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::Titles);
		TitlesSet.Add(FlowNodeBase.GetNodeTitle().ToString());
	}

	// Tooltips
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Tooltips))
	{
		TSet<FString>& TooltipsSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::Tooltips);
		TooltipsSet.Add(FlowNodeBase.GetNodeToolTip().ToString());
	}

	// Classes
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Classes))
	{
		TSet<FString>& ClassesSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::Classes);

		const FString DisplayName = FlowNodeBase.GetClass()->GetDisplayNameText().ToString();
		ClassesSet.Add(DisplayName);

		const FString NativeName = FlowNodeBase.GetClass()->GetName();
		ClassesSet.Add(NativeName);
	}

	// Descriptions
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Descriptions))
	{
		TSet<FString>& DescriptionsSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::Descriptions);

		DescriptionsSet.Add(FlowNodeBase.GetNodeDescription());
	}

	// Config Text
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::ConfigText))
	{
		TSet<FString>& ConfigSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::ConfigText);
		ConfigSet.Add(FlowNodeBase.GetNodeConfigText().ToString());
	}

	// Property-based scouring
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::PropertiesFlags))
	{
		AppendPropertyValues(&FlowNodeBase, FlowNodeBase.GetClass(), &FlowNodeBase, SearchFlagToStringMap, Depth);
	}

	// AddOns
	if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::AddOns))
	{
		TSet<FString>& AddOnsSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::AddOns);

		FlowNodeBase.ForEachAddOnConst([AddOnsSet, this, &SearchFlagToStringMap, &Depth](const UFlowNodeAddOn& AddOn)
			{
				// No depth penalty for AddOns
				UpdateSearchFlagToStringMapForFlowNodeBase(AddOn, SearchFlagToStringMap, Depth);

				return EFlowForEachAddOnFunctionReturnValue::Continue;
			});
	}
}

void SFindInFlow::AppendPropertyValues(const void* Container, const UStruct* Struct, const UObject* ParentObject, TMap<EFlowSearchFlags, TSet<FString>>& SearchFlagToStringMap, int32 Depth) const
{
	int32 MaxDepth = 1;
	if (const UFlowGraphEditorSettings* Settings = UFlowGraphEditorSettings::Get())
	{
		MaxDepth = Settings->DefaultMaxSearchDepth;
	}

	if (!Container || !Struct || !ParentObject || Depth >= MaxDepth)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Struct, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_SimpleDisplay | CPF_AdvancedDisplay | CPF_BlueprintVisible | CPF_Config))
		{
			continue;
		}

		const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Container);

		if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::PropertyNames))
		{
			TSet<FString>& PropertyNamesSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::PropertyNames);

			const FString DisplayName = Prop->GetMetaData(TEXT("DisplayName"));

			if (!DisplayName.IsEmpty())
			{
				PropertyNamesSet.Add(DisplayName);
			}

			PropertyNamesSet.Add(Prop->GetName());
		}

		if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::PropertyValues))
		{
			TSet<FString>& PropertyValuesSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::PropertyValues);

			FString ValueStr;
			UObject* MutableParentObject = const_cast<UObject*>(ParentObject);
			Prop->ExportText_InContainer(0, ValueStr, Container, nullptr, MutableParentObject, PPF_None);
			ValueStr = ValueStr.Replace(TEXT("\""), TEXT("")).TrimStartAndEnd();

			PropertyValuesSet.Add(ValueStr);
		}

		if (EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::Tooltips))
		{
			TSet<FString>& TooltipsSet = SearchFlagToStringMap.FindOrAdd(EFlowSearchFlags::Tooltips);
			TooltipsSet.Add(Prop->GetMetaData(TEXT("ToolTip")));
		}

		if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			// Recurse into structs (no depth penalty)
			AppendPropertyValues(ValuePtr, StructProp->Struct, ParentObject, SearchFlagToStringMap, Depth);
		}
		else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		{
			// Recurse into inline objects (incurs a depth penalty)
			UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
			if (IsValid(Obj) && !Obj->HasAnyFlags(RF_ClassDefaultObject))
			{
				AppendPropertyValues(Obj, Obj->GetClass(), Obj, SearchFlagToStringMap, Depth + 1);
			}
		}
	}
}

bool SFindInFlow::StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString)
{
	int32 MatchedTokenCount = 0;
	const int32 TotalTokenCount = Tokens.Num();

	// Must match all tokens
	for (const FString& Token : Tokens)
	{
		if (ComparisonString.Contains(Token))
		{
			++MatchedTokenCount;
		}
		else
		{
			break;
		}
	}

	if (MatchedTokenCount == TotalTokenCount)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SFindInFlow::StringSetMatchesSearchTokens(const TArray<FString>& Tokens, const TSet<FString>& StringSet)
{
	for (const FString& StringFromSet : StringSet)
	{
		if (StringMatchesSearchTokens(Tokens, StringFromSet))
		{
			return true;
		}
	}

	return false;
}

TSharedRef<ITableRow> SFindInFlow::OnGenerateRow(FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FSearchResult>, OwnerTable)
		.ToolTip(SNew(SToolTip).Text(InItem->GetToolTipText()))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2, 0)
				[
					InItem->CreateIcon()
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(InItem->Value))
						.HighlightText(HighlightText)
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(InItem->GetNodeTypeText()))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.8f, 1.0f)))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(4, 0)
				[
					SNew(STextBlock)
						.Text(InItem->GetMatchedCategoriesText())
						.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
				]
		];
}

void SFindInFlow::OnGetChildren(FSearchResult InItem, TArray<FSearchResult>& OutChildren)
{
	OutChildren = InItem->Children;
}

void SFindInFlow::OnTreeSelectionChanged(FSearchResult Item, ESelectInfo::Type)
{
	if (Item.IsValid())
	{
		Item->OnClick(FlowAssetEditorPtr);
	}
}

void SFindInFlow::OnTreeSelectionDoubleClicked(FSearchResult Item)
{
	if (Item.IsValid())
	{
		Item->OnDoubleClick();
	}
}

#undef LOCTEXT_NAMESPACE
