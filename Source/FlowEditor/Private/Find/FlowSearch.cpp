// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Find/FlowSearch.h"

#include "AddOns/FlowNodeAddOn.h"
#include "FlowAsset.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "UObject/TopLevelAssetPath.h"

//////////////////////////////////////////////////////////////////////////
// FFindInFlowCache

TMap<TWeakObjectPtr<UEdGraphNode>, FFindInFlowCacheEntry> FFindInFlowCache::CategoryStringCache;

void FFindInFlowCache::OnFlowAssetChanged(const UFlowAsset& ChangedFlowAsset)
{
	TArray<TWeakObjectPtr<UEdGraphNode>> EntriesToRemove;

	for (const auto& Pair : CategoryStringCache)
	{
		const TWeakObjectPtr<UEdGraphNode>& EdNodePtr = Pair.Key;

		UEdGraphNode* EdNode = EdNodePtr.Get();

		if (!IsValid(EdNode))
		{
			EntriesToRemove.Add(EdNodePtr);
			continue;
		}

		const UEdGraph* EdGraph = ChangedFlowAsset.GetGraph();
		if (EdGraph && EdGraph->Nodes.Contains(EdNode))
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
// FFlowSearch

bool FFlowSearch::Search(const FFlowSearchQuery& Query, TArray<FFlowSearchResultItem>& OutResults)
{
	if (Query.SearchText.IsEmpty())
	{
		return false;
	}

	FSearchContext Ctx;
	Ctx.Flags     = Query.Flags;
	Ctx.MaxDepth  = FMath::Max(Query.MaxDepth, 1);
	Ctx.PinFilter = Query.PinFilter;

	// Build upper-cased token list (AND semantics, same as SFindInFlow).
	Query.SearchText.ParseIntoArray(Ctx.Tokens, TEXT(" "), true);
	for (FString& Token : Ctx.Tokens)
	{
		Token = Token.ToUpper();
	}

	if (Ctx.Tokens.IsEmpty())
	{
		return false;
	}

	const int32 ResultsBefore = OutResults.Num();

	// Optional hook for callers to report progress on the search, since it can take a while for large projects.
	int32 TotalAssets = 0;
	int32 ProcessedAssets = 0;
	const auto ReportProgress = [&Query, &ProcessedAssets, &TotalAssets](const UFlowAsset* Asset)
	{
		++ProcessedAssets;
		if (Query.OnAssetSearchProgress)
		{
			Query.OnAssetSearchProgress(ProcessedAssets, TotalAssets, Asset ? Asset->GetName() : FString());
		}
	};

	switch (Query.Scope)
	{
	case EFlowSearchScope::ThisAssetOnly:
		{
			UFlowAsset* Asset = Query.ContextAsset.Get();
			if (Asset && Asset->GetGraph())
			{
				TotalAssets = 1;
				ProcessAsset(Asset, Ctx, false, FSoftObjectPath(), OutResults);
				ReportProgress(Asset);
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

			const UFlowAsset* ContextAsset = Query.ContextAsset.Get();
			if (Query.Scope == EFlowSearchScope::AllFlowAssets || !ContextAsset)
			{
				Filter.ClassPaths.Add(FTopLevelAssetPath(UFlowAsset::StaticClass()->GetClassPathName()));
			}
			else
			{
				Filter.ClassPaths.Add(FTopLevelAssetPath(ContextAsset->GetClass()->GetClassPathName()));
			}

			Registry.Get().GetAssets(Filter, Assets);
			TotalAssets = Assets.Num();

			for (const FAssetData& Data : Assets)
			{
				UFlowAsset* Asset = Cast<UFlowAsset>(Data.GetAsset());
				if (IsValid(Asset) && Asset->GetGraph())
				{
					ProcessAsset(Asset, Ctx, false, FSoftObjectPath(), OutResults);
				}
				ReportProgress(Asset);
			}
		}
		break;

	default:
		checkNoEntry();
		break;
	}

	return OutResults.Num() > ResultsBefore;
}

bool FFlowSearch::ProcessAsset(
	UFlowAsset* Asset,
	FSearchContext& Context,
	bool bIsSubGraphNode,
	const FSoftObjectPath& SubgraphOwnerPath,
	TArray<FFlowSearchResultItem>& OutResults)
{
	if (!Asset || !Asset->GetGraph() || Context.VisitedAssets.Contains(Asset))
	{
		return false;
	}

	Context.VisitedAssets.Add(Asset);

	const FSoftObjectPath AssetPath(Asset);
	bool bAnyMatches = false;

	for (UEdGraphNode* EdNode : Asset->GetGraph()->Nodes)
	{
		if (!IsValid(EdNode))
		{
			continue;
		}

		TArray<FFlowSearchMatchedPin> MatchedPins;
		const TMap<EFlowSearchFlags, TSet<FString>>* CategoryStrings = BuildCategoryStrings(EdNode, Context);
		EFlowSearchFlags NodeMatchedFlags = EFlowSearchFlags::None;
		if (CategoryStrings)
		{
			for (const TPair<EFlowSearchFlags, TSet<FString>>& Pair : *CategoryStrings)
			{
				if (EnumHasAnyFlags(Context.Flags, Pair.Key) && StringSetMatchesTokens(Context.Tokens, Pair.Value))
				{
					EnumAddFlags(NodeMatchedFlags, Pair.Key);
				}
			}
		}

		if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::PinNames)
			&& FindMatchingPins(*EdNode, Context.Tokens, Context.PinFilter, MatchedPins))
		{
			EnumAddFlags(NodeMatchedFlags, EFlowSearchFlags::PinNames);
		}

		if (NodeMatchedFlags != EFlowSearchFlags::None)
		{
			FFlowSearchResultItem Item;
			Item.AssetPath    = AssetPath;
			Item.MatchedFlags = NodeMatchedFlags;
			Item.MatchedPins  = MoveTemp(MatchedPins);
			Item.bIsSubGraphNode     = bIsSubGraphNode;
			Item.SubgraphOwnerAssetPath = SubgraphOwnerPath;
			Item.SubgraphPath = Context.SubgraphPath;
			Item.MatchedSnippet = BuildMatchedSnippet(*CategoryStrings, NodeMatchedFlags, Context.Tokens);

			Item.NodeGuid = EdNode->NodeGuid;

			Item.NodeTitle = EdNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
			if (Item.NodeTitle.IsEmpty())
			{
				Item.NodeTitle = EdNode->GetClass()->GetName();
			}

			if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode))
			{
				if (const UFlowNodeBase* Base = FlowGraphNode->GetFlowNodeBase())
				{
					Item.NodeTypeName = Base->GetClass()->GetName();
				}
			}
			if (Item.NodeTypeName.IsEmpty())
			{
				Item.NodeTypeName = EdNode->GetClass()->GetName();
			}

			OutResults.Add(Item);
			bAnyMatches = true;
		}

		bAnyMatches |= RecurseIntoSubgraphsIfEnabled(EdNode, AssetPath, Context, OutResults);
	}

	return bAnyMatches;
}

bool FFlowSearch::RecurseIntoSubgraphsIfEnabled(
	UEdGraphNode* EdNode,
	const FSoftObjectPath& OwnerAssetPath,
	FSearchContext& Context,
	TArray<FFlowSearchResultItem>& OutResults)
{
	if (!EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Subgraphs))
	{
		return false;
	}

	const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
	if (!FlowGraphNode || !FlowGraphNode->GetFlowNodeBase())
	{
		return false;
	}

	const UFlowNode_SubGraph* SubGraph = Cast<UFlowNode_SubGraph>(FlowGraphNode->GetFlowNodeBase());
	if (!SubGraph)
	{
		return false;
	}

	UFlowAsset* SubAsset = Cast<UFlowAsset>(const_cast<UFlowNode_SubGraph*>(SubGraph)->GetAssetToEdit());
	if (!SubAsset)
	{
		return false;
	}

	// Subgraphs don't count against depth - same convention as SFindInFlow.
	Context.SubgraphPath.Add({OwnerAssetPath, EdNode->NodeGuid});
	const bool bAnyMatches = ProcessAsset(SubAsset, Context, true, OwnerAssetPath, OutResults);
	Context.SubgraphPath.Pop();

	return bAnyMatches;
}

const TMap<EFlowSearchFlags, TSet<FString>>* FFlowSearch::BuildCategoryStrings(
	UEdGraphNode* EdNode,
	const FSearchContext& Context)
{
	if (!IsValid(EdNode))
	{
		return nullptr;
	}

	// Reuse the same global cache that SFindInFlow uses so the two paths don't double-build.
	if (const FFindInFlowCacheEntry* Cached = FFindInFlowCache::CategoryStringCache.Find(EdNode))
	{
		// Flags and depth decide which strings are collected
		if (Cached->Flags == Context.Flags && Cached->MaxDepth == Context.MaxDepth)
		{
			return &Cached->CategoryStrings;
		}
	}

	FFindInFlowCacheEntry NewEntry;
	NewEntry.Flags = Context.Flags;
	NewEntry.MaxDepth = Context.MaxDepth;
	UpdateCategoryStringsForEdGraphNode(*EdNode, Context, NewEntry.CategoryStrings);

	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode))
	{
		if (const UFlowNodeBase* Base = FlowGraphNode->GetFlowNodeBase())
		{
			UpdateCategoryStringsForFlowNodeBase(*Base, Context, 0, NewEntry.CategoryStrings);
		}
	}

	return &FFindInFlowCache::CategoryStringCache.Add(EdNode, MoveTemp(NewEntry)).CategoryStrings;
}

void FFlowSearch::UpdateCategoryStringsForEdGraphNode(
	const UEdGraphNode& EdGraphNode,
	const FSearchContext& Context,
	TMap<EFlowSearchFlags, TSet<FString>>& OutMap)
{
	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Comments))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Comments).Add(EdGraphNode.NodeComment);
	}
}

void FFlowSearch::UpdateCategoryStringsForFlowNodeBase(
	const UFlowNodeBase& FlowNodeBase,
	const FSearchContext& Context,
	int32 Depth,
	TMap<EFlowSearchFlags, TSet<FString>>& OutMap)
{
	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Titles))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Titles).Add(FlowNodeBase.GetNodeTitle().ToString());
	}

	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Tooltips))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Tooltips).Add(FlowNodeBase.GetNodeToolTip().ToString());
	}

	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Classes))
	{
		TSet<FString>& ClassesSet = OutMap.FindOrAdd(EFlowSearchFlags::Classes);
		ClassesSet.Add(FlowNodeBase.GetClass()->GetDisplayNameText().ToString());
		ClassesSet.Add(FlowNodeBase.GetClass()->GetName());
	}

	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Descriptions))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::Descriptions).Add(FlowNodeBase.GetNodeDescription());
	}

	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::ConfigText))
	{
		OutMap.FindOrAdd(EFlowSearchFlags::ConfigText).Add(FlowNodeBase.GetNodeConfigText().ToString());
	}

	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::PropertiesFlags))
	{
		AppendPropertyValues(&FlowNodeBase, FlowNodeBase.GetClass(), &FlowNodeBase, Context, Depth, OutMap);
	}

	if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::AddOns))
	{
		FlowNodeBase.ForEachAddOnConst([&Context, &OutMap, Depth](const UFlowNodeAddOn& AddOn)
		{
			FFlowSearch::UpdateCategoryStringsForFlowNodeBase(AddOn, Context, Depth, OutMap);
			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
	}
}

void FFlowSearch::AppendPropertyValues(
	const void* Container,
	const UStruct* Struct,
	const UObject* ParentObject,
	const FSearchContext& Context,
	int32 Depth,
	TMap<EFlowSearchFlags, TSet<FString>>& OutMap)
{
	if (!Container || !Struct || !ParentObject || Depth >= Context.MaxDepth)
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

		// Pin names are searched only by the Pin Names category, where pin filters apply
		const FProperty* ElementProp = Prop;
		if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			ElementProp = ArrayProp->Inner;
		}
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(Prop))
		{
			ElementProp = SetProp->ElementProp;
		}
		const FStructProperty* ElementStructProp = CastField<FStructProperty>(ElementProp);
		const bool bIsFlowPinProperty = ElementStructProp && ElementStructProp->Struct->IsChildOf(FFlowPin::StaticStruct());

		if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::PropertyNames))
		{
			TSet<FString>& NamesSet = OutMap.FindOrAdd(EFlowSearchFlags::PropertyNames);
			const FString DisplayName = Prop->GetMetaData(TEXT("DisplayName"));
			if (!DisplayName.IsEmpty())
			{
				NamesSet.Add(DisplayName);
			}
			NamesSet.Add(Prop->GetName());
		}

		if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::PropertyValues) && !bIsFlowPinProperty)
		{
			FString ValueStr;
			UObject* MutableParent = const_cast<UObject*>(ParentObject);
			Prop->ExportText_InContainer(0, ValueStr, Container, nullptr, MutableParent, PPF_None);
			ValueStr = ValueStr.Replace(TEXT("\""), TEXT("")).TrimStartAndEnd();
			OutMap.FindOrAdd(EFlowSearchFlags::PropertyValues).Add(ValueStr);
		}

		if (EnumHasAnyFlags(Context.Flags, EFlowSearchFlags::Tooltips))
		{
			OutMap.FindOrAdd(EFlowSearchFlags::Tooltips).Add(Prop->GetMetaData(TEXT("ToolTip")));
		}

		if (bIsFlowPinProperty)
		{
			continue;
		}

		if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			// Recurse into structs (no depth penalty - same as original).
			AppendPropertyValues(ValuePtr, StructProp->Struct, ParentObject, Context, Depth, OutMap);
		}
		else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		{
			UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
			if (IsValid(Obj) && !Obj->HasAnyFlags(RF_ClassDefaultObject))
			{
				// Inline objects incur a depth penalty (same as original).
				AppendPropertyValues(Obj, Obj->GetClass(), Obj, Context, Depth + 1, OutMap);
			}
		}
	}
}

bool FFlowSearch::FindMatchingPins(
	const UEdGraphNode& EdNode,
	const TArray<FString>& Tokens,
	const FFlowSearchPinFilter& Filter,
	TArray<FFlowSearchMatchedPin>& OutMatchedPins)
{
	for (const UEdGraphPin* Pin : EdNode.Pins)
	{
		if (!Pin || Pin->bOrphanedPin || !StringMatchesTokens(Tokens, Pin->PinName.ToString()))
		{
			continue;
		}

		const bool bIsInput = Pin->Direction == EGPD_Input;
		const bool bIsOutput = Pin->Direction == EGPD_Output;
		const bool bMatchesDirection = Filter.Direction == EFlowSearchPinDirection::Any
			|| (Filter.Direction == EFlowSearchPinDirection::Input && bIsInput)
			|| (Filter.Direction == EFlowSearchPinDirection::Output && bIsOutput);
		if (!bMatchesDirection)
		{
			continue;
		}

		const bool bConnected = !Pin->LinkedTo.IsEmpty();
		const bool bMatchesConnectionState = Filter.ConnectionState == EFlowSearchPinConnectionState::Any
			|| (Filter.ConnectionState == EFlowSearchPinConnectionState::Connected && bConnected)
			|| (Filter.ConnectionState == EFlowSearchPinConnectionState::Unconnected && !bConnected);
		if (!bMatchesConnectionState)
		{
			continue;
		}

		FFlowSearchMatchedPin& MatchedPin = OutMatchedPins.AddDefaulted_GetRef();
		MatchedPin.PinName = Pin->PinName;
		MatchedPin.Direction = bIsInput ? EFlowSearchPinDirection::Input : EFlowSearchPinDirection::Output;
		MatchedPin.bConnected = bConnected;
	}

	return !OutMatchedPins.IsEmpty();
}

bool FFlowSearch::StringMatchesTokens(const TArray<FString>& Tokens, const FString& Str)
{
	const FString Upper = Str.ToUpper();
	for (const FString& Token : Tokens)
	{
		if (!Upper.Contains(Token))
		{
			return false;
		}
	}
	return true;
}

bool FFlowSearch::StringSetMatchesTokens(const TArray<FString>& Tokens, const TSet<FString>& StringSet)
{
	for (const FString& Str : StringSet)
	{
		if (StringMatchesTokens(Tokens, Str))
		{
			return true;
		}
	}
	return false;
}

FString FFlowSearch::BuildMatchedSnippet(
	const TMap<EFlowSearchFlags, TSet<FString>>& CategoryStrings,
	const EFlowSearchFlags MatchedFlags,
	const TArray<FString>& Tokens)
{
	static constexpr EFlowSearchFlags FlagOrder[] = {
		EFlowSearchFlags::Titles,
		EFlowSearchFlags::Classes,
		EFlowSearchFlags::Descriptions,
		EFlowSearchFlags::Comments,
		EFlowSearchFlags::ConfigText,
		EFlowSearchFlags::PropertyValues,
		EFlowSearchFlags::PropertyNames,
		EFlowSearchFlags::Tooltips,
		EFlowSearchFlags::AddOns,
		EFlowSearchFlags::PinNames,
	};

	// Walk the matched categories in priority order; return the first string that hits a token.
	for (const EFlowSearchFlags Flag : FlagOrder)
	{
		if (!EnumHasAnyFlags(MatchedFlags, Flag))
		{
			continue;
		}
		const TSet<FString>* StringSet = CategoryStrings.Find(Flag);
		if (!StringSet)
		{
			continue;
		}
		for (const FString& Str : *StringSet)
		{
			if (StringMatchesTokens(Tokens, Str) && !Str.IsEmpty())
			{
				return Str.Left(120); // Truncate long property values for readability.
			}
		}
	}
	return FString();
}
