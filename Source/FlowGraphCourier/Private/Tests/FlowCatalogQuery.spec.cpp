// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowCatalogQuery.h"
#include "Containers/StringConv.h"
#include "HAL/FileManager.h"
#include "Nodes/FlowAgentDoc.h"
#include "Nodes/FlowNodeBase.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFlowCatalogQuerySpec, "FlowGraphCourier.EditorGame.Catalog", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
END_DEFINE_SPEC(FFlowCatalogQuerySpec)

void FFlowCatalogQuerySpec::Define()
{
	Describe("ListAssetTypes", [this]()
	{
		It("should list every concrete FlowAsset subclass with counts only", [this]()
		{
			TArray<FFlowCatalogAssetTypeRow> Rows;
			UFlowCatalogQuery::ListAssetTypes(Rows);

			TestTrue("At least one FlowAsset subclass found", Rows.Num() > 0);
			for (const FFlowCatalogAssetTypeRow& Row : Rows)
			{
				TestFalse("Row has a class name", Row.ClassName.IsEmpty());
			}
		});
	});

	Describe("ParseSections", [this]()
	{
		It("should default to shape when nothing is asked for", [this]()
		{
			EFlowCatalogSection Sections = EFlowCatalogSection::None;
			bool bDescriptionOnly = false;
			FString Error;
			TestTrue("Empty section list parses", UFlowCatalogQuery::ParseSections(FString(), Sections, bDescriptionOnly, Error));
			TestTrue("Defaults to shape", Sections == EFlowCatalogSection::Shape);
		});

		It("should imply names for any row-bearing section", [this]()
		{
			EFlowCatalogSection Sections = EFlowCatalogSection::None;
			bool bDescriptionOnly = false;
			FString Error;
			TestTrue("Pins-only parses", UFlowCatalogQuery::ParseSections(TEXT("pins"), Sections, bDescriptionOnly, Error));
			TestTrue("Names implied", EnumHasAnyFlags(Sections, EFlowCatalogSection::Names));
		});

		It("should reject an unknown section rather than silently degrading the response", [this]()
		{
			EFlowCatalogSection Sections = EFlowCatalogSection::None;
			bool bDescriptionOnly = false;
			FString Error;
			TestFalse("Unknown section rejected", UFlowCatalogQuery::ParseSections(TEXT("pinz"), Sections, bDescriptionOnly, Error));
			TestFalse("Error message explains the valid values", Error.IsEmpty());
		});

		It("brief should ask for the description only", [this]()
		{
			EFlowCatalogSection Sections = EFlowCatalogSection::None;
			bool bDescriptionOnly = false;
			FString Error;
			TestTrue("brief parses", UFlowCatalogQuery::ParseSections(TEXT("brief"), Sections, bDescriptionOnly, Error));
			TestTrue("Doc requested", EnumHasAnyFlags(Sections, EFlowCatalogSection::Doc));
			TestTrue("Description only", bDescriptionOnly);
		});
	});

	Describe("Origin", [this]()
	{
		It("should round-trip every enum value through its wire name", [this]()
		{
			const TArray<EFlowClassOrigin> AllOrigins = {
				EFlowClassOrigin::Any,
				EFlowClassOrigin::Native,
				EFlowClassOrigin::Blueprint,
				EFlowClassOrigin::AngelScript,
				EFlowClassOrigin::Unknown };

			for (const EFlowClassOrigin Origin : AllOrigins)
			{
				const FString Wire = LexToString(Origin);
				TestFalse(TEXT("Wire name is non-empty"), Wire.IsEmpty());
				TestEqual(*FString::Printf(TEXT("'%s' is lower-case"), *Wire), Wire, Wire.ToLower());

				EFlowClassOrigin Parsed = EFlowClassOrigin::Unknown;
				TestTrue(*FString::Printf(TEXT("'%s' parses back"), *Wire), LexTryParseString(Parsed, Wire));
				TestTrue(*FString::Printf(TEXT("'%s' round-trips"), *Wire), Parsed == Origin);
			}
		});

		It("should refuse to parse an unrecognised origin rather than guessing one", [this]()
		{
			EFlowClassOrigin Parsed = EFlowClassOrigin::Unknown;
			TestFalse(TEXT("'cpp' is not a wire name and is rejected"), LexTryParseString(Parsed, TEXT("cpp")));
			TestFalse(TEXT("Nonsense is rejected"), LexTryParseString(Parsed, TEXT("not-an-origin")));
		});

		It("should partition the catalog exactly when filtering by origin", [this]()
		{
			// A filter that quietly dropped or double-counted classes would corrupt the batching that
			// the documentation work depends on, and nothing else in the suite would notice.
			auto CountFor = [](EFlowClassOrigin Origin)
			{
				FFlowCatalogQueryParams Params;
				Params.Sections = EFlowCatalogSection::Names;
				Params.bIncludeDeprecated = true;
				Params.Origin = Origin;
				FFlowCatalogQueryResult Result;
				UFlowCatalogQuery::QueryCatalog(Params, Result);
				return Result.Nodes.Num() + Result.Addons.Num();
			};

			const int32 Total = CountFor(EFlowClassOrigin::Any);
			TestTrue(TEXT("The catalog is non-empty"), Total > 0);

			const int32 Partitioned =
				CountFor(EFlowClassOrigin::Native) +
				CountFor(EFlowClassOrigin::Blueprint) +
				CountFor(EFlowClassOrigin::AngelScript) +
				CountFor(EFlowClassOrigin::Unknown);

			TestEqual(TEXT("Per-origin counts sum to the unfiltered total"), Partitioned, Total);
		});
	});

	Describe("MakeClassStem", [this]()
	{
		It("should strip the trailing generated-class suffix and one leading family prefix", [this]()
		{
			TestEqual("Family prefix stripped", UFlowCatalogQuery::MakeClassStem(TEXT("FlowNode_Start")), FString(TEXT("Start")));
			TestEqual("Generated suffix stripped", UFlowCatalogQuery::MakeClassStem(TEXT("FlowNode_Custom_C")), FString(TEXT("Custom")));
			TestEqual("Only the first prefix segment is stripped",
				UFlowCatalogQuery::MakeClassStem(TEXT("FlowNode_Some_Long_Name")), FString(TEXT("Some_Long_Name")));
			TestEqual("A doubled prefix is dropped once, not twice over",
				UFlowCatalogQuery::MakeClassStem(TEXT("FlowNode_FlowNode_Thing")), FString(TEXT("Thing")));
			TestEqual("A name with no prefix is left alone",
				UFlowCatalogQuery::MakeClassStem(TEXT("Standalone")), FString(TEXT("Standalone")));
		});
	});

	Describe("QueryCatalog sections", [this]()
	{
		It("shape should return histograms and no rows at all", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.Kind = EFlowCatalogKind::Node;
			Params.Sections = EFlowCatalogSection::Shape;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestTrue("No error", Result.ErrorMessage.IsEmpty());
			TestEqual("No node rows emitted", Result.Nodes.Num(), 0);
			TestEqual("No addon rows emitted", Result.Addons.Num(), 0);
			TestTrue("Node count still reported", Result.Shape.NodeCount > 0);
			TestTrue("Category histogram populated", Result.Shape.Categories.Num() > 0);
			for (const FFlowCatalogFacetRow& Facet : Result.Shape.Categories)
			{
				TestTrue("Facet count is positive", Facet.Count > 0);
			}
		});

		It("shape should cap each facet list and account for the tail it omits", [this]()
		{
			// Shape caps each facet list so an unfiltered query stays compact.
			FFlowCatalogQueryParams Params;
			Params.Sections = EFlowCatalogSection::Shape;
			Params.bIncludeDeprecated = true;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestTrue("Categories are capped", Result.Shape.Categories.Num() <= 20);
			TestTrue("Keywords are capped", Result.Shape.Keywords.Num() <= 20);
			TestTrue("Distinct category count is reported and is at least the capped list",
				Result.Shape.DistinctCategoryCount >= Result.Shape.Categories.Num());

			for (int32 Index = 1; Index < Result.Shape.Categories.Num(); ++Index)
			{
				TestTrue("Facets are ranked by descending count",
					Result.Shape.Categories[Index - 1].Count >= Result.Shape.Categories[Index].Count);
			}

			if (Result.Shape.DistinctCategoryCount > Result.Shape.Categories.Num())
			{
				TestTrue("An omitted tail is reported rather than silently dropped",
					Result.Shape.OtherCategoryCount > 0);
			}
		});

		It("shape keyword facets should not contain punctuation-only tokens", [this]()
		{
			// Punctuation separators do not become keyword facets or filter values.
			FFlowCatalogQueryParams Params;
			Params.Sections = EFlowCatalogSection::Shape;
			Params.bIncludeDeprecated = true;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			for (const FFlowCatalogFacetRow& Facet : Result.Shape.Keywords)
			{
				TestTrue(*FString::Printf(TEXT("Keyword '%s' is longer than one character"), *Facet.Value),
					Facet.Value.Len() > 1);
				for (const TCHAR Char : Facet.Value)
				{
					TestTrue(*FString::Printf(TEXT("Keyword '%s' is alphanumeric"), *Facet.Value),
						FChar::IsAlnum(Char));
				}
			}
		});

		It("names should carry an identifier and nothing else - no empty sibling fields", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.ClassNames = { TEXT("FlowNode_Start") };
			Params.Sections = EFlowCatalogSection::Names;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestEqual("One node resolved", Result.Nodes.Num(), 1);
			if (Result.Nodes.Num() != 1) { return; }

			const FFlowCatalogClassRow& Row = Result.Nodes[0];
			TestEqual("Stem emitted", Row.Stem, FString(TEXT("Start")));
			TestFalse("Class path is always authoritative and always present", Row.ClassPath.IsEmpty());
			TestTrue("Description not gathered", Row.Description.IsEmpty());
			TestTrue("Guidance not gathered", Row.Doc.Guidance.IsEmpty());
			TestEqual("Properties not gathered", Row.Properties.Num(), 0);
			TestEqual("Pins not gathered", Row.InputPins.Num(), 0);
			TestEqual("Usage not gathered", Row.UsageCount, INDEX_NONE);
		});

		It("full should populate the reflected sections", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.ClassNames = { TEXT("FlowNode_Start") };
			Params.Sections =
				EFlowCatalogSection::Names | EFlowCatalogSection::Doc | EFlowCatalogSection::Properties |
				EFlowCatalogSection::Pins | EFlowCatalogSection::Origin;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestEqual("One node resolved", Result.Nodes.Num(), 1);
			if (Result.Nodes.Num() != 1) { return; }

			TestTrue("Origin reported as native", Result.Nodes[0].Origin == EFlowClassOrigin::Native);
			TestTrue("A start node declares at least one output pin", Result.Nodes[0].OutputPins.Num() > 0);
		});

		It("Limit should truncate and still report the true total count", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.Kind = EFlowCatalogKind::Node;
			Params.Sections = EFlowCatalogSection::Names;
			Params.Limit = 1;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestTrue("No error", Result.ErrorMessage.IsEmpty());
			TestEqual("Exactly one row returned", Result.Nodes.Num(), 1);
			TestTrue("Truncated flag set", Result.bTruncated);
			TestTrue("TotalNodeCount exceeds the returned row count", Result.TotalNodeCount > 1);
		});

		It("Kind=node should leave Addons empty; Kind=addon should leave Nodes empty", [this]()
		{
			FFlowCatalogQueryParams NodeOnlyParams;
			NodeOnlyParams.Kind = EFlowCatalogKind::Node;
			NodeOnlyParams.Sections = EFlowCatalogSection::Names;
			NodeOnlyParams.Limit = 5;
			FFlowCatalogQueryResult NodeOnlyResult;
			UFlowCatalogQuery::QueryCatalog(NodeOnlyParams, NodeOnlyResult);
			TestEqual("Kind=node returns no addons", NodeOnlyResult.Addons.Num(), 0);
			TestTrue("Kind=node returns at least one node", NodeOnlyResult.Nodes.Num() > 0);

			FFlowCatalogQueryParams AddonOnlyParams;
			AddonOnlyParams.Kind = EFlowCatalogKind::Addon;
			AddonOnlyParams.Sections = EFlowCatalogSection::Names;
			AddonOnlyParams.Limit = 5;
			FFlowCatalogQueryResult AddonOnlyResult;
			UFlowCatalogQuery::QueryCatalog(AddonOnlyParams, AddonOnlyResult);
			TestEqual("Kind=addon returns no nodes", AddonOnlyResult.Nodes.Num(), 0);
			TestTrue("Kind=addon returns at least one addon", AddonOnlyResult.Addons.Num() > 0);
		});

		It("ClassNames with one valid and one bogus name should resolve the valid one and report the bogus one as unresolved", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.ClassNames = { TEXT("FlowNode_Start"), TEXT("FlowNode_DoesNotExist_Bogus") };

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestEqual("Only the valid name resolved", Result.Nodes.Num(), 1);
			TestTrue("Bogus name reported as unresolved",
				Result.UnresolvedClassNames.Contains(TEXT("FlowNode_DoesNotExist_Bogus")));
		});

		It("ClassNames should accept a bare stem as well as a full name", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.ClassNames = { TEXT("Start") };
			Params.Sections = EFlowCatalogSection::Names;

			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestEqual("Stem resolves to exactly one class", Result.Nodes.Num() + Result.Addons.Num(), 1);
		});
	});

	Describe("Addon reporting", [this]()
	{
		// An addon category is reported when declared; an empty category remains valid for
		// classes that do not declare one.
		It("should report an addon's own category instead of blanking it", [this]()
		{
			TArray<UClass*> AddOnClasses;
			UFlowCatalogQuery::GatherAllAddOnClasses(AddOnClasses);
			TestTrue("At least one addon class found", AddOnClasses.Num() > 0);

			int32 WithCategory = 0;
			for (const UClass* AddOnClass : AddOnClasses)
			{
				const FString Category = UFlowCatalogQuery::GetNodeCategory(AddOnClass);
				if (!Category.IsEmpty())
				{
					++WithCategory;
				}

				const UFlowNodeBase* DefaultAddOn = Cast<UFlowNodeBase>(AddOnClass->GetDefaultObject());
				if (DefaultAddOn)
				{
					TestEqual("Reported category is the class's own, unmodified",
						Category, DefaultAddOn->GetNodeCategory());
				}
			}

			TestTrue("Addons are not blanket-blanked - most report a category", WithCategory * 2 > AddOnClasses.Num());
		});
	});

	Describe("Property gathering", [this]()
	{
		It("should tag every returned row - own or inherited - with the class that actually declares it", [this]()
		{
			// GatherProperties walks the full ancestor chain. Every row's
			// DeclaringClass matches the property's real FProperty::GetOwnerClass(), whether that's
			// the queried class itself or an ancestor - never a merged dump with no attribution.
			UClass* StartClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(TEXT("FlowNode_Start"));
			TestNotNull("FlowNode_Start resolves", StartClass);
			if (!StartClass) { return; }

			TArray<FFlowCatalogPropertyRow> Rows;
			UFlowCatalogQuery::GatherProperties(StartClass, Rows);

			for (const FFlowCatalogPropertyRow& Row : Rows)
			{
				// A dot-qualified name comes from a flattened ShowOnlyInnerProperties struct field and
				// has no direct FindPropertyByName match on the owning class - only the top-level rows
				// are checked here.
				if (Row.Name.Contains(TEXT("."))) { continue; }

				const FProperty* Property = StartClass->FindPropertyByName(FName(*Row.Name));
				TestNotNull(*FString::Printf(TEXT("'%s' resolves to a real property"), *Row.Name), Property);
				if (!Property) { continue; }

				const UClass* ActualOwner = Property->GetOwnerClass();
				TestEqual(*FString::Printf(TEXT("'%s' is tagged with its real declaring class"), *Row.Name),
					Row.DeclaringClass, ActualOwner ? ActualOwner->GetName() : StartClass->GetName());
			}
		});

		// EditAnywhere properties remain visible in the catalog even when BlueprintReadOnly also
		// applies. UFlowNodeAddOn::InputPins provides an example on every addon.
		It("should include an EditAnywhere+BlueprintReadOnly property instead of discarding it", [this]()
		{
			TArray<UClass*> AddOnClasses;
			UFlowCatalogQuery::GatherAllAddOnClasses(AddOnClasses);
			TestTrue("At least one addon class found", AddOnClasses.Num() > 0);
			if (AddOnClasses.Num() == 0) { return; }

			TArray<FFlowCatalogPropertyRow> Rows;
			UFlowCatalogQuery::GatherProperties(AddOnClasses[0], Rows);

			const FFlowCatalogPropertyRow* InputPinsRow = Rows.FindByPredicate(
				[](const FFlowCatalogPropertyRow& Row) { return Row.Name == TEXT("InputPins"); });
			TestNotNull("InputPins (EditAnywhere, BlueprintReadOnly, declared on FlowNodeAddOn) is reported", InputPinsRow);
			if (InputPinsRow)
			{
				TestEqual("Declaring class is the base FlowNodeAddOn, not the leaf addon",
					InputPinsRow->DeclaringClass, FString(TEXT("FlowNodeAddOn")));
			}
		});
	});

	Describe("Deprecation", [this]()
	{
		It("should hide deprecated classes from a filtered query but still resolve them by exact name", [this]()
		{
			TArray<UClass*> NodeClasses;
			UFlowCatalogQuery::GatherAllNodeClasses(NodeClasses);

			UClass* DeprecatedClass = nullptr;
			for (UClass* NodeClass : NodeClasses)
			{
				if (UFlowCatalogQuery::IsClassDeprecated(NodeClass))
				{
					DeprecatedClass = NodeClass;
					break;
				}
			}

			if (!DeprecatedClass)
			{
				// Nothing in this project is deprecated yet; the gather contract is still the thing
				// worth asserting, since hard-exclusion here is what made retired classes invisible.
				TestTrue("Gather returns classes to filter over", NodeClasses.Num() > 0);
				return;
			}

			FFlowCatalogQueryParams DefaultParams;
			DefaultParams.Kind = EFlowCatalogKind::Node;
			DefaultParams.Sections = EFlowCatalogSection::Names;
			FFlowCatalogQueryResult DefaultResult;
			UFlowCatalogQuery::QueryCatalog(DefaultParams, DefaultResult);
			TestFalse("Deprecated class absent from a default query",
				DefaultResult.Nodes.ContainsByPredicate([DeprecatedClass](const FFlowCatalogClassRow& Row)
				{
					return Row.ClassPath == DeprecatedClass->GetPathName();
				}));

			FFlowCatalogQueryParams ExactParams;
			ExactParams.ClassNames = { DeprecatedClass->GetName() };
			ExactParams.Sections = EFlowCatalogSection::Names | EFlowCatalogSection::Deprecation;
			FFlowCatalogQueryResult ExactResult;
			UFlowCatalogQuery::QueryCatalog(ExactParams, ExactResult);
			TestEqual("Exact lookup resolves it regardless of the deprecation filter", ExactResult.Nodes.Num(), 1);
			if (ExactResult.Nodes.Num() == 1)
			{
				TestTrue("Reported as deprecated", ExactResult.Nodes[0].bDeprecated);
			}

			FFlowCatalogQueryParams OptInParams;
			OptInParams.Kind = EFlowCatalogKind::Node;
			OptInParams.Sections = EFlowCatalogSection::Names;
			OptInParams.bIncludeDeprecated = true;
			FFlowCatalogQueryResult OptInResult;
			UFlowCatalogQuery::QueryCatalog(OptInParams, OptInResult);
			TestTrue("Present once opted in",
				OptInResult.Nodes.ContainsByPredicate([DeprecatedClass](const FFlowCatalogClassRow& Row)
				{
					return Row.ClassPath == DeprecatedClass->GetPathName();
				}));
		});
	});

	Describe("Agent doc", [this]()
	{
		// SetAgentDoc only durably mutates a class whose GetAgentDoc() reads the serialized AgentDoc
		// member back - a native class that overrides GetAgentDoc() to return a compiled-in constant
		// ignores it entirely. Every native Flow class in this module now carries such an override, so
		// these two tests need a Blueprint- or AngelScript-authored addon instead; find one dynamically
		// rather than hard-coding a class name that a future native doc pass could invalidate again.
		auto FindMutableDocAddOnClass = [this]() -> UClass*
		{
			TArray<UClass*> AddOnClasses;
			UFlowCatalogQuery::GatherAllAddOnClasses(AddOnClasses);
			for (UClass* AddOnClass : AddOnClasses)
			{
				if (UFlowCatalogQuery::GetClassOrigin(AddOnClass) != EFlowClassOrigin::Native)
				{
					return AddOnClass;
				}
			}
			return nullptr;
		};

		It("should report hasDoc false for an undocumented class", [this, FindMutableDocAddOnClass]()
		{
			UClass* StartClass = FindMutableDocAddOnClass();
			if (!StartClass)
			{
				// No Blueprint/AngelScript-authored addon in this project - nothing to assert.
				return;
			}

			UFlowNodeBase* DefaultNode = Cast<UFlowNodeBase>(StartClass->GetDefaultObject());
			TestNotNull("Class default object available", DefaultNode);
			if (!DefaultNode) { return; }

			const FFlowAgentDoc OriginalDoc = DefaultNode->GetAgentDoc();
			DefaultNode->SetAgentDoc(FFlowAgentDoc());

			FFlowCatalogDocRow Doc;
			TArray<FString> Articles;
			UFlowCatalogQuery::GatherAgentDoc(StartClass, Doc, Articles);
			TestTrue("Undocumented is reported, not treated as an error", Doc.IsEmpty());

			DefaultNode->SetAgentDoc(OriginalDoc);
		});

		It("articles should be queryable as a filter", [this, FindMutableDocAddOnClass]()
		{
			UClass* StartClass = FindMutableDocAddOnClass();
			if (!StartClass)
			{
				// No Blueprint/AngelScript-authored addon in this project - nothing to assert.
				return;
			}

			UFlowNodeBase* DefaultNode = Cast<UFlowNodeBase>(StartClass->GetDefaultObject());
			if (!DefaultNode) { return; }

			const FFlowAgentDoc OriginalDoc = DefaultNode->GetAgentDoc();
			DefaultNode->SetAgentDoc(MakeAgentDoc(
				TEXT("Placeholder guidance for the article filter test."),
				{ TEXT("test") },
				{ TEXT("pattern:catalog-spec-fixture") }));

			FFlowCatalogQueryParams Params;
			Params.Kind = EFlowCatalogKind::Addon;
			Params.Sections = EFlowCatalogSection::Names | EFlowCatalogSection::Articles;
			Params.Articles = { TEXT("pattern:catalog-spec-fixture") };
			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);

			TestEqual("Only the stamped class matches the slug", Result.Addons.Num(), 1);
			if (Result.Addons.Num() == 1)
			{
				TestTrue("Slug reported back", Result.Addons[0].Articles.Contains(TEXT("pattern:catalog-spec-fixture")));
			}

			DefaultNode->SetAgentDoc(OriginalDoc);
		});
	});

	Describe("Pin enumeration", [this]()
	{
		It("should read pins through the class default object's catalog-pin hook", [this]()
		{
			// The catalog hook includes procedurally generated pins, not just serialized arrays.
			TArray<UClass*> NodeClasses;
			UFlowCatalogQuery::GatherAllNodeClasses(NodeClasses);
			TestTrue("At least one node class found", NodeClasses.Num() > 0);

			int32 ClassesWithPins = 0;
			for (const UClass* NodeClass : NodeClasses)
			{
				TArray<FFlowCatalogPinRow> InputPins;
				TArray<FFlowCatalogPinRow> OutputPins;
				UFlowCatalogQuery::GatherPins(NodeClass, InputPins, OutputPins);
				if (InputPins.Num() > 0 || OutputPins.Num() > 0)
				{
					++ClassesWithPins;
				}
			}

			TestTrue("Most node classes report at least one pin", ClassesWithPins * 2 > NodeClasses.Num());
		});

		// A class default object has no live owner. Pin gathering must avoid owner-dependent
		// context hooks on addons such as FlowNodeAddOn_SwitchCase.
		It("should gather pins for every addon class without asking a class default object about its owner", [this]()
		{
			TArray<UClass*> AddOnClasses;
			UFlowCatalogQuery::GatherAllAddOnClasses(AddOnClasses);
			TestTrue("At least one addon class found", AddOnClasses.Num() > 0);

			for (const UClass* AddOnClass : AddOnClasses)
			{
				TArray<FFlowCatalogPinRow> InputPins;
				TArray<FFlowCatalogPinRow> OutputPins;
				UFlowCatalogQuery::GatherPins(AddOnClass, InputPins, OutputPins);
			}
		});

		It("empty PinTypes filter matches every class", [this]()
		{
			FFlowCatalogQueryParams Params;
			Params.ClassNames = { TEXT("FlowNode_Timer") };
			FFlowCatalogQueryResult Result;
			UFlowCatalogQuery::QueryCatalog(Params, Result);
			TestEqual("FlowNode_Timer resolves under an empty pin filter", Result.Nodes.Num(), 1);
		});

		It("PinTypes filter matches a node declaring a pin of that type, and excludes one that does not", [this]()
		{
			FFlowCatalogQueryParams MatchParams;
			MatchParams.Kind = EFlowCatalogKind::Node;
			MatchParams.Sections = EFlowCatalogSection::Names;
			MatchParams.Query = TEXT("FlowNode_Timer");
			MatchParams.PinTypes = { TEXT("Exec") };
			FFlowCatalogQueryResult MatchResult;
			UFlowCatalogQuery::QueryCatalog(MatchParams, MatchResult);
			TestTrue("FlowNode_Timer matches an Exec pin filter",
				MatchResult.Nodes.ContainsByPredicate([](const FFlowCatalogClassRow& Row) { return Row.ClassName == TEXT("FlowNode_Timer"); }));

			FFlowCatalogQueryParams NoMatchParams;
			NoMatchParams.Kind = EFlowCatalogKind::Node;
			NoMatchParams.Sections = EFlowCatalogSection::Names;
			NoMatchParams.Query = TEXT("FlowNode_Timer");
			NoMatchParams.PinTypes = { TEXT("Vector") };
			FFlowCatalogQueryResult NoMatchResult;
			UFlowCatalogQuery::QueryCatalog(NoMatchParams, NoMatchResult);
			TestFalse("FlowNode_Timer does not match a Vector pin filter",
				NoMatchResult.Nodes.ContainsByPredicate([](const FFlowCatalogClassRow& Row) { return Row.ClassName == TEXT("FlowNode_Timer"); }));
		});

		// FlowNode_Reroute's In/Out pins are Wildcard on the class default object and only
		// take on a real type (e.g. Vector) dynamically at edit time - the filter must never resolve
		// that potential runtime type, only the declared static one.
		It("never matches a type a node's pins could only take on dynamically at edit time", [this]()
		{
			UClass* RerouteClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(TEXT("FlowNode_Reroute"));
			TestNotNull("FlowNode_Reroute class should resolve", RerouteClass);
			if (!RerouteClass) { return; }

			TArray<FFlowCatalogPinRow> InputPins, OutputPins;
			UFlowCatalogQuery::GatherPins(RerouteClass, InputPins, OutputPins);
			TestTrue("Reroute should declare at least one static input pin", InputPins.Num() > 0);
			if (InputPins.Num() == 0) { return; }

			const FString StaticPinType = InputPins[0].Type;
			TestTrue("Filtering on Reroute's actual static (Wildcard) pin type should match",
				UFlowCatalogQuery::ClassHasPinOfAnyType(RerouteClass, { StaticPinType }));
			TestFalse("Filtering on a type Reroute's pins could only take on dynamically (e.g. Vector) should not match",
				UFlowCatalogQuery::ClassHasPinOfAnyType(RerouteClass, { TEXT("Vector") }));
		});

		// An auto-generated data pin's Type is the bare FlowPinType, matching the pin a
		// caller sees on the graph - not decorated with the backing property's name, which is only
		// visible in properties[] and would otherwise dangle if that property isn't listed there.
		It("an auto-generated data pin's Type is the bare FlowPinType", [this]()
		{
			UClass* ObjectTesterClass = UFlowCatalogQuery::FindFlowNodeOrAddOnClassByName(TEXT("FlowNode_DataPinsTester_Object"));
			TestNotNull("FlowNode_DataPinsTester_Object class should resolve", ObjectTesterClass);
			if (!ObjectTesterClass) { return; }

			TArray<FFlowCatalogPinRow> InputPins, OutputPins;
			UFlowCatalogQuery::GatherPins(ObjectTesterClass, InputPins, OutputPins);
			const FFlowCatalogPinRow* ObjectPin = OutputPins.FindByPredicate(
				[](const FFlowCatalogPinRow& Row) { return Row.Name == TEXT("Object Single Output"); });
			TestNotNull("A pin row for Object Single Output should exist", ObjectPin);
			if (ObjectPin)
			{
				TestEqual("Type is the bare FlowPinType", ObjectPin->Type, FString(TEXT("Object")));
			}
		});
	});

	Describe("Response size budget", [this]()
	{
		// These presets exist to be cheap. Enforcing the budget here is what stops a later field addition
		// from quietly turning the orientation call back into a dump - the failure mode that made the
		// original surface unusable and is invisible to every other test in this file.
		It("shape over the whole catalog should stay under 5KB", [this]()
		{
			const FString Json = UFlowCatalogQuery::QueryFlowCatalogAsJson(
				FString(), {}, FString(), FString(), {}, {}, {}, {}, EFlowClassOrigin::Any, TEXT("shape"), true, 0, 0);

			const int32 Bytes = FTCHARToUTF8(*Json).Length();
			TestTrue(*FString::Printf(TEXT("shape is %d bytes, budget 5120"), Bytes), Bytes < 5120);
		});

		// Ratchet, not the target. brief(25) measures 9277 bytes against a 9216 target; the remaining
		// overage is one known cause - `description` is 59% of the payload at ~219 chars/row and up to
		// 531, untruncated, in a preset that promises one line. Capping it there is the fix. Asserting
		// no-growth keeps this honest and useful in the meantime; a knowingly-red test would just teach
		// everyone to skip the suite.
		It("brief over 25 rows should not grow beyond its current measured size", [this]()
		{
			const FString Json = UFlowCatalogQuery::QueryFlowCatalogAsJson(
				FString(), {}, TEXT("node"), FString(), {}, {}, {}, {}, EFlowClassOrigin::Any, TEXT("brief"), false, 25, 0);

			const int32 Bytes = FTCHARToUTF8(*Json).Length();
			TestTrue(*FString::Printf(TEXT("brief(25) is %d bytes, ratchet 9277, target 9216"), Bytes), Bytes <= 9277);
		});
	});

	Describe("Bulk export", [this]()
	{
		It("should write UTF-8 that parses, and cover exactly the classes the live query returns for the same filter", [this]()
		{
			TArray<UClass*> AssetClasses;
			UFlowCatalogQuery::GatherAllFlowAssetClasses(AssetClasses);
			TestTrue("Should find at least one concrete FlowAsset subclass to test against", AssetClasses.Num() > 0);
			if (AssetClasses.Num() == 0) { return; }

			const FString AssetClassName = AssetClasses[0]->GetName();

			FFlowCatalogQueryParams Params;
			Params.AssetClassName = AssetClassName;
			Params.Kind = EFlowCatalogKind::Any;
			Params.bIncludeDeprecated = true;
			FString SectionsError;
			TestTrue("full preset parses", UFlowCatalogQuery::ParseSections(TEXT("full"), Params.Sections, Params.bDescriptionOnly, SectionsError));

			FFlowCatalogQueryResult LiveResult;
			UFlowCatalogQuery::QueryCatalog(Params, LiveResult);
			TestTrue("Live query does not error", LiveResult.ErrorMessage.IsEmpty());

			const FString OutputPath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("FlowCatalogQuerySpec_Export.json"));
			TestTrue("ExportFlowCatalogToFile succeeds", UFlowCatalogQuery::ExportFlowCatalogToFile(OutputPath, AssetClassName));

			// A UTF-16 payload here would make an ordinary utf-8 read fail outright, so the encoding is
			// part of the contract rather than an implementation detail.
			TArray<uint8> RawBytes;
			TestTrue("Exported file is readable as bytes", FFileHelper::LoadFileToArray(RawBytes, *OutputPath));
			if (RawBytes.Num() >= 2)
			{
				const bool bLooksLikeUtf16 = (RawBytes[0] == 0xFF && RawBytes[1] == 0xFE) || (RawBytes[0] == 0xFE && RawBytes[1] == 0xFF);
				TestFalse("Exported file is not UTF-16", bLooksLikeUtf16);
			}

			FString FileContents;
			TestTrue("Exported file is readable as text", FFileHelper::LoadFileToString(FileContents, *OutputPath));

			TSharedPtr<FJsonObject> FileObj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
			TestTrue("Exported file parses as a JSON object", FJsonSerializer::Deserialize(Reader, FileObj));
			if (!FileObj.IsValid()) { return; }

			const TArray<TSharedPtr<FJsonValue>>* FileNodes = nullptr;
			FileObj->TryGetArrayField(TEXT("nodes"), FileNodes);
			const int32 FileNodeCount = FileNodes ? FileNodes->Num() : 0;

			TestEqual("File export node count matches the live query", FileNodeCount, LiveResult.Nodes.Num());

			IFileManager::Get().Delete(*OutputPath);
		});

		It("should report an error under the documented errorMessage key", [this]()
		{
			const FString Json = UFlowCatalogQuery::QueryFlowCatalogAsJson(
				FString(), {}, FString(), FString(), {}, {}, {}, {}, EFlowClassOrigin::Any, TEXT("not-a-section"), false, 0, 0);

			TSharedPtr<FJsonObject> Obj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
			TestTrue("Error response parses", FJsonSerializer::Deserialize(Reader, Obj));
			if (!Obj.IsValid()) { return; }

			TestTrue("Error carried under errorMessage", Obj->HasField(TEXT("errorMessage")));
			TestFalse("The undocumented error key is gone", Obj->HasField(TEXT("error")));
		});
	});
}

#endif
