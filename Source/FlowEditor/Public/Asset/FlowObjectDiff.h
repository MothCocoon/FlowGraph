// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 7
#include "Editor/Kismet/Private/DiffControl.h"
#else
#include "Editor/Kismet/Internal/DiffControl.h"
#endif

class FBlueprintDifferenceTreeEntry;
class FDetailsDiff;
class FFlowObjectDiff;
class UEdGraphNode;
struct FDiffResultItem;
struct FFlowGraphToDiff;

enum class ENodeDiffType
{
	Old,
	New,
	Invalid
};

/////////////////////////////////////////////////////////////////////////////
/// FFlowObjectDiffArgs: Used for FOnDiffEntryFocused arguments.
struct FLOWEDITOR_API FFlowObjectDiffArgs
{
	FFlowObjectDiffArgs(TWeakPtr<FFlowObjectDiff> InFlowNodeDiff, const FSingleObjectDiffEntry& InPropertyDiff);

	TWeakPtr<FFlowObjectDiff> FlowNodeDiff;
	FSingleObjectDiffEntry PropertyDiff;
};

/////////////////////////////////////////////////////////////////////////////
/// FFlowObjectDiff: represents diff data for a particular node or pin.
class FLOWEDITOR_API FFlowObjectDiff : public TSharedFromThis<FFlowObjectDiff>
{
public:
	FFlowObjectDiff(TSharedPtr<FDiffResultItem> InDiffResult, const FFlowGraphToDiff& GraphToDiff);

	void OnSelectDiff(const FSingleObjectDiffEntry& Property) const;

	void DiffProperties(TArray<FSingleObjectDiffEntry>& OutPropertyDiffsArray) const;

private:
	void InitializeDetailsDiffFromNode(UEdGraphNode* Node, const UObject* Object, const FFlowGraphToDiff& GraphToDiff);

public:
	//the tree entry for this diff object, which can be the parent tree node to other changes such as property changes,
	//added/removed add-ons, moves or comments.
	TSharedPtr<FBlueprintDifferenceTreeEntry> DiffTreeEntry;
	TSharedPtr<FDiffResultItem> DiffResult;

	//parent of this diff. Certain nodes like Add-Ons are displayed inside the DiffTreeEntry for the node they're attached to.
	TWeakPtr<FFlowObjectDiff> ParentNodeDiff;

	TSharedPtr<FDetailsDiff> OldDetailsView;
	TSharedPtr<FDetailsDiff> NewDetailsView;

	//arguments used for FOnDiffEntryFocused.
	TSharedPtr<FFlowObjectDiffArgs> DiffEntryFocusArg;
	TArray<TSharedPtr<FFlowObjectDiffArgs>> PropertyDiffArgList;

	//a list for deferring creation of DiffTreeEntries that are lower priority.
	TArray<TSharedPtr<FDiffResultItem>> LowPriorityChildDiffResult;
};
