// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Find/FindInFlow.h"
#include "Asset/FlowAssetEditor.h"
#include "Find/SFindInFlowFilterPopup.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowAsset.h"
#include "FlowEditorModule.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

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
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "FindInFlow"

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
	const UFlowGraphEditorSettings* Settings = GetDefault<UFlowGraphEditorSettings>();
	if (ensure(Settings))
	{
		MaxSearchDepth = Settings->DefaultMaxSearchDepth;
		SearchFlags = static_cast<EFlowSearchFlags>(Settings->DefaultSearchFlags);
		PinDirection = static_cast<EFlowSearchPinDirection>(Settings->DefaultSearchPinDirection);
		PinConnection = static_cast<EFlowSearchPinConnectionState>(Settings->DefaultSearchPinConnection);
		if (!EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::PinNames))
		{
			PinDirection = EFlowSearchPinDirection::Any;
			PinConnection = EFlowSearchPinConnectionState::Any;
		}
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
										const FFindInFlowApplyDelegate OnSaveAsDefault = FFindInFlowApplyDelegate::CreateLambda([](EFlowSearchFlags Flags)
											{
												if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
												{
													GraphEditorSettings->DefaultSearchFlags = static_cast<uint32>(Flags);
													GraphEditorSettings->SaveConfig();
												}
											});

										const TSharedRef<SFindInFlowFilterPopup> FilterPopup = SNew(SFindInFlowFilterPopup)
											.OnApply(FFindInFlowApplyDelegate::CreateLambda([this](EFlowSearchFlags NewSearchFlags)
												{
													SearchFlags = NewSearchFlags;
													if (!EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::PinNames))
													{
														PinDirection = EFlowSearchPinDirection::Any;
														PinConnection = EFlowSearchPinConnectionState::Any;
														if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
														{
															GraphEditorSettings->DefaultSearchPinDirection = 0;
															GraphEditorSettings->DefaultSearchPinConnection = 0;
															GraphEditorSettings->SaveConfig();
														}
													}
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
							SNew(SComboButton)
								.IsEnabled(this, &SFindInFlow::ArePinFiltersEnabled)
								.ToolTipText(this, &SFindInFlow::GetPinFilterToolTip)
								.OnGetMenuContent(this, &SFindInFlow::MakePinFilterMenu)
								.ButtonContent()
								[
									SNew(STextBlock)
										.Text(this, &SFindInFlow::GetPinFilterSummaryText)
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
	if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
	{
		GraphEditorSettings->DefaultMaxSearchDepth = NewDepth;
		GraphEditorSettings->SaveConfig();
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

TSharedRef<SWidget> SFindInFlow::MakePinFilterMenu()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(10.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PinFiltersTitle", "Pin Filters"))
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(82.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PinDirectionLabel", "Direction"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SSegmentedControl<EFlowSearchPinDirection>)
					.Value_Lambda([this]() { return PinDirection; })
					.OnValueChanged(this, &SFindInFlow::OnPinDirectionChanged)
					+ SSegmentedControl<EFlowSearchPinDirection>::Slot(EFlowSearchPinDirection::Any)
						.Text(LOCTEXT("PinDirectionAny", "Any"))
						.ToolTip(LOCTEXT("PinDirectionAnyTooltip", "Match input and output pin names."))
					+ SSegmentedControl<EFlowSearchPinDirection>::Slot(EFlowSearchPinDirection::Input)
						.Text(LOCTEXT("PinDirectionInput", "Input"))
						.ToolTip(LOCTEXT("PinDirectionInputTooltip", "Match input pin names only."))
					+ SSegmentedControl<EFlowSearchPinDirection>::Slot(EFlowSearchPinDirection::Output)
						.Text(LOCTEXT("PinDirectionOutput", "Output"))
						.ToolTip(LOCTEXT("PinDirectionOutputTooltip", "Match output pin names only."))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(82.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PinConnectivityLabel", "Connectivity"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SSegmentedControl<EFlowSearchPinConnectionState>)
					.Value_Lambda([this]() { return PinConnection; })
					.OnValueChanged(this, &SFindInFlow::OnPinConnectionChanged)
					+ SSegmentedControl<EFlowSearchPinConnectionState>::Slot(EFlowSearchPinConnectionState::Any)
						.Text(LOCTEXT("PinConnectionAny", "Any"))
						.ToolTip(LOCTEXT("PinConnectionAnyTooltip", "Match connected and unconnected pin names."))
					+ SSegmentedControl<EFlowSearchPinConnectionState>::Slot(EFlowSearchPinConnectionState::Connected)
						.Text(LOCTEXT("PinConnectionConnected", "Connected"))
						.ToolTip(LOCTEXT("PinConnectionConnectedTooltip", "Match connected pin names only."))
					+ SSegmentedControl<EFlowSearchPinConnectionState>::Slot(EFlowSearchPinConnectionState::Unconnected)
						.Text(LOCTEXT("PinConnectionUnconnected", "Unconnected"))
						.ToolTip(LOCTEXT("PinConnectionUnconnectedTooltip", "Match unconnected pin names only."))
				]
			]
		];
}

FText SFindInFlow::GetPinFilterSummaryText() const
{
	TArray<FText> ActiveFilters;
	if (PinDirection == EFlowSearchPinDirection::Input)
	{
		ActiveFilters.Add(LOCTEXT("PinDirectionInputSummary", "Input"));
	}
	else if (PinDirection == EFlowSearchPinDirection::Output)
	{
		ActiveFilters.Add(LOCTEXT("PinDirectionOutputSummary", "Output"));
	}

	if (PinConnection == EFlowSearchPinConnectionState::Connected)
	{
		ActiveFilters.Add(LOCTEXT("PinConnectionConnectedSummary", "Connected"));
	}
	else if (PinConnection == EFlowSearchPinConnectionState::Unconnected)
	{
		ActiveFilters.Add(LOCTEXT("PinConnectionUnconnectedSummary", "Unconnected"));
	}

	if (ActiveFilters.IsEmpty())
	{
		return LOCTEXT("PinsAnySummary", "Pins: Any");
	}
	return FText::Format(LOCTEXT("PinsSummary", "Pins: {0}"), FText::Join(LOCTEXT("PinsSummaryDelimiter", ", "), ActiveFilters));
}

FText SFindInFlow::GetPinFilterToolTip() const
{
	return ArePinFiltersEnabled()
		? LOCTEXT("PinFiltersTooltip", "Filter Pin Names matches by direction and connectivity.")
		: LOCTEXT("PinFiltersDisabledTooltip", "Enable the Pin Names search category to use pin filters.");
}

bool SFindInFlow::ArePinFiltersEnabled() const
{
	return EnumHasAnyFlags(SearchFlags, EFlowSearchFlags::PinNames);
}

void SFindInFlow::OnPinDirectionChanged(EFlowSearchPinDirection NewDirection)
{
	PinDirection = NewDirection;
	if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
	{
		GraphEditorSettings->DefaultSearchPinDirection = static_cast<uint8>(NewDirection);
		GraphEditorSettings->SaveConfig();
	}
	InitiateSearch();
}

void SFindInFlow::OnPinConnectionChanged(EFlowSearchPinConnectionState NewConnection)
{
	PinConnection = NewConnection;
	if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
	{
		GraphEditorSettings->DefaultSearchPinConnection = static_cast<uint8>(NewConnection);
		GraphEditorSettings->SaveConfig();
	}
	InitiateSearch();
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

	if (GCompilingBlueprint && SearchScope != EFlowSearchScope::ThisAssetOnly)
	{
		FSearchResult CompilePending = MakeShareable(new FFindInFlowResult(TEXT("Search deferred while Blueprints are compiling")));
		SearchResults.ItemsFound.Add(CompilePending);
		TreeView->RequestTreeRefresh();
		return;
	}

	FFlowSearchQuery Query;
	Query.SearchText   = SearchValue;
	Query.Flags        = SearchFlags;
	Query.Scope        = SearchScope;
	Query.MaxDepth     = MaxSearchDepth;
	Query.ContextAsset = CurrentAsset;
	Query.PinFilter.Direction = PinDirection;
	Query.PinFilter.ConnectionState = PinConnection;

	FScopedSlowTask SearchTask(1.0f, LOCTEXT("FlowSearchSlowTask", "Searching Flow assets..."));
	SearchTask.MakeDialogDelayed(0.5f);
	float LastReportedProgress = 0.0f;
	Query.OnAssetSearchProgress = [&SearchTask, &LastReportedProgress](int32 ProcessedAssets, int32 TotalAssets, const FString& CurrentAssetName)
	{
		const float CurrentProgress = TotalAssets > 0
			? FMath::Clamp(static_cast<float>(ProcessedAssets) / static_cast<float>(TotalAssets), 0.0f, 1.0f)
			: LastReportedProgress;
		const float ProgressDelta = FMath::Max(CurrentProgress - LastReportedProgress, 0.0f);
		LastReportedProgress = CurrentProgress;
		SearchTask.EnterProgressFrame(ProgressDelta, CurrentAssetName.IsEmpty()
			? LOCTEXT("FlowSearchProgress", "Searching Flow assets...")
			: FText::Format(LOCTEXT("FlowSearchProgressAsset", "Searching {0}..."), FText::FromString(CurrentAssetName)));
	};

	TArray<FFlowSearchResultItem> RawResults;
	FFlowSearch::Search(Query, RawResults);

	// Group flat results by asset into per-asset root nodes (same visual layout as before).
	TMap<FSoftObjectPath, FSearchResult> AssetRoots;
	for (const FFlowSearchResultItem& Item : RawResults)
	{
		FSearchResult& AssetRoot = AssetRoots.FindOrAdd(Item.AssetPath);
		if (!AssetRoot.IsValid())
		{
			UFlowAsset* Asset = Cast<UFlowAsset>(Item.AssetPath.TryLoad());
			const FString AssetName = Asset ? Asset->GetName() : Item.AssetPath.GetAssetName();
			AssetRoot = MakeShareable(new FFindInFlowResult(AssetName, Asset));
		}

		FSearchResult ResultItem = MakeResultItem(Item);
		ResultItem->Parent = AssetRoot;
		AssetRoot->Children.Add(ResultItem);
	}

	for (auto& KV : AssetRoots)
	{
		SearchResults.ItemsFound.Add(KV.Value);

		// Auto-expand the current asset's group.
		if (KV.Key == FSoftObjectPath(CurrentAsset))
		{
			TreeView->SetItemExpansion(KV.Value, true);
		}
	}

	if (SearchResults.ItemsFound.IsEmpty())
	{
		FSearchResult NoResults = MakeShareable(new FFindInFlowResult(TEXT("No results found")));
		SearchResults.ItemsFound.Add(NoResults);
	}

	TreeView->RequestTreeRefresh();
}

SFindInFlow::FSearchResult SFindInFlow::MakeResultItem(const FFlowSearchResultItem& Item) const
{
	UFlowAsset* Asset   = Cast<UFlowAsset>(Item.AssetPath.TryLoad());
	UEdGraphNode* EdNode = nullptr;
	if (Asset && Asset->GetGraph())
	{
		for (UEdGraphNode* Node : Asset->GetGraph()->Nodes)
		{
			if (Node && Node->NodeGuid == Item.NodeGuid)
			{
				EdNode = Node;
				break;
			}
		}
	}

	FSearchResult Result = MakeShareable(
		new FFindInFlowResult(Item.NodeTitle, nullptr, EdNode, Item.bIsSubGraphNode, Asset));
	Result->MatchedFlags          = Item.MatchedFlags;
	Result->MatchedPropertySnippet = Item.MatchedSnippet;
	return Result;
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
