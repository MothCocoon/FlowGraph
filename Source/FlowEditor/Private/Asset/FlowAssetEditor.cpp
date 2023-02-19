// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetEditor.h"

#include "FlowEditorCommands.h"
#include "FlowEditorModule.h"
#include "FlowMessageLog.h"

#include "Asset/FlowAssetEditorContext.h"
#include "Asset/FlowAssetToolbar.h"
#include "Asset/FlowDebugger.h"
#include "Asset/FlowMessageLogListing.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Widgets/SFlowPalette.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "EdGraphUtilities.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "Framework/Commands/GenericCommands.h"
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
#include "ScopedTransaction.h"
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
	FlowDebugger = MakeShareable(new FFlowDebugger);

	BindToolbarCommands();
	CreateToolbar();

	BindGraphCommands();
	CreateWidgets();

	FlowAsset->OnRuntimeMessageAdded().AddSP(this, &FFlowAssetEditor::OnRuntimeMessageAdded);
	FEditorDelegates::BeginPIE.AddSP(this, &FFlowAssetEditor::OnBeginPIE);

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
	GraphEditor = CreateGraphWidget();

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

	Palette = SNew(SFlowPalette, SharedThis(this));

#if ENABLE_SEARCH_IN_ASSET_EDITOR
	SearchBrowser = SNew(SSearchBrowser, GetFlowAsset());
#endif

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

TSharedRef<SGraphEditor> FFlowAssetEditor::CreateGraphWidget()
{
	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FFlowAssetEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FFlowAssetEditor::OnNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FFlowAssetEditor::OnNodeTitleCommitted);
	InEvents.OnSpawnNodeByShortcut = SGraphEditor::FOnSpawnNodeByShortcut::CreateStatic(&FFlowAssetEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(FlowAsset->GetGraph()));

	return SNew(SGraphEditor)
		.AdditionalCommands(ToolkitCommands)
		.IsEditable(true)
		.Appearance(GetGraphAppearanceInfo())
		.GraphToEdit(FlowAsset->GetGraph())
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}

FGraphAppearanceInfo FFlowAssetEditor::GetGraphAppearanceInfo() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = GetCornerText();

	if (FlowDebugger.IsValid() && FFlowDebugger::IsPlaySessionPaused())
	{
		AppearanceInfo.PIENotifyText = LOCTEXT("PausedLabel", "PAUSED");
	}

	return AppearanceInfo;
}

FText FFlowAssetEditor::GetCornerText() const
{
	return LOCTEXT("AppearanceCornerText_FlowAsset", "FLOW");
}

void FFlowAssetEditor::BindGraphCommands()
{
	FGraphEditorCommands::Register();
	FFlowGraphCommands::Register();
	FFlowSpawnNodeCommands::Register();

	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphCommands = FGraphEditorCommands::Get();
	const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();

	// Graph commands
	ToolkitCommands->MapAction(GraphCommands.CreateComment,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnCreateComment),
								FCanExecuteAction::CreateStatic(&FFlowAssetEditor::CanEdit));

	ToolkitCommands->MapAction(GraphCommands.StraightenConnections,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnStraightenConnections));

	// Generic Node commands
	ToolkitCommands->MapAction(GenericCommands.Undo,
								FExecuteAction::CreateStatic(&FFlowAssetEditor::UndoGraphAction),
								FCanExecuteAction::CreateStatic(&FFlowAssetEditor::CanEdit));

	ToolkitCommands->MapAction(GenericCommands.Redo,
								FExecuteAction::CreateStatic(&FFlowAssetEditor::RedoGraphAction),
								FCanExecuteAction::CreateStatic(&FFlowAssetEditor::CanEdit));

	ToolkitCommands->MapAction(GenericCommands.SelectAll,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::SelectAllNodes),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanSelectAllNodes));

	ToolkitCommands->MapAction(GenericCommands.Delete,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::DeleteSelectedNodes),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDeleteNodes));

	ToolkitCommands->MapAction(GenericCommands.Copy,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::CopySelectedNodes),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanCopyNodes));

	ToolkitCommands->MapAction(GenericCommands.Cut,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::CutSelectedNodes),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanCutNodes));

	ToolkitCommands->MapAction(GenericCommands.Paste,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::PasteNodes),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanPasteNodes));

	ToolkitCommands->MapAction(GenericCommands.Duplicate,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::DuplicateNodes),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDuplicateNodes));

	// Pin commands
	ToolkitCommands->MapAction(FlowGraphCommands.RefreshContextPins,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::RefreshContextPins),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRefreshContextPins));

	ToolkitCommands->MapAction(FlowGraphCommands.AddInput,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::AddInput),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddInput));

	ToolkitCommands->MapAction(FlowGraphCommands.AddOutput,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::AddOutput),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddOutput));

	ToolkitCommands->MapAction(FlowGraphCommands.RemovePin,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::RemovePin),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRemovePin));

	// Breakpoint commands
	ToolkitCommands->MapAction(GraphCommands.AddBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnAddBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanAddBreakpoint)
	);

	ToolkitCommands->MapAction(GraphCommands.RemoveBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnRemoveBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRemoveBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanRemoveBreakpoint)
	);

	ToolkitCommands->MapAction(GraphCommands.EnableBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnEnableBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEnableBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanEnableBreakpoint)
	);

	ToolkitCommands->MapAction(GraphCommands.DisableBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnDisableBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDisableBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanDisableBreakpoint)
	);

	ToolkitCommands->MapAction(GraphCommands.ToggleBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnToggleBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanToggleBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanToggleBreakpoint)
	);

	// Pin Breakpoint commands
	ToolkitCommands->MapAction(FlowGraphCommands.AddPinBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnAddPinBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddPinBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanAddPinBreakpoint)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.RemovePinBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnRemovePinBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRemovePinBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanRemovePinBreakpoint)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.EnablePinBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnEnablePinBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEnablePinBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanEnablePinBreakpoint)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.DisablePinBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnDisablePinBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDisablePinBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanDisablePinBreakpoint)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.TogglePinBreakpoint,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnTogglePinBreakpoint),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanTogglePinBreakpoint),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanTogglePinBreakpoint)
	);

	// Execution Override commands
	ToolkitCommands->MapAction(FlowGraphCommands.EnableNode,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::SetSignalMode, EFlowSignalMode::Enabled),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanSetSignalMode, EFlowSignalMode::Enabled),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanSetSignalMode, EFlowSignalMode::Enabled)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.DisableNode,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::SetSignalMode, EFlowSignalMode::Disabled),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanSetSignalMode, EFlowSignalMode::Disabled),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanSetSignalMode, EFlowSignalMode::Disabled)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.SetPassThrough,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::SetSignalMode, EFlowSignalMode::PassThrough),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanSetSignalMode, EFlowSignalMode::PassThrough),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanSetSignalMode, EFlowSignalMode::PassThrough)
	);

	ToolkitCommands->MapAction(FlowGraphCommands.ForcePinActivation,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnForcePinActivation),
								FCanExecuteAction::CreateStatic(&FFlowAssetEditor::IsPIE),
								FIsActionChecked(),
								FIsActionButtonVisible::CreateStatic(&FFlowAssetEditor::IsPIE)
	);

	// Jump commands
	ToolkitCommands->MapAction(FlowGraphCommands.FocusViewport,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::FocusViewport),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanFocusViewport));

	ToolkitCommands->MapAction(FlowGraphCommands.JumpToNodeDefinition,
								FExecuteAction::CreateSP(this, &FFlowAssetEditor::JumpToNodeDefinition),
								FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanJumpToNodeDefinition));
}

void FFlowAssetEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void FFlowAssetEditor::RedoGraphAction()
{
	GEditor->RedoTransaction();
}

FReply FFlowAssetEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph)
{
	UEdGraph* Graph = InGraph;

	if (FFlowSpawnNodeCommands::IsRegistered())
	{
		const TSharedPtr<FEdGraphSchemaAction> Action = FFlowSpawnNodeCommands::Get().GetActionByChord(InChord);
		if (Action.IsValid())
		{
			TArray<UEdGraphPin*> DummyPins;
			Action->PerformAction(Graph, DummyPins, InPosition);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
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

void FFlowAssetEditor::OnCreateComment() const
{
	FFlowGraphSchemaAction_NewComment CommentAction;
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, GraphEditor->GetPasteLocation());
}

void FFlowAssetEditor::OnStraightenConnections() const
{
	GraphEditor->OnStraightenConnections();
}

bool FFlowAssetEditor::CanEdit()
{
	return GEditor->PlayWorld == nullptr;
}

bool FFlowAssetEditor::IsPIE()
{
	return GEditor->PlayWorld != nullptr;
}

EVisibility FFlowAssetEditor::GetDebuggerVisibility()
{
	return GEditor->PlayWorld ? EVisibility::Visible : EVisibility::Collapsed;
}

void FFlowAssetEditor::OnBeginPIE(const bool bInSimulateInEditor) const
{
	RuntimeLogListing->ClearMessages();
}

TSet<UFlowGraphNode*> FFlowAssetEditor::GetSelectedFlowNodes() const
{
	TSet<UFlowGraphNode*> Result;

	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			Result.Emplace(SelectedNode);
		}
	}

	return Result;
}

int32 FFlowAssetEditor::GetNumberOfSelectedNodes() const
{
	return GraphEditor->GetSelectedNodes().Num();
}

bool FFlowAssetEditor::GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const
{
	return GraphEditor->GetBoundsForSelectedNodes(Rect, Padding);
}

void FFlowAssetEditor::OnSelectedNodesChanged(const TSet<UObject*>& Nodes)
{
	TArray<UObject*> SelectedObjects;

	if (Nodes.Num() > 0)
	{
		SetUISelectionState(GraphTab);

		for (TSet<UObject*>::TConstIterator SetIt(Nodes); SetIt; ++SetIt)
		{
			if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(*SetIt))
			{
				SelectedObjects.Add(Cast<UObject>(GraphNode->GetFlowNode()));
			}
			else
			{
				SelectedObjects.Add(*SetIt);
			}
		}
	}
	else
	{
		SetUISelectionState(NAME_None);
		SelectedObjects.Add(GetFlowAsset());
	}

	if (DetailsView.IsValid())
	{
		DetailsView->SetObjects(SelectedObjects);
	}
}

void FFlowAssetEditor::SelectSingleNode(UEdGraphNode* Node) const
{
	GraphEditor->ClearSelectionSet();
	GraphEditor->SetNodeSelection(Node, true);
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

void FFlowAssetEditor::OnRuntimeMessageAdded(const UFlowAsset* AssetInstance, const TSharedRef<FTokenizedMessage>& Message) const
{
	// push messages to its window
	TabManager->TryInvokeTab(RuntimeLogTab);
	RuntimeLogListing->AddMessage(Message);
	RuntimeLogListing->OnDataChanged().Broadcast();
}

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

void FFlowAssetEditor::SelectAllNodes() const
{
	GraphEditor->SelectAllNodes();
}

bool FFlowAssetEditor::CanSelectAllNodes() const
{
	return true;
}

void FFlowAssetEditor::DeleteSelectedNodes()
{
	const FScopedTransaction Transaction(LOCTEXT("DeleteSelectedNode", "Delete Selected Node"));
	GraphEditor->GetCurrentGraph()->Modify();
	FlowAsset->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
	SetUISelectionState(NAME_None);

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);
		if (Node->CanUserDeleteNode())
		{
			if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
			{
				if (FlowGraphNode->GetFlowNode())
				{
					const FGuid NodeGuid = FlowGraphNode->GetFlowNode()->GetGuid();

					GraphEditor->GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
					Node->DestroyNode();

					FlowAsset->UnregisterNode(NodeGuid);
					continue;
				}
			}

			GraphEditor->GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
			Node->DestroyNode();
		}
	}
}

void FFlowAssetEditor::DeleteSelectedDuplicableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GraphEditor->GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	GraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(OldSelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt))
		{
			if (Node->CanDuplicateNode())
			{
				GraphEditor->SetNodeSelection(Node, true);
			}
			else
			{
				RemainingNodes.Add(Node);
			}
		}
	}

	// Delete the duplicable nodes
	DeleteSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(RemainingNodes); SelectedIt; ++SelectedIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt))
		{
			GraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FFlowAssetEditor::CanDeleteNodes() const
{
	if (CanEdit())
	{
		const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (const UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
			{
				if (!Node->CanUserDeleteNode())
				{
					return false;
				}
			}
		}

		return SelectedNodes.Num() > 0;
	}

	return false;
}

void FFlowAssetEditor::CutSelectedNodes()
{
	CopySelectedNodes();

	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicableNodes();
}

bool FFlowAssetEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FFlowAssetEditor::CopySelectedNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UFlowGraphNode* Node = Cast<UFlowGraphNode>(*SelectedIt))
		{
			Node->PrepareForCopying();
		}
	}

	// Export the selected nodes and place the text on the clipboard
	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UFlowGraphNode* Node = Cast<UFlowGraphNode>(*SelectedIt))
		{
			Node->PostCopyNode();
		}
	}
}

bool FFlowAssetEditor::CanCopyNodes() const
{
	if (CanEdit())
	{
		const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
		{
			const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt);
			if (Node && Node->CanDuplicateNode())
			{
				return true;
			}
		}
	}

	return false;
}

void FFlowAssetEditor::PasteNodes()
{
	PasteNodesHere(GraphEditor->GetPasteLocation());
}

void FFlowAssetEditor::PasteNodesHere(const FVector2D& Location)
{
	SetUISelectionState(NAME_None);

	// Undo/Redo support
	const FScopedTransaction Transaction(LOCTEXT("PasteNode", "Paste Node"));
	FlowAsset->GetGraph()->Modify();
	FlowAsset->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	GraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(FlowAsset->GetGraph(), TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		const UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if (PastedNodes.Num() > 0)
	{
		const float InvNumNodes = 1.0f / static_cast<float>(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();

		if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
		{
			FlowAsset->RegisterNode(Node->NodeGuid, FlowGraphNode->GetFlowNode());
		}

		// Select the newly pasted stuff
		GraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());
	}

	// Force new pasted FlowNodes to have same connections as graph nodes
	FlowAsset->HarvestNodeConnections();

	// Update UI
	GraphEditor->NotifyGraphChanged();

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();
}

bool FFlowAssetEditor::CanPasteNodes() const
{
	if (CanEdit())
	{
		FString ClipboardContent;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

		return FEdGraphUtilities::CanImportNodesFromText(FlowAsset->GetGraph(), ClipboardContent);
	}

	return false;
}

void FFlowAssetEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FFlowAssetEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FFlowAssetEditor::OnNodeDoubleClicked(class UEdGraphNode* Node) const
{
	UFlowNode* FlowNode = Cast<UFlowGraphNode>(Node)->GetFlowNode();

	if (FlowNode)
	{
		if (UFlowGraphEditorSettings::Get()->NodeDoubleClickTarget == EFlowNodeDoubleClickTarget::NodeDefinition)
		{
			Node->JumpToDefinition();
		}
		else
		{
			const FString AssetPath = FlowNode->GetAssetPath();
			if (!AssetPath.IsEmpty())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetPath);
			}
			else if (UObject* AssetToEdit = FlowNode->GetAssetToEdit())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetToEdit);

				if (IsPIE())
				{
					if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(FlowNode))
					{
						const TWeakObjectPtr<UFlowAsset> SubFlowInstance = SubGraphNode->GetFlowAsset()->GetFlowInstance(SubGraphNode);
						if (SubFlowInstance.IsValid())
						{
							SubGraphNode->GetFlowAsset()->GetTemplateAsset()->SetInspectedInstance(SubFlowInstance->GetDisplayName());
						}
					}
				}
			}
		}
	}
}

void FFlowAssetEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Node"));
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void FFlowAssetEditor::RefreshContextPins() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->RefreshContextPins(true);
	}
}

bool FFlowAssetEditor::CanRefreshContextPins() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			return SelectedNode->SupportsContextPins();
		}
	}

	return false;
}

void FFlowAssetEditor::AddInput() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->AddUserInput();
	}
}

bool FFlowAssetEditor::CanAddInput() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			return SelectedNode->CanUserAddInput();
		}
	}

	return false;
}

void FFlowAssetEditor::AddOutput() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->AddUserOutput();
	}
}

bool FFlowAssetEditor::CanAddOutput() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			return SelectedNode->CanUserAddOutput();
		}
	}

	return false;
}

void FFlowAssetEditor::RemovePin() const
{
	if (UEdGraphPin* SelectedPin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(SelectedPin->GetOwningNode()))
		{
			SelectedNode->RemoveInstancePin(SelectedPin);
		}
	}
}

bool FFlowAssetEditor::CanRemovePin() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		if (const UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
		{
			if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
			{
				if (Pin->Direction == EGPD_Input)
				{
					return GraphNode->CanUserRemoveInput(Pin);
				}
				else
				{
					return GraphNode->CanUserRemoveOutput(Pin);
				}
			}
		}
	}

	return false;
}

void FFlowAssetEditor::OnAddBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.AddBreakpoint();
	}
}

void FFlowAssetEditor::OnAddPinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Add(Pin, FFlowBreakpoint());
			GraphNode->PinBreakpoints[Pin].AddBreakpoint();
		}
	}
}

bool FFlowAssetEditor::CanAddBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return !SelectedNode->NodeBreakpoint.HasBreakpoint();
	}

	return false;
}

bool FFlowAssetEditor::CanAddPinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return !GraphNode->PinBreakpoints.Contains(Pin) || !GraphNode->PinBreakpoints[Pin].HasBreakpoint();
		}
	}

	return false;
}

void FFlowAssetEditor::OnRemoveBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.RemoveBreakpoint();
	}
}

void FFlowAssetEditor::OnRemovePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Remove(Pin);
		}
	}
}

bool FFlowAssetEditor::CanRemoveBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->NodeBreakpoint.HasBreakpoint();
	}

	return false;
}

bool FFlowAssetEditor::CanRemovePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin);
		}
	}

	return false;
}

void FFlowAssetEditor::OnEnableBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.EnableBreakpoint();
	}
}

void FFlowAssetEditor::OnEnablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints[Pin].EnableBreakpoint();
		}
	}
}

bool FFlowAssetEditor::CanEnableBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin);
		}
	}

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->NodeBreakpoint.CanEnableBreakpoint();
	}

	return false;
}

bool FFlowAssetEditor::CanEnablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin) && GraphNode->PinBreakpoints[Pin].CanEnableBreakpoint();
		}
	}

	return false;
}

void FFlowAssetEditor::OnDisableBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.DisableBreakpoint();
	}
}

void FFlowAssetEditor::OnDisablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints[Pin].DisableBreakpoint();
		}
	}
}

bool FFlowAssetEditor::CanDisableBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->NodeBreakpoint.IsBreakpointEnabled();
	}

	return false;
}

bool FFlowAssetEditor::CanDisablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin) && GraphNode->PinBreakpoints[Pin].IsBreakpointEnabled();
		}
	}

	return false;
}

void FFlowAssetEditor::OnToggleBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.ToggleBreakpoint();
	}
}

void FFlowAssetEditor::OnTogglePinBreakpoint() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Add(Pin, FFlowBreakpoint());
			GraphNode->PinBreakpoints[Pin].ToggleBreakpoint();
		}
	}
}

bool FFlowAssetEditor::CanToggleBreakpoint() const
{
	return GetSelectedFlowNodes().Num() > 0;
}

bool FFlowAssetEditor::CanTogglePinBreakpoint() const
{
	return GraphEditor->GetGraphPinForMenu() != nullptr;
}

void FFlowAssetEditor::SetSignalMode(const EFlowSignalMode Mode) const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->SetSignalMode(Mode);
	}

	FlowAsset->Modify();
}

bool FFlowAssetEditor::CanSetSignalMode(const EFlowSignalMode Mode) const
{
	if (IsPIE())
	{
		return false;
	}

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->CanSetSignalMode(Mode);
	}

	return false;
}

void FFlowAssetEditor::OnForcePinActivation() const
{
	if (UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu())
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->ForcePinActivation(Pin);
		}
	}
}

void FFlowAssetEditor::FocusViewport() const
{
	// Iterator used but should only contain one node
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		const UFlowNode* FlowNode = Cast<UFlowGraphNode>(SelectedNode)->GetFlowNode();
		if (UFlowNode* NodeInstance = FlowNode->GetInspectedInstance())
		{
			if (AActor* ActorToFocus = NodeInstance->GetActorToFocus())
			{
				GEditor->SelectNone(false, false, false);
				GEditor->SelectActor(ActorToFocus, true, true, true);
				GEditor->NoteSelectionChange();

				GEditor->MoveViewportCamerasToActor(*ActorToFocus, false);

				const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
				const TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
				if (LevelEditorTab.IsValid())
				{
					LevelEditorTab->DrawAttention();
				}
			}
		}

		return;
	}
}

bool FFlowAssetEditor::CanFocusViewport() const
{
	return GetSelectedFlowNodes().Num() == 1;
}

void FFlowAssetEditor::JumpToNodeDefinition() const
{
	// Iterator used but should only contain one node
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->JumpToDefinition();
		return;
	}
}

bool FFlowAssetEditor::CanJumpToNodeDefinition() const
{
	return GetSelectedFlowNodes().Num() == 1;
}

#undef LOCTEXT_NAMESPACE
