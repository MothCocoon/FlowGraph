// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetEditor.h"

#include "FlowEditorCommands.h"
#include "FlowEditorModule.h"
#include "FlowMessageLog.h"

#include "Asset/FlowAssetEditorContext.h"
#include "Asset/FlowAssetToolbar.h"
#include "Asset/FlowMessageLogListing.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Widgets/SFlowPalette.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "EdGraphUtilities.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "IMessageLogListing.h"
#include "Kismet2/DebuggerCommands.h"
#include "LevelEditor.h"
#include "MessageLogModule.h"
#include "Misc/UObjectToken.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SNodePanel.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#if ENABLE_SEARCH_IN_ASSET_EDITOR
#include "Source/Private/Widgets/SSearchBrowser.h"
#endif

#define LOCTEXT_NAMESPACE "FlowAssetEditor"

const FName FFlowAssetEditor::DetailsTab(TEXT("Details"));
const FName FFlowAssetEditor::GraphTab(TEXT("Graph"));
const FName FFlowAssetEditor::PaletteTab(TEXT("Palette"));
const FName FFlowAssetEditor::RuntimeLogTab(TEXT("RuntimeLog"));
const FName FFlowAssetEditor::SearchTab(TEXT("Search"));
const FName FFlowAssetEditor::ValidationLogTab(TEXT("ValidationLog"));

FFlowAssetEditor::FFlowAssetEditor()
	: FlowAsset(nullptr)
{
}

FFlowAssetEditor::~FFlowAssetEditor()
{
	GEditor->UnregisterForUndo(this);
}

void FFlowAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(FlowAsset);
}

void FFlowAssetEditor::PostUndo(bool bSuccess)
{
	HandleUndoTransaction();
}

void FFlowAssetEditor::PostRedo(bool bSuccess)
{
	HandleUndoTransaction();
}

void FFlowAssetEditor::HandleUndoTransaction()
{
	SetUISelectionState(NAME_None);
	GraphEditor->NotifyGraphChanged();
	FSlateApplication::Get().DismissAllMenus();
}

void FFlowAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		GraphEditor->NotifyGraphChanged();
	}
}

FName FFlowAssetEditor::GetToolkitFName() const
{
	return FName("FlowEditor");
}

FText FFlowAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "FlowAsset Editor");
}

FString FFlowAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "FlowAsset").ToString();
}

FLinearColor FFlowAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

void FFlowAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_FlowAssetEditor", "Flow Editor"));
	const auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(DetailsTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Details))
				.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(GraphTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Graph))
				.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(PaletteTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Palette))
				.SetDisplayName(LOCTEXT("PaletteTab", "Palette"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));

	InTabManager->RegisterTabSpawner(RuntimeLogTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_RuntimeLog))
				.SetDisplayName(LOCTEXT("RuntimeLog", "Runtime Log"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults"));

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	InTabManager->RegisterTabSpawner(SearchTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Search))
				.SetDisplayName(LOCTEXT("SearchTab", "Search"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults"));
#endif

	InTabManager->RegisterTabSpawner(ValidationLogTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_ValidationLog))
				.SetDisplayName(LOCTEXT("ValidationLog", "Validation Log"))
				.SetGroup(WorkspaceMenuCategoryRef)
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Debug"));
}

void FFlowAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(DetailsTab);
	InTabManager->UnregisterTabSpawner(GraphTab);
	InTabManager->UnregisterTabSpawner(ValidationLogTab);
	InTabManager->UnregisterTabSpawner(PaletteTab);
#if ENABLE_SEARCH_IN_ASSET_EDITOR
	InTabManager->UnregisterTabSpawner(SearchTab);
#endif
}

void FFlowAssetEditor::InitToolMenuContext(FToolMenuContext& MenuContext)
{
	FAssetEditorToolkit::InitToolMenuContext(MenuContext);

	UFlowAssetEditorContext* Context = NewObject<UFlowAssetEditorContext>();
	Context->FlowAssetEditor = SharedThis(this);
	MenuContext.AddObject(Context);
}

bool FFlowAssetEditor::IsTabFocused(const FTabId& TabId) const
{
	if (const TSharedPtr<SDockTab> CurrentGraphTab = GetToolkitHost()->GetTabManager()->FindExistingLiveTab(TabId))
	{
		return CurrentGraphTab->IsActive();
	}

	return false;
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == DetailsTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("FlowDetailsTitle", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Graph(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == GraphTab);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("FlowGraphTitle", "Graph"));

	if (GraphEditor.IsValid())
	{
		SpawnedTab->SetContent(GraphEditor.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Palette(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == PaletteTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("FlowPaletteTitle", "Palette"))
		[
			Palette.ToSharedRef()
		];
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_RuntimeLog(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == RuntimeLogTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("FlowRuntimeLogTitle", "Runtime Log"))
		[
			RuntimeLog.ToSharedRef()
		];
}

#if ENABLE_SEARCH_IN_ASSET_EDITOR
TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Search(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == SearchTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("FlowSearchTitle", "Search"))
		[
			SNew(SBox)
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("FlowSearch")))
			[
				SearchBrowser.ToSharedRef()
			]
		];
}
#endif

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_ValidationLog(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == ValidationLogTab);

	return SNew(SDockTab)
		.Label(LOCTEXT("FlowValidationLogTitle", "Validation Log"))
		[
			ValidationLog.ToSharedRef()
		];
}

void FFlowAssetEditor::InitFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit)
{
	FlowAsset = CastChecked<UFlowAsset>(ObjectToEdit);

	// Support undo/redo
	FlowAsset->SetFlags(RF_Transactional);
	GEditor->RegisterForUndo(this);

	UFlowGraphSchema::SubscribeToAssetChanges();

	BindToolbarCommands();
	CreateToolbar();

	CreateWidgets();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("FlowAssetEditor_Layout_v5.1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
										->Split
										(
											FTabManager::NewStack()
											->SetSizeCoefficient(0.225f)
											->AddTab(DetailsTab, ETabState::OpenedTab)
										)
										->Split
										(
											FTabManager::NewSplitter()
											->SetSizeCoefficient(0.65f)
											->SetOrientation(Orient_Vertical)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.8f)
												->SetHideTabWell(true)
												->AddTab(GraphTab, ETabState::OpenedTab)
											)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.15f)
												->AddTab(RuntimeLogTab, ETabState::ClosedTab)
											)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.15f)
												->AddTab(SearchTab, ETabState::ClosedTab)
											)
											->Split
											(
												FTabManager::NewStack()
												->SetSizeCoefficient(0.15f)
												->AddTab(ValidationLogTab, ETabState::ClosedTab)
											)
										)
										->Split
										(
											FTabManager::NewStack()
											->SetSizeCoefficient(0.125f)
											->AddTab(PaletteTab, ETabState::OpenedTab)
										)
		);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, TEXT("FlowEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit, false);

	RegenerateMenusAndToolbars();
}

void FFlowAssetEditor::CreateToolbar()
{
	FName ParentToolbarName;
	const FName ToolBarName = GetToolMenuToolbarName(ParentToolbarName);

	UToolMenus* ToolMenus = UToolMenus::Get();
	UToolMenu* FoundMenu = ToolMenus->FindMenu(ToolBarName);
	if (!FoundMenu || !FoundMenu->IsRegistered())
	{
		FoundMenu = ToolMenus->RegisterMenu(ToolBarName, ParentToolbarName, EMultiBoxType::ToolBar);
	}

	if (FoundMenu)
	{
		AssetToolbar = MakeShareable(new FFlowAssetToolbar(SharedThis(this), FoundMenu));
	}
}

void FFlowAssetEditor::BindToolbarCommands()
{
	FFlowToolbarCommands::Register();
	const FFlowToolbarCommands& ToolbarCommands = FFlowToolbarCommands::Get();

	// Editing
	ToolkitCommands->MapAction(ToolbarCommands.RefreshAsset,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::RefreshAsset),
								FCanExecuteAction::CreateStatic(&FFlowAssetEditor::CanEdit));

	ToolkitCommands->MapAction(ToolbarCommands.ValidateAsset,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::ValidateAsset_Internal),
								FCanExecuteAction());

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	ToolkitCommands->MapAction(ToolbarCommands.SearchInAsset,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::SearchInAsset),
								FCanExecuteAction());
#endif

	// Engine's Play commands
	ToolkitCommands->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	// Debugging
	ToolkitCommands->MapAction(ToolbarCommands.GoToParentInstance,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::GoToParentInstance),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanGoToParentInstance),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanGoToParentInstance));
}

void FFlowAssetEditor::RefreshAsset()
{
	// attempt to refresh graph, fix common issues automatically
	CastChecked<UFlowGraph>(FlowAsset->GetGraph())->RefreshGraph();
}

void FFlowAssetEditor::ValidateAsset_Internal()
{
	FFlowMessageLog LogResults;
	ValidateAsset(LogResults);

	// push messages to its window
	ValidationLogListing->ClearMessages();
	if (LogResults.Messages.Num() > 0)
	{
		TabManager->TryInvokeTab(ValidationLogTab);
		ValidationLogListing->AddMessages(LogResults.Messages);
	}
	ValidationLogListing->OnDataChanged().Broadcast();
}

void FFlowAssetEditor::ValidateAsset(FFlowMessageLog& MessageLog)
{
	FlowAsset->ValidateAsset(MessageLog);
}

#if ENABLE_SEARCH_IN_ASSET_EDITOR
void FFlowAssetEditor::SearchInAsset()
{
	TabManager->TryInvokeTab(SearchTab);
	SearchBrowser->FocusForUse();
}
#endif

void FFlowAssetEditor::GoToParentInstance()
{
	const UFlowAsset* AssetThatInstancedThisAsset = FlowAsset->GetInspectedInstance()->GetParentInstance();

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetThatInstancedThisAsset->GetTemplateAsset());
	AssetThatInstancedThisAsset->GetTemplateAsset()->SetInspectedInstance(AssetThatInstancedThisAsset->GetDisplayName());
}

bool FFlowAssetEditor::CanGoToParentInstance()
{
	return FlowAsset->GetInspectedInstance() && FlowAsset->GetInspectedInstance()->GetNodeOwningThisAssetInstance() != nullptr;
}

void FFlowAssetEditor::CreateWidgets()
{
	// Details View
	{
		FDetailsViewArgs Args;
		Args.bHideSelectionTip = true;
		Args.bShowPropertyMatrixButton = false;
		Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
		Args.NotifyHook = this;

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		DetailsView = PropertyModule.CreateDetailView(Args);
		DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic(&FFlowAssetEditor::CanEdit));
		DetailsView->SetObject(FlowAsset);
	}

	// Graph
	CreateGraphWidget();
	GraphEditor->OnSelectionChangedEvent.BindRaw(this, &FFlowAssetEditor::OnSelectedNodesChanged);

	// Palette
	Palette = SNew(SFlowPalette, SharedThis(this));

	// Search
#if ENABLE_SEARCH_IN_ASSET_EDITOR
	SearchBrowser = SNew(SSearchBrowser, GetFlowAsset());
#endif

	// Logs
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	{
		RuntimeLogListing = FFlowMessageLogListing::GetLogListing(FlowAsset, EFlowLogType::Runtime);
		RuntimeLogListing->OnMessageTokenClicked().AddSP(this, &FFlowAssetEditor::OnLogTokenClicked);
		RuntimeLog = MessageLogModule.CreateLogListingWidget(RuntimeLogListing.ToSharedRef());
	}
	{
		ValidationLogListing = FFlowMessageLogListing::GetLogListing(FlowAsset, EFlowLogType::Validation);
		ValidationLogListing->OnMessageTokenClicked().AddSP(this, &FFlowAssetEditor::OnLogTokenClicked);
		ValidationLog = MessageLogModule.CreateLogListingWidget(ValidationLogListing.ToSharedRef());
	}
}

void FFlowAssetEditor::CreateGraphWidget()
{
	 SAssignNew(GraphEditor, SFlowGraphEditor, SharedThis(this))
		.DetailsView(DetailsView);
}

bool FFlowAssetEditor::CanEdit()
{
	return GEditor->PlayWorld == nullptr;
}

void FFlowAssetEditor::SetUISelectionState(const FName SelectionOwner)
{
	if (SelectionOwner != CurrentUISelection)
	{
		ClearSelectionStateFor(CurrentUISelection);
		CurrentUISelection = SelectionOwner;
	}
}

void FFlowAssetEditor::ClearSelectionStateFor(const FName SelectionOwner)
{
	if (SelectionOwner == GraphTab)
	{
		GraphEditor->ClearSelectionSet();
	}
	else if (SelectionOwner == PaletteTab)
	{
		if (Palette.IsValid())
		{
			Palette->ClearGraphActionMenuSelection();
		}
	}
}

FName FFlowAssetEditor::GetUISelectionState() const
{
	return CurrentUISelection;
}

#if ENABLE_JUMP_TO_INNER_OBJECT
void FFlowAssetEditor::JumpToInnerObject(UObject* InnerObject)
{
	if (const UFlowNode* FlowNode = Cast<UFlowNode>(InnerObject))
	{
		GraphEditor->JumpToNode(FlowNode->GetGraphNode(), true);
	}
}
#endif

void FFlowAssetEditor::OnLogTokenClicked(const TSharedRef<IMessageToken>& Token) const
{
	if (Token->GetType() == EMessageToken::Object)
	{
		const TSharedRef<FUObjectToken> ObjectToken = StaticCastSharedRef<FUObjectToken>(Token);
		if (const UObject* Object = ObjectToken->GetObject().Get())
		{
			if (Object->IsAsset())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UObject*>(Object));
			}
			else
			{
				UE_LOG(LogFlowEditor, Warning, TEXT("Unknown type of hyperlinked object (%s), cannot focus it"), *GetNameSafe(Object));
			}
		}
	}
	else if (Token->GetType() == EMessageToken::EdGraph && GraphEditor.IsValid())
	{
		const TSharedRef<FFlowGraphToken> EdGraphToken = StaticCastSharedRef<FFlowGraphToken>(Token);

		if (const UEdGraphPin* GraphPin = EdGraphToken->GetPin())
		{
			if (!GraphPin->IsPendingKill())
			{
				GraphEditor->JumpToPin(GraphPin);
			}
		}
		else if (const UEdGraphNode* GraphNode = EdGraphToken->GetGraphNode())
		{
			GraphEditor->JumpToNode(GraphNode, true);
		}
	}
}

#undef LOCTEXT_NAMESPACE
