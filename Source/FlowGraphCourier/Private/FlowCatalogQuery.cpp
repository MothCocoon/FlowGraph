// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowCatalogQuery.h"
#include "FlowLogChannels.h"
#include "FlowAsset.h"
#include "FlowNodeUsageIndex.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "Algo/Find.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// Gathers all concrete Blueprint-generated classes whose GeneratedClass is a subclass of
// BaseClass, applying the same flag/name filters used for native classes.
static void GatherBlueprintDerivedClasses(UClass* BlueprintBaseClass, UClass* RequiredBaseClass, TArray<UClass*>& OutClasses)
{
	FARFilter Filter;
	Filter.ClassPaths.Add(BlueprintBaseClass->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FoundAssets;
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistryModule.Get().GetAssets(Filter, FoundAssets);

	for (const FAssetData& AssetData : FoundAssets)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			UClass* GeneratedClass = Blueprint->GeneratedClass;
			if (GeneratedClass && GeneratedClass->IsChildOf(RequiredBaseClass))
			{
				if (!GeneratedClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
				{
					OutClasses.Add(GeneratedClass);
				}
			}
		}
	}
}

// Converts a TArray<FString> into a TArray<TSharedPtr<FJsonValue>> for JSON serialization.
static TArray<TSharedPtr<FJsonValue>> StringsToJsonArray(const TArray<FString>& Strings)
{
	TArray<TSharedPtr<FJsonValue>> Out;
	Out.Reserve(Strings.Num());
	for (const FString& S : Strings)
	{
		Out.Add(MakeShared<FJsonValueString>(S));
	}
	return Out;
}

namespace
{
	/**
	 * How many entries each Shape facet list may carry.
	 *
	 * Sized from the real distribution rather than picked round: the project's ~98 categories need only
	 * their top 18 to cover the large groupings an author is orienting against, and the long tail is
	 * mostly categories owned by exactly one class - detail that belongs to a filtered query, not to the
	 * cheapest call in the surface.
	 */
	constexpr int32 	MaxShapeFacets = 18;

	bool MatchesQuery(const FString& ClassName, const FString& Description, const FString& Query)
	{
		if (Query.IsEmpty())
		{
			return true;
		}
		return ClassName.Contains(Query, ESearchCase::IgnoreCase) || Description.Contains(Query, ESearchCase::IgnoreCase);
	}

	/**
	 * Splits a Keywords metadata string into searchable tokens.
	 *
	 * Authors write keywords as free prose - "Spawn & Destroy", "Move|Rotate" - so splitting on spaces
	 * alone leaves punctuation as its own token and "&" and "|" end up presented as keywords. Any
	 * non-alphanumeric character is a separator here, and single characters are dropped: nothing one
	 * character long is a useful search term, and it is always noise from a separator.
	 */
	TArray<FString> SplitAndLowerKeywords(const FString& RawKeywords)
	{
		TArray<FString> Tokens;

		FString Current;
		auto FlushToken = [&Tokens, &Current]()
		{
			if (Current.Len() > 1)
			{
				Tokens.Add(Current.ToLower());
			}
			Current.Reset();
		};

		for (const TCHAR Char : RawKeywords)
		{
			if (FChar::IsAlnum(Char))
			{
				Current.AppendChar(Char);
			}
			else
			{
				FlushToken();
			}
		}
		FlushToken();

		return Tokens;
	}

	bool MatchesAnyKeyword(const FString& RawKeywords, const TArray<FString>& KeywordFilter)
	{
		if (KeywordFilter.IsEmpty())
		{
			return true;
		}
		const TArray<FString> Tokens = SplitAndLowerKeywords(RawKeywords);
		for (const FString& Filter : KeywordFilter)
		{
			if (Tokens.Contains(Filter.ToLower()))
			{
				return true;
			}
		}
		return false;
	}

	bool MatchesAnyCategory(const FString& Category, const TArray<FString>& CategoryFilter)
	{
		if (CategoryFilter.IsEmpty())
		{
			return true;
		}
		for (const FString& Filter : CategoryFilter)
		{
			if (Category.Equals(Filter, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	bool MatchesAnyArticle(const TArray<FString>& ClassArticles, const TArray<FString>& ArticleFilter)
	{
		if (ArticleFilter.IsEmpty())
		{
			return true;
		}
		for (const FString& Filter : ArticleFilter)
		{
			for (const FString& Article : ClassArticles)
			{
				if (Article.Equals(Filter, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}
		return false;
	}

	// Resolves AssetClassName (short name or full path) to its CDO. bOutFound is false only when
	// AssetClassName is non-empty and does not resolve to any known FlowAsset subclass.
	const UFlowAsset* ResolveAssetCDOByName(const FString& AssetClassName, bool& bOutFound)
	{
		bOutFound = true;
		if (AssetClassName.IsEmpty())
		{
			return nullptr;
		}

		TArray<UClass*> AssetClasses;
		UFlowCatalogQuery::GatherAllFlowAssetClasses(AssetClasses);

		for (const UClass* AssetClass : AssetClasses)
		{
			if (AssetClass->GetName() == AssetClassName || AssetClass->GetPathName() == AssetClassName)
			{
				return AssetClass->GetDefaultObject<UFlowAsset>();
			}
		}

		bOutFound = false;
		return nullptr;
	}

	void ApplyPaging(int32 Offset, int32 Limit, TArray<FFlowCatalogClassRow>& Rows, bool& bOutTruncated)
	{
		const int32 TotalNum = Rows.Num();
		if (Offset > 0)
		{
			Rows.RemoveAt(0, FMath::Min(Offset, TotalNum));
		}
		if (Limit > 0 && Rows.Num() > Limit)
		{
			Rows.SetNum(Limit);
			bOutTruncated = true;
		}
	}

	FFlowCatalogFacetRow* FindOrAddFacet(TArray<FFlowCatalogFacetRow>& Facets, const FString& Value)
	{
		for (FFlowCatalogFacetRow& Facet : Facets)
		{
			if (Facet.Value == Value)
			{
				return &Facet;
			}
		}
		FFlowCatalogFacetRow& NewFacet = Facets.AddDefaulted_GetRef();
		NewFacet.Value = Value;
		return &NewFacet;
	}
}

namespace
{
	struct FFlowCatalogSectionName
	{
		const TCHAR* Name;
		EFlowCatalogSection Section;
	};

	const FFlowCatalogSectionName GFlowCatalogSectionNames[] =
	{
		{ TEXT("shape"),       EFlowCatalogSection::Shape },
		{ TEXT("names"),       EFlowCatalogSection::Names },
		{ TEXT("doc"),         EFlowCatalogSection::Doc },
		{ TEXT("properties"),  EFlowCatalogSection::Properties },
		{ TEXT("pins"),        EFlowCatalogSection::Pins },
		{ TEXT("articles"),    EFlowCatalogSection::Articles },
		{ TEXT("usage"),       EFlowCatalogSection::Usage },
		{ TEXT("origin"),      EFlowCatalogSection::Origin },
		{ TEXT("deprecation"), EFlowCatalogSection::Deprecation },
	};

	constexpr EFlowCatalogSection FlowCatalogSectionsFull =
		EFlowCatalogSection::Names |
		EFlowCatalogSection::Doc |
		EFlowCatalogSection::Properties |
		EFlowCatalogSection::Pins |
		EFlowCatalogSection::Articles |
		EFlowCatalogSection::Origin |
		EFlowCatalogSection::Deprecation;
}

bool UFlowCatalogQuery::ParseSections(const FString& SectionsString, EFlowCatalogSection& OutSections, bool& bOutDescriptionOnly, FString& OutError)
{
	OutSections = EFlowCatalogSection::None;
	bOutDescriptionOnly = false;

	if (SectionsString.TrimStartAndEnd().IsEmpty())
	{
		OutSections = EFlowCatalogSection::Shape;
		return true;
	}

	TArray<FString> Tokens;
	SectionsString.ParseIntoArray(Tokens, TEXT(","), /*bCullEmpty=*/true);

	for (FString& Token : Tokens)
	{
		Token = Token.TrimStartAndEnd().ToLower();
		if (Token.IsEmpty())
		{
			continue;
		}

		if (Token == TEXT("full"))
		{
			OutSections |= FlowCatalogSectionsFull;
			continue;
		}
		if (Token == TEXT("brief"))
		{
			OutSections |= EFlowCatalogSection::Names | EFlowCatalogSection::Doc;
			bOutDescriptionOnly = true;
			continue;
		}

		const FFlowCatalogSectionName* Match = Algo::FindByPredicate(GFlowCatalogSectionNames,
			[&Token](const FFlowCatalogSectionName& Candidate) { return Token == Candidate.Name; });

		if (!Match)
		{
			OutError = FString::Printf(
				TEXT("Unknown section \"%s\". Expected any of shape, names, doc, properties, pins, articles, usage, origin, deprecation, or a preset: shape, brief, full."),
				*Token);
			return false;
		}

		OutSections |= Match->Section;
	}

	if (OutSections == EFlowCatalogSection::None)
	{
		OutSections = EFlowCatalogSection::Shape;
		return true;
	}

	// Every row-bearing section needs an identifier to hang off, so Names is implied rather than
	// something the caller has to remember to ask for alongside everything else.
	if (OutSections != EFlowCatalogSection::Shape)
	{
		OutSections |= EFlowCatalogSection::Names;
	}

	// A description-only doc is a request for less, so an explicit doc request alongside "brief" wins.
	if (Tokens.Contains(TEXT("doc")) || Tokens.Contains(TEXT("full")))
	{
		bOutDescriptionOnly = false;
	}

	return true;
}

FString UFlowCatalogQuery::SectionsToString(EFlowCatalogSection Sections)
{
	TArray<FString> Names;
	for (const FFlowCatalogSectionName& Candidate : GFlowCatalogSectionNames)
	{
		if (EnumHasAnyFlags(Sections, Candidate.Section))
		{
			Names.Add(Candidate.Name);
		}
	}
	return FString::Join(Names, TEXT(","));
}

EFlowCatalogKind UFlowCatalogQuery::ParseKind(const FString& KindString)
{
	if (KindString.Equals(TEXT("node"), ESearchCase::IgnoreCase))
	{
		return EFlowCatalogKind::Node;
	}
	if (KindString.Equals(TEXT("addon"), ESearchCase::IgnoreCase))
	{
		return EFlowCatalogKind::Addon;
	}
	return EFlowCatalogKind::Any;
}

FString UFlowCatalogQuery::MakeClassStem(const FString& ClassName)
{
	FString Stem = ClassName;
	Stem.RemoveFromEnd(TEXT("_C"));

	// Strip one leading family prefix - the "MyNode_" in "MyNode_DoThing". Deliberately data-driven
	// rather than a hardcoded prefix list, so a project's own naming families cost nothing to support.
	int32 UnderscoreIndex = INDEX_NONE;
	if (!Stem.FindChar(TEXT('_'), UnderscoreIndex) || UnderscoreIndex <= 0)
	{
		return Stem;
	}

	const FString Prefix = Stem.Left(UnderscoreIndex);
	FString Remainder = Stem.RightChop(UnderscoreIndex + 1);
	if (Remainder.IsEmpty())
	{
		return Stem;
	}

	// A doubled prefix ("MyNode_MyNode_DoThing") is a naming slip, not two families; drop both.
	if (Remainder.StartsWith(Prefix + TEXT("_"), ESearchCase::CaseSensitive))
	{
		const FString Doubled = Remainder.RightChop(Prefix.Len() + 1);
		if (!Doubled.IsEmpty())
		{
			Remainder = Doubled;
		}
	}

	return Remainder;
}

void UFlowCatalogQuery::ResolveStems(TArray<FFlowCatalogClassRow>& Rows)
{
	// Precedence ladder, first rule that separates two rows wins. A lower score wins the bare stem.
	auto ScoreRow = [](const FFlowCatalogClassRow& Row) -> int32
	{
		// 1. Deprecated always loses - an author should never be handed the retired name.
		if (Row.bDeprecated)
		{
			return 100;
		}

		// 2. A class from the plugin that defines the Flow node hierarchy owns the plain name; a
		//    domain class deliberately shadowing it is the one that qualifies itself.
		if (const UClass* Class = FindObject<UClass>(nullptr, *Row.ClassPath))
		{
			static const TSharedPtr<IPlugin> CorePlugin = IPluginManager::Get().GetModuleOwnerPlugin(
				FPackageName::GetShortFName(UFlowNode::StaticClass()->GetPackage()->GetName()));

			const FName ModuleName = FPackageName::GetShortFName(Class->GetPackage()->GetName());
			if (CorePlugin.IsValid() && IPluginManager::Get().GetModuleOwnerPlugin(ModuleName) == CorePlugin)
			{
				return 0;
			}
		}

		// 3. Compiled code outranks script, which outranks content.
		if (Row.Origin == EFlowClassOrigin::Blueprint)
		{
			return 30;
		}
		if (Row.Origin == EFlowClassOrigin::Native)
		{
			return 10;
		}
		return 20;
	};

	TMap<FString, TArray<int32>> StemToRowIndices;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		Rows[Index].Stem = MakeClassStem(Rows[Index].ClassName);
		StemToRowIndices.FindOrAdd(Rows[Index].Stem).Add(Index);
	}

	for (const TPair<FString, TArray<int32>>& Pair : StemToRowIndices)
	{
		if (Pair.Value.Num() < 2)
		{
			continue;
		}

		int32 BestIndex = INDEX_NONE;
		int32 BestScore = MAX_int32;
		bool bBestIsTied = false;
		for (const int32 Index : Pair.Value)
		{
			const int32 Score = ScoreRow(Rows[Index]);
			if (Score < BestScore)
			{
				BestScore = Score;
				BestIndex = Index;
				bBestIsTied = false;
			}
			else if (Score == BestScore)
			{
				bBestIsTied = true;
			}
		}

		// An unbreakable tie means two implementations of one thing; neither may claim the stem, and
		// the ambiguity is left visible rather than resolved by an arbitrary rule.
		for (const int32 Index : Pair.Value)
		{
			if (bBestIsTied || Index != BestIndex)
			{
				Rows[Index].Stem = Rows[Index].ClassName;
			}
		}
	}
}

FString LexToString(EFlowClassOrigin Origin)
{
	switch (Origin)
	{
	case EFlowClassOrigin::Any:         return TEXT("any");
	case EFlowClassOrigin::Native:      return TEXT("native");
	case EFlowClassOrigin::Blueprint:   return TEXT("blueprint");
	case EFlowClassOrigin::AngelScript: return TEXT("angelscript");
	case EFlowClassOrigin::Unknown:     return TEXT("unknown");
	}
	return TEXT("unknown");
}

bool LexTryParseString(EFlowClassOrigin& OutOrigin, const FStringView Text)
{
	if (Text.Equals(TEXT("any"), ESearchCase::IgnoreCase) || Text.IsEmpty())
	{
		OutOrigin = EFlowClassOrigin::Any;
		return true;
	}
	if (Text.Equals(TEXT("native"), ESearchCase::IgnoreCase))
	{
		OutOrigin = EFlowClassOrigin::Native;
		return true;
	}
	if (Text.Equals(TEXT("blueprint"), ESearchCase::IgnoreCase))
	{
		OutOrigin = EFlowClassOrigin::Blueprint;
		return true;
	}
	if (Text.Equals(TEXT("angelscript"), ESearchCase::IgnoreCase))
	{
		OutOrigin = EFlowClassOrigin::AngelScript;
		return true;
	}
	if (Text.Equals(TEXT("unknown"), ESearchCase::IgnoreCase))
	{
		OutOrigin = EFlowClassOrigin::Unknown;
		return true;
	}
	return false;
}

EFlowClassOrigin UFlowCatalogQuery::GetClassOrigin(const UClass* NodeOrAddOnClass)
{
	if (!NodeOrAddOnClass)
	{
		return EFlowClassOrigin::Unknown;
	}

	if (NodeOrAddOnClass->ClassGeneratedBy != nullptr)
	{
		return EFlowClassOrigin::Blueprint;
	}

	// Detected by package rather than by class type: the AngelScript integration's UASClass lives in
	// its own plugin, and this module must not take a dependency on it to answer a documentation query.
	const UPackage* Package = NodeOrAddOnClass->GetPackage();
	const FString PackageName = Package ? Package->GetName() : FString();
	if (PackageName == TEXT("/Script/Angelscript"))
	{
		return EFlowClassOrigin::AngelScript;
	}

#if WITH_EDITOR
	// Only the header tool emits ModuleRelativePath, so its presence is a reliable "compiled from a
	// .h/.cpp" marker that distinguishes native classes from any script VM's generated ones.
	static const FName NAME_ModuleRelativePath(TEXT("ModuleRelativePath"));
	if (NodeOrAddOnClass->HasMetaData(NAME_ModuleRelativePath))
	{
		return EFlowClassOrigin::Native;
	}
	return EFlowClassOrigin::Unknown;
#else
	return EFlowClassOrigin::Native;
#endif
}

bool UFlowCatalogQuery::IsClassDeprecated(const UClass* NodeOrAddOnClass)
{
#if WITH_EDITORONLY_DATA
	if (const UFlowNodeBase* DefaultNode = NodeOrAddOnClass ? Cast<UFlowNodeBase>(NodeOrAddOnClass->GetDefaultObject()) : nullptr)
	{
		return DefaultNode->IsDeprecated();
	}
#endif
	return false;
}

FString UFlowCatalogQuery::GetReplacedByClassName(const UClass* NodeOrAddOnClass)
{
#if WITH_EDITORONLY_DATA
	if (const UFlowNodeBase* DefaultNode = NodeOrAddOnClass ? Cast<UFlowNodeBase>(NodeOrAddOnClass->GetDefaultObject()) : nullptr)
	{
		if (const UClass* Successor = DefaultNode->GetReplacedByClass())
		{
			return Successor->GetName();
		}
	}
#endif
	return FString();
}

void UFlowCatalogQuery::GatherAgentDoc(const UClass* NodeOrAddOnClass, FFlowCatalogDocRow& OutDoc, TArray<FString>& OutArticles)
{
	OutDoc = FFlowCatalogDocRow();
	OutArticles.Reset();

#if WITH_EDITOR
	const UFlowNodeBase* DefaultNode = NodeOrAddOnClass ? Cast<UFlowNodeBase>(NodeOrAddOnClass->GetDefaultObject()) : nullptr;
	if (!DefaultNode)
	{
		return;
	}

	const FFlowAgentDoc& AgentDoc = DefaultNode->GetAgentDoc();
	OutDoc.Guidance = AgentDoc.Guidance;

	for (const FName& Tag : AgentDoc.Tags)
	{
		OutDoc.Tags.Add(Tag.ToString());
	}

	// UCLASS Keywords is the only searchable-term metadata an AngelScript or Blueprint class can set -
	// neither can author a Tags array, which is C++-only (GetAgentDoc is a virtual override). Folding
	// tokenized Keywords into Tags here, deduped, means an agent reading Doc.Tags sees every class's
	// searchable terms in one place instead of native classes' terms being silently invisible once
	// Keywords stops being its own reported field.
	for (const FString& Keyword : SplitAndLowerKeywords(GetNodeKeywords(NodeOrAddOnClass)))
	{
		OutDoc.Tags.AddUnique(Keyword);
	}

	for (const FName& Article : AgentDoc.Articles)
	{
		OutArticles.Add(Article.ToString());
	}
#endif
}

FFlowCatalogClassRow UFlowCatalogQuery::BuildRow(const UClass* Class, bool bIsAddon, const FFlowCatalogQueryParams& Params)
{
	FFlowCatalogClassRow Row;
	if (!Class)
	{
		return Row;
	}

	// Identity is never optional: a row without its authoritative class path cannot be acted on.
	Row.ClassName = Class->GetName();
	Row.ClassPath = Class->GetPathName();
	Row.bIsAddon = bIsAddon;
	Row.Category = GetNodeCategory(Class);
	Row.Origin = GetClassOrigin(Class);
	Row.bDeprecated = IsClassDeprecated(Class);

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Doc))
	{
		Row.Description = GetNodeDescription(Class);

		if (Params.bDescriptionOnly)
		{
			// The "brief" preset carries only Description; avoid gathering the authored doc.
			Row.Doc = FFlowCatalogDocRow();
		}
		else
		{
			TArray<FString> DocArticles;
			GatherAgentDoc(Class, Row.Doc, DocArticles);
		}
	}

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Articles))
	{
		FFlowCatalogDocRow UnusedDoc;
		GatherAgentDoc(Class, UnusedDoc, Row.Articles);
	}

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Properties))
	{
		GatherProperties(Class, Row.Properties);
	}

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Pins))
	{
		GatherPins(Class, Row.InputPins, Row.OutputPins);
	}

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Deprecation))
	{
		Row.ReplacedBy = GetReplacedByClassName(Class);
	}

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Usage))
	{
		const FFlowNodeUsageSummary Usage = FFlowNodeUsageIndex::Get().GetUsageSummary(Row.ClassPath);
		Row.UsageCount = Usage.InstanceCount;
		Row.UsageAssetCount = Usage.AssetCount;
	}

	return Row;
}

void UFlowCatalogQuery::ListAssetTypes(TArray<FFlowCatalogAssetTypeRow>& OutAssetTypes)
{
	OutAssetTypes.Reset();

#if WITH_EDITOR
	TArray<UClass*> NodeClasses;
	GatherAllNodeClasses(NodeClasses);

	TArray<UClass*> AddOnClasses;
	GatherAllAddOnClasses(AddOnClasses);

	TArray<UClass*> AssetClasses;
	GatherAllFlowAssetClasses(AssetClasses);

	for (const UClass* AssetClass : AssetClasses)
	{
		const UFlowAsset* AssetCDO = AssetClass->GetDefaultObject<UFlowAsset>();
		if (!AssetCDO)
		{
			continue;
		}

		FFlowCatalogAssetTypeRow Row;
		Row.ClassName = AssetClass->GetName();
		Row.ClassPath = AssetClass->GetPathName();
		Row.Description = AssetClass->GetToolTipText().ToString();
		Row.Keywords = GetNodeKeywords(AssetClass);

		for (const UClass* NodeClass : NodeClasses)
		{
			if (AssetCDO->IsNodeOrAddOnClassAllowed(NodeClass))
			{
				++Row.AllowedNodeCount;
			}
		}
		for (const UClass* AddOnClass : AddOnClasses)
		{
			if (AssetCDO->IsNodeOrAddOnClassAllowed(AddOnClass))
			{
				++Row.AllowedAddonCount;
			}
		}

		OutAssetTypes.Add(Row);
	}
#endif
}

void UFlowCatalogQuery::QueryCatalog(const FFlowCatalogQueryParams& Params, FFlowCatalogQueryResult& OutResult)
{
	OutResult = FFlowCatalogQueryResult();
	OutResult.EffectiveSections = SectionsToString(Params.Sections);

	TArray<UClass*> AllNodeClasses;
	GatherAllNodeClasses(AllNodeClasses);

	TArray<UClass*> AllAddOnClasses;
	GatherAllAddOnClasses(AllAddOnClasses);

	// ClassNames is exclusive: when non-empty, it is the only filter applied.
	if (!Params.ClassNames.IsEmpty())
	{
		TArray<const UClass*> ResolvedNodes;
		TArray<const UClass*> ResolvedAddons;

		for (const FString& RequestedName : Params.ClassNames)
		{
			// Lenient input, canonical output: a stem, a full short name and a full class path are all
			// accepted, because an agent that read a stem out of one response must be able to feed it
			// straight back in.
			auto MatchesRequest = [&RequestedName](const UClass* Class)
			{
				return Class->GetName() == RequestedName ||
					Class->GetPathName() == RequestedName ||
					MakeClassStem(Class->GetName()) == RequestedName;
			};

			TArray<const UClass*> Candidates;
			for (const UClass* Class : AllNodeClasses)
			{
				if (MatchesRequest(Class))
				{
					Candidates.Add(Class);
				}
			}
			const int32 NodeCandidateCount = Candidates.Num();
			for (const UClass* Class : AllAddOnClasses)
			{
				if (MatchesRequest(Class))
				{
					Candidates.Add(Class);
				}
			}

			if (Candidates.IsEmpty())
			{
				OutResult.UnresolvedClassNames.Add(RequestedName);
				continue;
			}

			if (Candidates.Num() > 1)
			{
				// Never guess. Guessing here would silently document the wrong class.
				FFlowCatalogAmbiguousNameRow& Ambiguous = OutResult.AmbiguousClassNames.AddDefaulted_GetRef();
				Ambiguous.Stem = RequestedName;
				for (const UClass* Candidate : Candidates)
				{
					Ambiguous.Candidates.Add(Candidate->GetName());
				}
				continue;
			}

			if (NodeCandidateCount == 1)
			{
				ResolvedNodes.Add(Candidates[0]);
			}
			else
			{
				ResolvedAddons.Add(Candidates[0]);
			}
		}

		OutResult.TotalNodeCount = ResolvedNodes.Num();
		OutResult.TotalAddonCount = ResolvedAddons.Num();

		// An exact request always resolves, deprecated or not: reading a legacy graph is precisely
		// when a retired class has to be describable.
		for (const UClass* Class : ResolvedNodes)
		{
			OutResult.Nodes.Add(BuildRow(Class, false, Params));
		}
		for (const UClass* Class : ResolvedAddons)
		{
			OutResult.Addons.Add(BuildRow(Class, true, Params));
		}

		ResolveStems(OutResult.Nodes);
		ResolveStems(OutResult.Addons);
		return;
	}

	// Filtered-query path.
	bool bAssetClassFound = true;
	const UFlowAsset* AssetCDO = ResolveAssetCDOByName(Params.AssetClassName, bAssetClassFound);
	if (!bAssetClassFound)
	{
		OutResult.ErrorMessage = FString::Printf(TEXT("No FlowAsset subclass found with name: %s"), *Params.AssetClassName);
		return;
	}

	auto MatchesFilters = [&Params, AssetCDO](const UClass* Class)
	{
		if (AssetCDO && !AssetCDO->IsNodeOrAddOnClassAllowed(Class))
		{
			return false;
		}
		if (!Params.bIncludeDeprecated && IsClassDeprecated(Class))
		{
			return false;
		}
		if (!ClassHasPinOfAnyType(Class, Params.PinTypes))
		{
			return false;
		}

		FFlowCatalogDocRow Doc;
		TArray<FString> Articles;
		GatherAgentDoc(Class, Doc, Articles);

		if (!MatchesQuery(Class->GetName(), GetNodeDescription(Class), Params.Query))
		{
			return false;
		}
		if (!MatchesAnyKeyword(GetNodeKeywords(Class) + TEXT(" ") + FString::Join(Doc.Tags, TEXT(" ")), Params.Keywords))
		{
			return false;
		}
		if (!MatchesAnyCategory(GetNodeCategory(Class), Params.Categories))
		{
			return false;
		}
		if (Params.Origin != EFlowClassOrigin::Any && GetClassOrigin(Class) != Params.Origin)
		{
			return false;
		}
		if (!MatchesAnyArticle(Articles, Params.Articles))
		{
			return false;
		}
		return true;
	};

	TArray<const UClass*> MatchedNodes;
	if (Params.Kind == EFlowCatalogKind::Node || Params.Kind == EFlowCatalogKind::Any)
	{
		for (const UClass* NodeClass : AllNodeClasses)
		{
			if (MatchesFilters(NodeClass))
			{
				MatchedNodes.Add(NodeClass);
			}
		}
	}

	TArray<const UClass*> MatchedAddons;
	if (Params.Kind == EFlowCatalogKind::Addon || Params.Kind == EFlowCatalogKind::Any)
	{
		for (const UClass* AddOnClass : AllAddOnClasses)
		{
			if (MatchesFilters(AddOnClass))
			{
				MatchedAddons.Add(AddOnClass);
			}
		}
	}

	OutResult.TotalNodeCount = MatchedNodes.Num();
	OutResult.TotalAddonCount = MatchedAddons.Num();

	if (EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Shape))
	{
		OutResult.Shape.NodeCount = MatchedNodes.Num();
		OutResult.Shape.AddonCount = MatchedAddons.Num();

		auto AccumulateShape = [&OutResult](const UClass* Class)
		{
			const FString Category = GetNodeCategory(Class);
			++FindOrAddFacet(OutResult.Shape.Categories, Category.IsEmpty() ? TEXT("(none)") : Category)->Count;

			for (const FString& Keyword : SplitAndLowerKeywords(GetNodeKeywords(Class)))
			{
				++FindOrAddFacet(OutResult.Shape.Keywords, Keyword)->Count;
			}

			FFlowCatalogDocRow Doc;
			TArray<FString> Articles;
			GatherAgentDoc(Class, Doc, Articles);

			if (!Doc.IsEmpty())
			{
				++OutResult.Shape.DocumentedCount;
			}

			if (IsClassDeprecated(Class))
			{
				++OutResult.Shape.DeprecatedCount;
			}

			for (const FString& Tag : Doc.Tags)
			{
				++FindOrAddFacet(OutResult.Shape.Tags, Tag.ToLower())->Count;
			}

			for (const FString& Article : Articles)
			{
				++FindOrAddFacet(OutResult.Shape.Articles, Article.ToLower())->Count;
			}
		};

		for (const UClass* Class : MatchedNodes)
		{
			AccumulateShape(Class);
		}
		for (const UClass* Class : MatchedAddons)
		{
			AccumulateShape(Class);
		}

		// Ranked by count, then capped: a shape call that lists all 98 categories - 39 of them appearing
		// exactly once - costs more than it explains and is what a caller pays for on every orientation
		// query. The tail is reported as counts so nothing is silently hidden, and the full list is one
		// filtered query away.
		auto RankAndCapFacets = [](TArray<FFlowCatalogFacetRow>& Facets, int32& OutDistinctCount, int32& OutOtherCount)
		{
			Facets.Sort([](const FFlowCatalogFacetRow& A, const FFlowCatalogFacetRow& B)
			{
				if (A.Count != B.Count)
				{
					return A.Count > B.Count;
				}
				return A.Value < B.Value;
			});

			OutDistinctCount = Facets.Num();
			OutOtherCount = 0;
			for (int32 Index = MaxShapeFacets; Index < Facets.Num(); ++Index)
			{
				OutOtherCount += Facets[Index].Count;
			}
			if (Facets.Num() > MaxShapeFacets)
			{
				Facets.SetNum(MaxShapeFacets);
			}
		};
		RankAndCapFacets(OutResult.Shape.Categories, OutResult.Shape.DistinctCategoryCount, OutResult.Shape.OtherCategoryCount);
		RankAndCapFacets(OutResult.Shape.Keywords, OutResult.Shape.DistinctKeywordCount, OutResult.Shape.OtherKeywordCount);
		RankAndCapFacets(OutResult.Shape.Tags, OutResult.Shape.DistinctTagCount, OutResult.Shape.OtherTagCount);
		RankAndCapFacets(OutResult.Shape.Articles, OutResult.Shape.DistinctArticleCount, OutResult.Shape.OtherArticleCount);
	}

	// Shape is a histogram of the whole matched scope, so emitting rows alongside it would defeat the
	// point of the cheapest call in the surface.
	if (!EnumHasAnyFlags(Params.Sections, EFlowCatalogSection::Names))
	{
		return;
	}

	TArray<FFlowCatalogClassRow> NodeRows;
	NodeRows.Reserve(MatchedNodes.Num());
	for (const UClass* Class : MatchedNodes)
	{
		NodeRows.Add(BuildRow(Class, false, Params));
	}

	TArray<FFlowCatalogClassRow> AddonRows;
	AddonRows.Reserve(MatchedAddons.Num());
	for (const UClass* Class : MatchedAddons)
	{
		AddonRows.Add(BuildRow(Class, true, Params));
	}

	ApplyPaging(Params.Offset, Params.Limit, NodeRows, OutResult.bTruncated);
	ApplyPaging(Params.Offset, Params.Limit, AddonRows, OutResult.bTruncated);

	// Stems are resolved against what is actually emitted, so a collision only costs a full name when
	// both colliding classes are in the same response.
	ResolveStems(NodeRows);
	ResolveStems(AddonRows);

	OutResult.Nodes = MoveTemp(NodeRows);
	OutResult.Addons = MoveTemp(AddonRows);
}

namespace
{
	TArray<TSharedPtr<FJsonValue>> FacetsToJson(const TArray<FFlowCatalogFacetRow>& Facets)
	{
		TArray<TSharedPtr<FJsonValue>> FacetsJson;
		for (const FFlowCatalogFacetRow& Facet : Facets)
		{
			TSharedPtr<FJsonObject> FacetObj = MakeShared<FJsonObject>();
			FacetObj->SetStringField(TEXT("value"), Facet.Value);
			FacetObj->SetNumberField(TEXT("count"), Facet.Count);
			FacetsJson.Add(MakeShared<FJsonValueObject>(FacetObj));
		}
		return FacetsJson;
	}

	TArray<TSharedPtr<FJsonValue>> PinRowsToJson(const TArray<FFlowCatalogPinRow>& PinRows)
	{
		TArray<TSharedPtr<FJsonValue>> PinsJson;
		for (const FFlowCatalogPinRow& PinRow : PinRows)
		{
			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("name"), PinRow.Name);
			PinObj->SetStringField(TEXT("type"), PinRow.Type);
			if (!PinRow.Description.IsEmpty())
			{
				PinObj->SetStringField(TEXT("description"), PinRow.Description);
			}
			PinsJson.Add(MakeShared<FJsonValueObject>(PinObj));
		}
		return PinsJson;
	}

	/**
	 * Emits only requested sections so a bare name list contains no empty detail fields.
	 */
	TArray<TSharedPtr<FJsonValue>> RowsToJson(const TArray<FFlowCatalogClassRow>& Rows, EFlowCatalogSection Sections)
	{
		TArray<TSharedPtr<FJsonValue>> RowsJson;
		for (const FFlowCatalogClassRow& Row : Rows)
		{
			TSharedPtr<FJsonObject> RowObj = MakeShared<FJsonObject>();
			RowObj->SetStringField(TEXT("name"), Row.Stem);
			RowObj->SetStringField(TEXT("class_path"), Row.ClassPath);

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Doc))
			{
				if (!Row.Description.IsEmpty())
				{
					RowObj->SetStringField(TEXT("description"), Row.Description);
				}

				TSharedPtr<FJsonObject> DocObj = MakeShared<FJsonObject>();
				if (!Row.Doc.Guidance.IsEmpty())
				{
					DocObj->SetStringField(TEXT("guidance"), Row.Doc.Guidance);
				}
				if (!Row.Doc.Tags.IsEmpty())
				{
					DocObj->SetArrayField(TEXT("tags"), StringsToJsonArray(Row.Doc.Tags));
				}
				if (DocObj->Values.Num() > 0)
				{
					RowObj->SetObjectField(TEXT("doc"), DocObj);
				}
			}

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Articles) && !Row.Articles.IsEmpty())
			{
				RowObj->SetArrayField(TEXT("articles"), StringsToJsonArray(Row.Articles));
			}

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Origin))
			{
				RowObj->SetStringField(TEXT("origin"), LexToString(Row.Origin));
			}

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Deprecation) && Row.bDeprecated)
			{
				RowObj->SetBoolField(TEXT("deprecated"), true);
				if (!Row.ReplacedBy.IsEmpty())
				{
					RowObj->SetStringField(TEXT("replaced_by"), Row.ReplacedBy);
				}
			}

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Properties))
			{
				TArray<TSharedPtr<FJsonValue>> PropertiesJson;
				for (const FFlowCatalogPropertyRow& PropRow : Row.Properties)
				{
					TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
					PropObj->SetStringField(TEXT("name"), PropRow.Name);
					PropObj->SetStringField(TEXT("type"), PropRow.Type);
					PropObj->SetStringField(TEXT("declaring_class"), PropRow.DeclaringClass);
					if (PropRow.PinBinding != TEXT("-"))
					{
						PropObj->SetStringField(TEXT("pin_binding"), PropRow.PinBinding);
					}
					if (!PropRow.Value.IsEmpty())
					{
						PropObj->SetStringField(TEXT("value"), PropRow.Value);
					}
					if (!PropRow.Description.IsEmpty())
					{
						PropObj->SetStringField(TEXT("description"), PropRow.Description);
					}
					PropertiesJson.Add(MakeShared<FJsonValueObject>(PropObj));
				}
				RowObj->SetArrayField(TEXT("properties"), PropertiesJson);
			}

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Pins))
			{
				RowObj->SetArrayField(TEXT("input_pins"), PinRowsToJson(Row.InputPins));
				RowObj->SetArrayField(TEXT("output_pins"), PinRowsToJson(Row.OutputPins));
			}

			if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Usage))
			{
				RowObj->SetNumberField(TEXT("usage_count"), Row.UsageCount);
				RowObj->SetNumberField(TEXT("usage_asset_count"), Row.UsageAssetCount);
			}

			RowsJson.Add(MakeShared<FJsonValueObject>(RowObj));
		}
		return RowsJson;
	}

	FString SerializeQueryResultToJson(const FFlowCatalogQueryResult& Result, EFlowCatalogSection Sections)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		if (!Result.ErrorMessage.IsEmpty())
		{
			Obj->SetStringField(TEXT("errorMessage"), Result.ErrorMessage);
		}

		if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Shape))
		{
			TSharedPtr<FJsonObject> ShapeObj = MakeShared<FJsonObject>();
			ShapeObj->SetNumberField(TEXT("node_count"), Result.Shape.NodeCount);
			ShapeObj->SetNumberField(TEXT("addon_count"), Result.Shape.AddonCount);
			ShapeObj->SetNumberField(TEXT("documented_count"), Result.Shape.DocumentedCount);
			ShapeObj->SetNumberField(TEXT("deprecated_count"), Result.Shape.DeprecatedCount);

			// Each facet list is capped, so its distinct total and omitted tail travel with it. Without
			// these a caller cannot tell "these are all the categories" from "these are the top few".
			auto EmitFacets = [&ShapeObj](const FString& Key, const TArray<FFlowCatalogFacetRow>& Facets,
				int32 DistinctCount, int32 OtherCount)
			{
				ShapeObj->SetArrayField(Key, FacetsToJson(Facets));
				ShapeObj->SetNumberField(Key + TEXT("_distinct"), DistinctCount);
				if (OtherCount > 0)
				{
					ShapeObj->SetNumberField(Key + TEXT("_other"), OtherCount);
				}
			};
			EmitFacets(TEXT("categories"), Result.Shape.Categories, Result.Shape.DistinctCategoryCount, Result.Shape.OtherCategoryCount);
			EmitFacets(TEXT("keywords"), Result.Shape.Keywords, Result.Shape.DistinctKeywordCount, Result.Shape.OtherKeywordCount);
			EmitFacets(TEXT("tags"), Result.Shape.Tags, Result.Shape.DistinctTagCount, Result.Shape.OtherTagCount);
			EmitFacets(TEXT("articles"), Result.Shape.Articles, Result.Shape.DistinctArticleCount, Result.Shape.OtherArticleCount);
			Obj->SetObjectField(TEXT("shape"), ShapeObj);
		}

		if (EnumHasAnyFlags(Sections, EFlowCatalogSection::Names))
		{
			Obj->SetArrayField(TEXT("nodes"), RowsToJson(Result.Nodes, Sections));
			Obj->SetArrayField(TEXT("addons"), RowsToJson(Result.Addons, Sections));
		}

		Obj->SetNumberField(TEXT("total_node_count"), Result.TotalNodeCount);
		Obj->SetNumberField(TEXT("total_addon_count"), Result.TotalAddonCount);
		if (Result.bTruncated)
		{
			Obj->SetBoolField(TEXT("truncated"), true);
		}
		Obj->SetStringField(TEXT("sections"), Result.EffectiveSections);

		if (!Result.UnresolvedClassNames.IsEmpty())
		{
			Obj->SetArrayField(TEXT("unresolved_class_names"), StringsToJsonArray(Result.UnresolvedClassNames));
		}
		if (!Result.AmbiguousClassNames.IsEmpty())
		{
			TArray<TSharedPtr<FJsonValue>> AmbiguousJson;
			for (const FFlowCatalogAmbiguousNameRow& Ambiguous : Result.AmbiguousClassNames)
			{
				TSharedPtr<FJsonObject> AmbiguousObj = MakeShared<FJsonObject>();
				AmbiguousObj->SetStringField(TEXT("stem"), Ambiguous.Stem);
				AmbiguousObj->SetArrayField(TEXT("candidates"), StringsToJsonArray(Ambiguous.Candidates));
				AmbiguousJson.Add(MakeShared<FJsonValueObject>(AmbiguousObj));
			}
			Obj->SetArrayField(TEXT("ambiguous_class_names"), AmbiguousJson);
		}

		FString OutputJson;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputJson);
		FJsonSerializer::Serialize(Obj.ToSharedRef(), Writer);
		return OutputJson;
	}

	FString MakeErrorJson(const FString& ErrorMessage)
	{
		TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
		ErrorObj->SetStringField(TEXT("errorMessage"), ErrorMessage);
		FString ErrorJson;
		const TSharedRef<TJsonWriter<>> ErrorWriter = TJsonWriterFactory<>::Create(&ErrorJson);
		FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), ErrorWriter);
		return ErrorJson;
	}
}

FString UFlowCatalogQuery::QueryFlowCatalogAsJson(
	const FString& AssetClassName,
	const TArray<FString>& ClassNames,
	const FString& Kind,
	const FString& Query,
	const TArray<FString>& Keywords,
	const TArray<FString>& Categories,
	const TArray<FString>& Articles,
	const TArray<FString>& PinTypes,
	EFlowClassOrigin Origin,
	const FString& Sections,
	bool bIncludeDeprecated,
	int32 Limit,
	int32 Offset)
{
	FFlowCatalogQueryParams Params;
	Params.AssetClassName = AssetClassName;
	Params.ClassNames = ClassNames;
	Params.Kind = ParseKind(Kind);
	Params.Query = Query;
	Params.Keywords = Keywords;
	Params.Categories = Categories;
	Params.Articles = Articles;
	Params.PinTypes = PinTypes;
	Params.Origin = Origin;
	Params.bIncludeDeprecated = bIncludeDeprecated;
	Params.Limit = Limit;
	Params.Offset = Offset;

	FString SectionsError;
	if (!ParseSections(Sections, Params.Sections, Params.bDescriptionOnly, SectionsError))
	{
		return MakeErrorJson(SectionsError);
	}

	FFlowCatalogQueryResult Result;
	QueryCatalog(Params, Result);
	return SerializeQueryResultToJson(Result, Params.Sections);
}

bool UFlowCatalogQuery::ExportFlowCatalogToFile(const FString& OutputFilePath, const FString& AssetClassName)
{
	FFlowCatalogQueryParams Params;
	Params.AssetClassName = AssetClassName;
	Params.Kind = EFlowCatalogKind::Any;
	Params.Sections = FlowCatalogSectionsFull;
	Params.bIncludeDeprecated = true;

	FFlowCatalogQueryResult Result;
	QueryCatalog(Params, Result);

	const FString Json = SerializeQueryResultToJson(Result, Params.Sections);

	// UTF-8 without a BOM: the consumers of this file are text tools, and a UTF-16 payload makes an
	// ordinary utf-8 read fail outright rather than degrade.
	if (FFileHelper::SaveStringToFile(Json, *OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogFlow, Log, TEXT("FlowCatalogQuery: Successfully exported catalog to %s"), *OutputFilePath);
		return true;
	}
	else
	{
		UE_LOG(LogFlow, Error, TEXT("FlowCatalogQuery: Failed to write catalog to file %s"), *OutputFilePath);
		return false;
	}
}

void UFlowCatalogQuery::GatherAllNodeClasses(TArray<UClass*>& OutNodeClasses)
{
	OutNodeClasses.Reset();
	
	TArray<UClass*> NativeFlowNodes;
	GetDerivedClasses(UFlowNode::StaticClass(), NativeFlowNodes);
	
	for (UClass* Class : NativeFlowNodes)
	{
		if (Class->ClassGeneratedBy != nullptr)
		{
			continue;
		}
		
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) ||
			Class->HasMetaData(TEXT("ExcludeFromFlowCatalog")))
		{
			continue;
		}
		
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || 
			Class->GetName().StartsWith(TEXT("REINST_")) ||
			Class->GetName().StartsWith(TEXT("TRASHCLASS_")))
		{
			continue;
		}
		
		OutNodeClasses.Add(Class);
	}
	
	GatherBlueprintDerivedClasses(UFlowNodeBlueprint::StaticClass(), UFlowNode::StaticClass(), OutNodeClasses);
}

UClass* UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(const FString& ClassName)
{
	if (ClassName.Contains(TEXT("/Script/")) || ClassName.Contains(TEXT(".")))
	{
		if (UClass* Found = FindObject<UClass>(nullptr, *ClassName))
		{
			return Found;
		}
	}

	TArray<UClass*> Candidates;
	GatherAllNodeClasses(Candidates);

	TArray<UClass*> AddOnClasses;
	GatherAllAddOnClasses(AddOnClasses);
	Candidates.Append(AddOnClasses);

	for (UClass* Class : Candidates)
	{
		if (Class && Class->GetName() == ClassName)
		{
			return Class;
		}
	}

	// Fall back to the stem only after exact names have been exhausted, and only when the stem is
	// unambiguous - a single caller-facing name must never resolve to an arbitrary one of two classes.
	UClass* StemMatch = nullptr;
	for (UClass* Class : Candidates)
	{
		if (Class && MakeClassStem(Class->GetName()) == ClassName)
		{
			if (StemMatch)
			{
				return nullptr;
			}
			StemMatch = Class;
		}
	}

	return StemMatch;
}

FString UFlowCatalogQuery::GetNodeDescription(const UClass* NodeOrAddOnClass)
{
	if (!NodeOrAddOnClass)
	{
		return FString();
	}
	
#if WITH_EDITOR
	// UFlowNodeBase supplies tooltips for both nodes and addons.
	if (const UFlowNodeBase* DefaultNode = Cast<UFlowNodeBase>(NodeOrAddOnClass->GetDefaultObject()))
	{
		const FText Tooltip = DefaultNode->GetNodeToolTip();
		if (!Tooltip.IsEmpty())
		{
			return Tooltip.ToString();
		}
	}
	
	const FText ClassTooltip = NodeOrAddOnClass->GetToolTipText();
	if (!ClassTooltip.IsEmpty())
	{
		return ClassTooltip.ToString();
	}
#endif
	
	return FString();
}

FString UFlowCatalogQuery::GetNodeCategory(const UClass* NodeOrAddOnClass)
{
	if (!NodeOrAddOnClass)
	{
		return FString();
	}

#if WITH_EDITOR
	if (const UFlowNodeBase* DefaultNode = Cast<UFlowNodeBase>(NodeOrAddOnClass->GetDefaultObject()))
	{
		return DefaultNode->GetNodeCategory();
	}
#endif
	
	return FString();
}

namespace
{
	// The pin name an auto-generated data pin is shown under: the explicit name given to the
	// SourceForOutputFlowPin/DefaultForInputFlowPin meta key, or the property's display name if that
	// meta key was left blank.
	FString GetAutoDataPinName(const FProperty* Property, const FName& MetadataKey)
	{
		FString PinName = Property->GetMetaData(MetadataKey);
		if (PinName.IsEmpty())
		{
			PinName = Property->GetDisplayNameText().ToString();
		}
		return PinName;
	}

	// EditAnywhere and EditInstanceOnly both edit the placed node instance; EditDefaultsOnly
	// (CPF_Edit with CPF_DisableEditOnInstance) only edits the class default, which isn't reachable
	// on a specific node in a graph, so it's excluded here.
	bool IsCatalogPropertyEditable(const FProperty* Property)
	{
		return Property->HasAnyPropertyFlags(CPF_Edit) && !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
	}

	bool IsCatalogPropertyBlueprintWritable(const FProperty* Property)
	{
		return Property->HasAnyPropertyFlags(CPF_BlueprintVisible) && !Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly);
	}

	// A property is worth reporting if it's configurable (editable, or Blueprint-writable) or if it
	// plays a role as a Flow data pin's source/default - the latter can be true of a property with
	// neither Edit nor BlueprintVisible set (e.g. a pure SourceForOutputFlowPin backing field), so
	// this is deliberately an OR, not folded into the editable check.
	bool ShouldIncludeCatalogProperty(const FProperty* Property)
	{
		if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
		{
			return false;
		}

		const bool bHasPinBinding =
			Property->HasMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin) ||
			Property->HasMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin) ||
			Property->HasMetaData(FFlowPin::MetadataKey_FlowPinType);

		return IsCatalogPropertyEditable(Property) || IsCatalogPropertyBlueprintWritable(Property) || bHasPinBinding;
	}

	// True for a property whose only qualifying reason is being a live Flow data pin's source/default
	// (not bare FlowPinType, which just describes a struct's shape rather than a live pin) and that
	// isn't otherwise editable/Blueprint-writable. GatherPins already reports such a property as its
	// own pin row, cross-referenced back via FFlowCatalogPinRow::Property, so a properties[] row for
	// it too would just duplicate the same name/description under a second row shape. Only used at
	// the top level (GatherProperties) - a ShowOnlyInnerProperties struct's flattened fields are never
	// scanned by GatherPins, so excluding one there would drop it from both outputs entirely.
	bool IsPureDataPinSourceProperty(const FProperty* Property)
	{
		const bool bHasLiveBinding =
			Property->HasMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin) ||
			Property->HasMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);
		return bHasLiveBinding && !IsCatalogPropertyEditable(Property) && !IsCatalogPropertyBlueprintWritable(Property);
	}

	// Recurses into a ShowOnlyInnerProperties struct's own fields (dot-prefixing their names) instead
	// of reporting the struct as one opaque row, mirroring how the Details panel flattens it with no
	// foldout. Stops at any struct that isn't itself ShowOnlyInnerProperties (e.g. FVector, or a plain
	// nested struct), reporting that one as a single leaf row.
	void AppendCatalogPropertyRow(
		const FProperty* Property,
		const void* ContainerPtr,
		const FString& DeclaringClassName,
		const FString& QualifiedName,
		TArray<FFlowCatalogPropertyRow>& OutRows)
	{
		if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			if (StructProp->Struct && StructProp->HasMetaData(TEXT("ShowOnlyInnerProperties")))
			{
				const void* InnerContainer = StructProp->ContainerPtrToValuePtr<void>(ContainerPtr);
				for (TFieldIterator<FProperty> InnerIt(StructProp->Struct); InnerIt; ++InnerIt)
				{
					const FProperty* InnerProperty = *InnerIt;
					if (InnerProperty && ShouldIncludeCatalogProperty(InnerProperty))
					{
						AppendCatalogPropertyRow(InnerProperty, InnerContainer, DeclaringClassName,
							QualifiedName + TEXT(".") + InnerProperty->GetName(), OutRows);
					}
				}
				return;
			}
		}

		FFlowCatalogPropertyRow& Row = OutRows.AddDefaulted_GetRef();
		Row.Name = QualifiedName;
		Row.Type = UFlowCatalogQuery::GetPropertyTypeName(Property);
		Row.PinBinding = UFlowCatalogQuery::GetPropertyPinBinding(Property);
		Row.Description = UFlowCatalogQuery::GetPropertyTooltip(Property);
		Row.DeclaringClass = DeclaringClassName;
		Row.Value = UFlowCatalogQuery::GetPropertyValueString(Property, ContainerPtr);
	}
}

FString UFlowCatalogQuery::GetPropertyValueString(const FProperty* Property, const void* ContainerPtr)
{
	if (!Property || !ContainerPtr)
	{
		return TEXT("");
	}

	// Opaque (non-flattened) struct types get no terse value - stringifying an arbitrary struct isn't
	// well-defined, and ShowOnlyInnerProperties structs never reach here (their fields are flattened
	// into their own rows before GetPropertyValueString is called).
	if (CastField<FStructProperty>(Property))
	{
		return TEXT("");
	}

	if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		return BoolProp->GetPropertyValue_InContainer(ContainerPtr) ? TEXT("true") : TEXT("false");
	}

	if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(ContainerPtr));
		return FString::Printf(TEXT("%d elements"), Helper.Num());
	}

	if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(Property))
	{
		UObject* Value = ObjectProp->GetObjectPropertyValue_InContainer(ContainerPtr);
		return Value ? Value->GetName() : TEXT("None");
	}

	// Bare enum value name regardless of what qualified form GetNameStringByValue returns (e.g.
	// "EMyEnum::MyValue" or "MyValue") - the type is already reported separately in Row.Type, so
	// repeating it here would be redundant.
	auto GetBareEnumValueName = [](const UEnum* Enum, int64 Value) -> FString
	{
		const FString FullName = Enum->GetNameStringByValue(Value);
		int32 LastColon;
		return FullName.FindLastChar(':', LastColon) ? FullName.Mid(LastColon + 1) : FullName;
	};

	if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (const UEnum* Enum = ByteProp->Enum)
		{
			return GetBareEnumValueName(Enum, ByteProp->GetPropertyValue_InContainer(ContainerPtr));
		}
	}

	if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		if (const UEnum* Enum = EnumProp->GetEnum())
		{
			const FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
			const int64 Value = UnderlyingProp->GetSignedIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(ContainerPtr));
			return GetBareEnumValueName(Enum, Value);
		}
	}

	if (const FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
	{
		const void* ValuePtr = NumericProp->ContainerPtrToValuePtr<void>(ContainerPtr);
		return NumericProp->IsFloatingPoint()
			? FString::SanitizeFloat(NumericProp->GetFloatingPointPropertyValue(ValuePtr))
			: LexToString(NumericProp->GetSignedIntPropertyValue(ValuePtr));
	}

	if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		return StrProp->GetPropertyValue_InContainer(ContainerPtr);
	}

	if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		return NameProp->GetPropertyValue_InContainer(ContainerPtr).ToString();
	}

	if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		return TextProp->GetPropertyValue_InContainer(ContainerPtr).ToString();
	}

	return TEXT("");
}

void UFlowCatalogQuery::GatherProperties(const UClass* NodeOrAddOnClass, TArray<FFlowCatalogPropertyRow>& OutRows)
{
	OutRows.Reset();

	if (!NodeOrAddOnClass)
	{
		return;
	}

	const void* ContainerPtr = NodeOrAddOnClass->GetDefaultObject();

	// TFieldIterator<FProperty> already walks the full ancestor chain, visiting the leaf class's own
	// fields before its superclass's - which is also the order a caller wants to display them in, so
	// no re-sort by DeclaringClass is needed.
	for (TFieldIterator<FProperty> PropIt(NodeOrAddOnClass); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		if (!Property || !ShouldIncludeCatalogProperty(Property) || IsPureDataPinSourceProperty(Property))
		{
			continue;
		}

		const UClass* DeclaringClass = Property->GetOwnerClass();
		const FString DeclaringClassName = DeclaringClass ? DeclaringClass->GetName() : NodeOrAddOnClass->GetName();

		AppendCatalogPropertyRow(Property, ContainerPtr, DeclaringClassName, Property->GetName(), OutRows);
	}
}

void UFlowCatalogQuery::GatherPins(const UClass* NodeOrAddOnClass, TArray<FFlowCatalogPinRow>& OutInputPins, TArray<FFlowCatalogPinRow>& OutOutputPins)
{
	OutInputPins.Reset();
	OutOutputPins.Reset();

#if WITH_EDITOR
	// Cast<>() on the untyped CDO rather than GetDefaultObject<T>(), which would assert for a class
	// outside the expected hierarchy instead of reporting "no pins".
	const UFlowNodeBase* DefaultNode = NodeOrAddOnClass ? Cast<UFlowNodeBase>(NodeOrAddOnClass->GetDefaultObject()) : nullptr;
	if (!DefaultNode)
	{
		return;
	}

	// GetCatalogPins can run authored Blueprint or script logic on the CDO. This checks the *result* is
	// plausible and names the culprit if not; it cannot make the *call* safe, so every override on this
	// path must be CDO-safe by construction - reading defaults, never dereferencing an owner.
	TArray<FFlowPin> InputPins;
	TArray<FFlowPin> OutputPins;
	{
		UE_LOG(LogFlow, VeryVerbose, TEXT("FlowCatalogQuery: enumerating catalog pins for %s"), *NodeOrAddOnClass->GetPathName());

		DefaultNode->GetCatalogPins(InputPins, OutputPins);

		if (!ensureMsgf(InputPins.Num() < 512 && OutputPins.Num() < 512,
			TEXT("FlowCatalogQuery: %s reported an implausible pin count (%d in / %d out) from GetCatalogPins; ignoring its pins."),
			*NodeOrAddOnClass->GetPathName(), InputPins.Num(), OutputPins.Num()))
		{
			return;
		}
	}

	auto AppendPins = [](const TArray<FFlowPin>& Pins, TArray<FFlowCatalogPinRow>& OutRows)
	{
		for (const FFlowPin& Pin : Pins)
		{
			FFlowCatalogPinRow& Row = OutRows.AddDefaulted_GetRef();
			Row.Name = Pin.PinName.ToString();
			Row.Type = GetPinTypeName(Pin);
			Row.Description = Pin.PinToolTip;
		}
	};
	AppendPins(InputPins, OutInputPins);
	AppendPins(OutputPins, OutOutputPins);

	// Auto-generated data pins never appear in GetCatalogPins - that only covers the user-declared
	// exec pin arrays - so fold them in here from the full ancestor chain's properties. They're real
	// graph pins on the node just like the exec pins above, so callers shouldn't have to separately
	// cross-reference a property's PinBinding to know a data pin exists.
	for (TFieldIterator<FProperty> PropIt(NodeOrAddOnClass); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		if (!Property)
		{
			continue;
		}

		if (Property->HasMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin))
		{
			FFlowCatalogPinRow& Row = OutOutputPins.AddDefaulted_GetRef();
			Row.Name = GetAutoDataPinName(Property, FFlowPin::MetadataKey_SourceForOutputFlowPin);
			Row.Type = Property->GetMetaData(FFlowPin::MetadataKey_FlowPinType);
			Row.Description = Property->GetToolTipText().ToString();
		}
		else if (Property->HasMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin))
		{
			FFlowCatalogPinRow& Row = OutInputPins.AddDefaulted_GetRef();
			Row.Name = GetAutoDataPinName(Property, FFlowPin::MetadataKey_DefaultForInputFlowPin);
			Row.Type = Property->GetMetaData(FFlowPin::MetadataKey_FlowPinType);
			Row.Description = Property->GetToolTipText().ToString();
		}
	}

	// Deliberately NOT queried here: IFlowContextPinSupplierInterface. A context pin is derived from
	// the instance's live context - FlowNodeAddOn_SwitchCase reads its owning FlowNode to build one -
	// and a class default object has no owner, so the question is meaningless to ask of one. It is also
	// unguardable: those implementations assert on the missing owner, and an ensure is a debug break
	// rather than something a caller can catch or flag its way past. Pins that only exist on a real
	// instance are reported by FindFlowNodeUsage, which reads shipped assets.
#endif
}

FString UFlowCatalogQuery::GetPropertyTypeName(const FProperty* Property)
{
	if (!Property)
	{
		return TEXT("Unknown");
	}
	
	if (CastField<FBoolProperty>(Property))
	{
		return TEXT("Boolean");
	}
	else if (CastField<FIntProperty>(Property))
	{
		return TEXT("Integer");
	}
	else if (CastField<FInt64Property>(Property))
	{
		return TEXT("Integer64");
	}
	else if (CastField<FFloatProperty>(Property))
	{
		return TEXT("Float");
	}
	else if (CastField<FDoubleProperty>(Property))
	{
		return TEXT("Double");
	}
	else if (CastField<FStrProperty>(Property))
	{
		return TEXT("String");
	}
	else if (CastField<FNameProperty>(Property))
	{
		return TEXT("Name");
	}
	else if (CastField<FTextProperty>(Property))
	{
		return TEXT("Text");
	}
	else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		if (const UEnum* Enum = EnumProp->GetEnum())
		{
			return FString::Printf(TEXT("Enum (%s)"), *Enum->GetName());
		}
		return TEXT("Enum");
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (ByteProp->Enum)
		{
			return FString::Printf(TEXT("Enum (%s)"), *ByteProp->Enum->GetName());
		}
		return TEXT("Byte");
	}
	else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		if (ObjProp->PropertyClass)
		{
			return FString::Printf(TEXT("Object (%s)"), *ObjProp->PropertyClass->GetName());
		}
		return TEXT("Object");
	}
	else if (const FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
	{
		if (SoftObjProp->PropertyClass)
		{
			return FString::Printf(TEXT("Soft Object (%s)"), *SoftObjProp->PropertyClass->GetName());
		}
		return TEXT("Soft Object");
	}
	else if (const FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		if (ClassProp->MetaClass)
		{
			return FString::Printf(TEXT("Class (%s)"), *ClassProp->MetaClass->GetName());
		}
		return TEXT("Class");
	}
	else if (const FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
	{
		if (SoftClassProp->MetaClass)
		{
			return FString::Printf(TEXT("Soft Class (%s)"), *SoftClassProp->MetaClass->GetName());
		}
		return TEXT("Soft Class");
	}
	else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		if (StructProp->Struct)
		{
			return FString::Printf(TEXT("Struct (%s)"), *StructProp->Struct->GetName());
		}
		return TEXT("Struct");
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FString InnerType = GetPropertyTypeName(ArrayProp->Inner);
		return FString::Printf(TEXT("Array<%s>"), *InnerType);
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		FString ElementType = GetPropertyTypeName(SetProp->ElementProp);
		return FString::Printf(TEXT("Set<%s>"), *ElementType);
	}
	else if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		FString KeyType = GetPropertyTypeName(MapProp->KeyProp);
		FString ValueType = GetPropertyTypeName(MapProp->ValueProp);
		return FString::Printf(TEXT("Map<%s, %s>"), *KeyType, *ValueType);
	}
	
	return Property->GetCPPType();
}

FString UFlowCatalogQuery::GetPropertyTooltip(const FProperty* Property)
{
	if (!Property)
	{
		return TEXT("");
	}
	
#if WITH_EDITOR
	FText Tooltip = Property->GetToolTipText();
	if (!Tooltip.IsEmpty())
	{
		return Tooltip.ToString();
	}
#endif
	
	return TEXT("");
}

FString UFlowCatalogQuery::GetPropertyPinBinding(const FProperty* Property)
{
	if (!Property)
	{
		return TEXT("-");
	}
	
#if WITH_EDITOR
	bool bIsOutputPin = Property->HasMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin);
	bool bIsInputPin = Property->HasMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);
	bool bHasFlowPinType = Property->HasMetaData(FFlowPin::MetadataKey_FlowPinType);
	
	if (bIsOutputPin)
	{
		const FString PinName = GetAutoDataPinName(Property, FFlowPin::MetadataKey_SourceForOutputFlowPin);
		return FString::Printf(TEXT("Output Pin: `%s`"), *PinName);
	}
	else if (bIsInputPin)
	{
		const FString PinName = GetAutoDataPinName(Property, FFlowPin::MetadataKey_DefaultForInputFlowPin);
		return FString::Printf(TEXT("Input Pin (default): `%s`"), *PinName);
	}
	else if (bHasFlowPinType)
	{
		FString PinType = Property->GetMetaData(FFlowPin::MetadataKey_FlowPinType);
		return FString::Printf(TEXT("Data Pin (%s)"), *PinType);
	}
	
	if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		if (StructProp->Struct)
		{
			if (StructProp->Struct->HasMetaData(FFlowPin::MetadataKey_FlowPinType))
			{
				FString PinType = StructProp->Struct->GetMetaData(FFlowPin::MetadataKey_FlowPinType);
				return FString::Printf(TEXT("Data Pin (%s)"), *PinType);
			}
			
			FString StructName = StructProp->Struct->GetName();
			if (StructName.StartsWith(TEXT("FlowDataPinValue_")) || 
				StructName.StartsWith(TEXT("FlowDataPinOutputProperty_")) ||
				StructName.StartsWith(TEXT("FlowDataPinInputProperty_")))
			{
				FString PinType = StructName;
				PinType.RemoveFromStart(TEXT("FlowDataPinValue_"));
				PinType.RemoveFromStart(TEXT("FlowDataPinOutputProperty_"));
				PinType.RemoveFromStart(TEXT("FlowDataPinInputProperty_"));
				return FString::Printf(TEXT("Data Pin (%s)"), *PinType);
			}
		}
	}
#endif
	
	return TEXT("-");
}

FString UFlowCatalogQuery::GetPinTypeName(const FFlowPin& Pin)
{
	const FFlowPinTypeName& PinTypeName = Pin.GetPinTypeName();
	FString TypeStr = PinTypeName.ToString();
	
	if (TypeStr.IsEmpty() || TypeStr == TEXT("exec") || TypeStr == TEXT("Exec"))
	{
		return TEXT("Exec");
	}
	
	return TypeStr;
}

bool UFlowCatalogQuery::CanNodeAddUserInputPins(const UClass* NodeClass)
{
	if (!NodeClass)
	{
		return false;
	}
	
#if WITH_EDITOR
	const UFlowNode* DefaultNode = NodeClass->GetDefaultObject<UFlowNode>();
	if (DefaultNode)
	{
		return DefaultNode->CanUserAddInput();
	}
#endif
	
	return false;
}

bool UFlowCatalogQuery::CanNodeAddUserOutputPins(const UClass* NodeClass)
{
	if (!NodeClass)
	{
		return false;
	}
	
#if WITH_EDITOR
	const UFlowNode* DefaultNode = NodeClass->GetDefaultObject<UFlowNode>();
	if (DefaultNode)
	{
		return DefaultNode->CanUserAddOutput();
	}
#endif
	
	return false;
}

void UFlowCatalogQuery::GatherAllAddOnClasses(TArray<UClass*>& OutAddOnClasses)
{
	OutAddOnClasses.Reset();

	// Native C++ addon classes
	TArray<UClass*> NativeAddOns;
	GetDerivedClasses(UFlowNodeAddOn::StaticClass(), NativeAddOns);

	for (UClass* Class : NativeAddOns)
	{
		if (Class->ClassGeneratedBy != nullptr)
		{
			continue;
		}

		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) ||
			Class->HasMetaData(TEXT("ExcludeFromFlowCatalog")))
		{
			continue;
		}

		if (Class->GetName().StartsWith(TEXT("SKEL_")) ||
			Class->GetName().StartsWith(TEXT("REINST_")) ||
			Class->GetName().StartsWith(TEXT("TRASHCLASS_")))
		{
			continue;
		}

		OutAddOnClasses.Add(Class);
	}

	GatherBlueprintDerivedClasses(UFlowNodeAddOnBlueprint::StaticClass(), UFlowNodeAddOn::StaticClass(), OutAddOnClasses);
}

FString UFlowCatalogQuery::GetNodeKeywords(const UClass* NodeClass)
{
	if (!NodeClass)
	{
		return TEXT("");
	}

#if WITH_EDITOR
	static const FName NAME_Keywords(TEXT("Keywords"));
	if (NodeClass->HasMetaData(NAME_Keywords))
	{
		return NodeClass->GetMetaData(NAME_Keywords);
	}
#endif

	return TEXT("");
}

void UFlowCatalogQuery::GatherAllKeywords(
	const TArray<UClass*>& NodeClasses,
	const TArray<UClass*>& AddOnClasses,
	TMap<FString, TArray<FString>>& OutKeywordMap)
{
	OutKeywordMap.Reset();

	auto AccumulateClass = [&OutKeywordMap](const UClass* Class)
	{
		if (!Class)
		{
			return;
		}

		const FString RawKeywords = GetNodeKeywords(Class);
		if (RawKeywords.IsEmpty())
		{
			return;
		}

		// Determine the display name: prefer DisplayName meta, else strip common prefixes
		FString DisplayName;
#if WITH_EDITOR
		static const FName NAME_DisplayName(TEXT("DisplayName"));
		if (Class->HasMetaData(NAME_DisplayName))
		{
			DisplayName = Class->GetMetaData(NAME_DisplayName);
		}
#endif
		if (DisplayName.IsEmpty())
		{
			DisplayName = Class->GetName();
			DisplayName.RemoveFromStart(TEXT("FlowNode_"));
			DisplayName.RemoveFromStart(TEXT("FlowNodeAddOn_"));
			DisplayName.RemoveFromEnd(TEXT("_C"));
		}

		for (const FString& Token : SplitAndLowerKeywords(RawKeywords))
		{
			OutKeywordMap.FindOrAdd(Token).AddUnique(DisplayName);
		}
	};

	for (const UClass* Class : NodeClasses)
	{
		AccumulateClass(Class);
	}
	for (const UClass* Class : AddOnClasses)
	{
		AccumulateClass(Class);
	}

	// Sort display-name lists within each keyword entry for stable output
	for (auto& Pair : OutKeywordMap)
	{
		Pair.Value.Sort();
	}
}

void UFlowCatalogQuery::GatherAllFlowAssetClasses(TArray<UClass*>& OutAssetClasses)
{
	OutAssetClasses.Reset();

	TArray<UClass*> Derived;
	GetDerivedClasses(UFlowAsset::StaticClass(), Derived, /*bRecursive=*/true);

	for (UClass* Class : Derived)
	{
		if (Class->ClassGeneratedBy != nullptr)
		{
			continue;
		}

		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}

		if (Class->GetName().StartsWith(TEXT("SKEL_")) ||
			Class->GetName().StartsWith(TEXT("REINST_")) ||
			Class->GetName().StartsWith(TEXT("TRASHCLASS_")))
		{
			continue;
		}

		OutAssetClasses.Add(Class);
	}

	OutAssetClasses.Sort([](const UClass& A, const UClass& B)
	{
		return A.GetName() < B.GetName();
	});
}

bool UFlowCatalogQuery::ClassHasPinOfAnyType(const UClass* NodeOrAddOnClass, const TArray<FString>& PinTypeFilter)
{
	if (PinTypeFilter.IsEmpty())
	{
		return true;
	}

	TArray<FFlowCatalogPinRow> InputPins;
	TArray<FFlowCatalogPinRow> OutputPins;
	GatherPins(NodeOrAddOnClass, InputPins, OutputPins);

	for (const FFlowCatalogPinRow& Row : InputPins)
	{
		if (PinTypeFilter.Contains(Row.Type))
		{
			return true;
		}
	}
	for (const FFlowCatalogPinRow& Row : OutputPins)
	{
		if (PinTypeFilter.Contains(Row.Type))
		{
			return true;
		}
	}

	return false;
}

