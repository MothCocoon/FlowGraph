// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphValidation.h"

#include "FlowGraphImporter.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Policies/FlowPinConnectionPolicy.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

void FFlowGraphValidation::AddFinding(
	TArray<FFlowValidationFinding>& OutFindings,
	EFlowValidationSeverity Severity,
	const FString& Code,
	const FString& Message,
	const FGuid& NodeGuid,
	const FGuid& AddOnGuid,
	FName PinName)
{
	FFlowValidationFinding& Finding = OutFindings.AddDefaulted_GetRef();
	Finding.Severity = Severity;
	Finding.Code = Code;
	Finding.Message = Message;
	Finding.NodeGuid = NodeGuid;
	Finding.AddOnGuid = AddOnGuid;
	Finding.PinName = PinName;
}

// Meyers-singleton registry storage - avoids static-initialization-order issues with a
// module-level global. Keyed by the exact UClass RegisterHook was called with.
static TMap<TSubclassOf<UFlowAsset>, TArray<FFlowValidationHookDelegate>>& GetHookRegistry()
{
	static TMap<TSubclassOf<UFlowAsset>, TArray<FFlowValidationHookDelegate>> Registry;
	return Registry;
}

void FFlowGraphValidation::RegisterHook(TSubclassOf<UFlowAsset> AssetClass, FFlowValidationHookDelegate Hook)
{
	if (!AssetClass || !Hook.IsBound())
	{
		return;
	}
	GetHookRegistry().FindOrAdd(AssetClass).Add(MoveTemp(Hook));
}

void FFlowGraphValidation::UnregisterHooksForClass(TSubclassOf<UFlowAsset> AssetClass)
{
	GetHookRegistry().Remove(AssetClass);
}

static bool HasPinNamed(const TArray<FFlowPin>& Pins, FName PinName)
{
	for (const FFlowPin& Pin : Pins)
	{
		if (Pin.PinName == PinName)
		{
			return true;
		}
	}
	return false;
}

// Returns the pin type name (e.g. "Exec", "Text") for the pin named PinName, or NAME_None if absent.
static FName FindPinTypeName(const TArray<FFlowPin>& Pins, FName PinName)
{
	for (const FFlowPin& Pin : Pins)
	{
		if (Pin.PinName == PinName)
		{
			return Pin.GetPinTypeName().Name;
		}
	}
	return NAME_None;
}

static bool HasNodePin(const UFlowNode& Node, FName PinName, bool bIsOutputPin)
{
	const TArray<FFlowPin>& StaticPins = bIsOutputPin ? Node.GetOutputPins() : Node.GetInputPins();
	const TArray<FFlowPin> ContextPins = bIsOutputPin ? Node.GetContextOutputs() : Node.GetContextInputs();
	return HasPinNamed(StaticPins, PinName) || HasPinNamed(ContextPins, PinName);
}

bool FFlowGraphValidation::DoesNodeHavePin(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const FGuid& NodeGuid,
	FName PinName,
	bool bIsOutputPin)
{
	const FFlowGraphParsedNode* ParsedNode = ParsedNodes.FindByPredicate([&NodeGuid](const FFlowGraphParsedNode& Node)
	{
		return !Node.bIsDeleteMarker && Node.NodeGuid == NodeGuid;
	});

	if (TargetAsset)
	{
		if (const UFlowNode* Node = TargetAsset->GetNode(NodeGuid);
			IsValid(Node) && HasNodePin(*Node, PinName, bIsOutputPin))
		{
			return true;
		}
	}

	if (ParsedNode)
	{
		// Declarations are only claims about the resulting schema. They must be checked against the
		// live node or its class CDO before a connection can use them; trusting the declaration here
		// lets a phantom pin look wired while no runtime pin can execute it.
		const TArray<FFlowPin>* RuntimePins = nullptr;
		bool bHasGeneratedPin = false;
		if (TargetAsset)
		{
			if (const UFlowNode* ExistingNode = TargetAsset->GetNode(NodeGuid))
			{
				RuntimePins = &(bIsOutputPin ? ExistingNode->GetOutputPins() : ExistingNode->GetInputPins());
			}
		}
		if (!RuntimePins && !ParsedNode->NodeType.IsEmpty())
		{
			UClass* NodeClass = UFlowGraphImporter::ResolveNodeClass(ParsedNode->NodeType);
			const UFlowNode* NodeCDO = (NodeClass && NodeClass->IsChildOf(UFlowNode::StaticClass()))
				? NodeClass->GetDefaultObject<UFlowNode>()
				: nullptr;
			if (NodeCDO)
			{
				RuntimePins = &(bIsOutputPin ? NodeCDO->GetOutputPins() : NodeCDO->GetInputPins());
				if (!HasPinNamed(*RuntimePins, PinName) && !NodeClass->HasAnyClassFlags(CLASS_Abstract))
				{
					UFlowNode* Preview = NewObject<UFlowNode>(GetTransientPackage(), NodeClass, NAME_None, RF_Transient);
					if (IsValid(Preview))
					{
						Preview->TryUpdateAutoDataPins();
						bHasGeneratedPin = HasNodePin(*Preview, PinName, bIsOutputPin);
					}
				}
			}
		}

		if (!RuntimePins)
		{
			return false;
		}

		const TArray<FString>& DeclaredPins = bIsOutputPin ? ParsedNode->OutputPins : ParsedNode->InputPins;
		for (const FString& PinDecl : DeclaredPins)
		{
			FString DeclaredName, DeclaredType;
			const FString TrimmedDecl = PinDecl.TrimStartAndEnd();
			if (TrimmedDecl.Split(TEXT(" ["), &DeclaredName, &DeclaredType))
			{
				if (FName(*DeclaredName.TrimStartAndEnd()) == PinName)
				{
					return HasPinNamed(*RuntimePins, PinName) || bHasGeneratedPin;
				}
			}
			else if (FName(*TrimmedDecl) == PinName)
			{
				return HasPinNamed(*RuntimePins, PinName) || bHasGeneratedPin;
			}
		}

		return HasPinNamed(*RuntimePins, PinName) || bHasGeneratedPin;
	}

	if (TargetAsset)
	{
		if (const UFlowNode* ExistingNode = TargetAsset->GetNode(NodeGuid))
		{
			return HasPinNamed(bIsOutputPin ? ExistingNode->GetOutputPins() : ExistingNode->GetInputPins(), PinName);
		}
	}

	return false;
}

FName FFlowGraphValidation::GetConnectionPinType(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const FGuid& NodeGuid,
	FName PinName,
	bool bIsOutputPin)
{
	const FFlowGraphParsedNode* ParsedNode = ParsedNodes.FindByPredicate([&NodeGuid](const FFlowGraphParsedNode& Node)
	{
		return !Node.bIsDeleteMarker && Node.NodeGuid == NodeGuid;
	});

	if (TargetAsset)
	{
		if (const UFlowNode* Node = TargetAsset->GetNode(NodeGuid); IsValid(Node))
		{
			const FName PinType = FindPinTypeName(
				bIsOutputPin ? Node->GetContextOutputs() : Node->GetContextInputs(), PinName);
			if (!PinType.IsNone())
			{
				return PinType;
			}
		}
	}

	if (ParsedNode)
	{
		// The document's own declared pins take priority, matching DoesNodeHavePin's order.
		const TArray<FString>& DeclaredPins = bIsOutputPin ? ParsedNode->OutputPins : ParsedNode->InputPins;
		for (const FString& PinDecl : DeclaredPins)
		{
			FName DeclaredName;
			FString DeclaredType;
			FString DeclaredSubCategoryPath;
			if (UFlowGraphImporter::ParsePinDecl(PinDecl, DeclaredName, DeclaredType, DeclaredSubCategoryPath) &&
				DeclaredName == PinName)
			{
				return DeclaredType.IsEmpty() ? NAME_None : FName(*DeclaredType);
			}
		}

		if (!ParsedNode->NodeType.IsEmpty())
		{
			// A Type: was declared - resolve pins from the class CDO (dynamic pins aren't resolvable
			// without a live instance, which doesn't exist yet for a node this document creates).
			UClass* NodeClass = UFlowGraphImporter::ResolveNodeClass(ParsedNode->NodeType);
			const UFlowNode* NodeCDO = (NodeClass && NodeClass->IsChildOf(UFlowNode::StaticClass())) ? NodeClass->GetDefaultObject<UFlowNode>() : nullptr;
			return NodeCDO ? FindPinTypeName(bIsOutputPin ? NodeCDO->GetOutputPins() : NodeCDO->GetInputPins(), PinName) : NAME_None;
		}

		// A pure merge update - fall through to the real existing node below.
	}

	if (TargetAsset)
	{
		if (const UFlowNode* ExistingNode = TargetAsset->GetNode(NodeGuid))
		{
			return FindPinTypeName(bIsOutputPin ? ExistingNode->GetOutputPins() : ExistingNode->GetInputPins(), PinName);
		}
	}

	return NAME_None;
}

bool FFlowGraphValidation::HasNoErrors(const TArray<FFlowValidationFinding>& Findings)
{
	for (const FFlowValidationFinding& Finding : Findings)
	{
		if (Finding.Severity == EFlowValidationSeverity::Error)
		{
			return false;
		}
	}
	return true;
}

void FFlowGraphValidation::ValidateAddOnsRecursive(
	const UFlowAsset* TargetAsset,
	const UClass* OwnerClass,
	const FGuid& OwnerNodeGuid,
	const TArray<FFlowGraphParsedNodeAddOn>& ParsedAddOns,
	TArray<FFlowValidationFinding>& OutFindings)
{
	// GUID uniqueness is scoped per owner: two sibling addons sharing a GUID is an error;
	// the same GUID under a different owner is fine.
	TSet<FGuid> SeenAddOnGuids;

	for (const FFlowGraphParsedNodeAddOn& ParsedAddOn : ParsedAddOns)
	{
		if (ParsedAddOn.bIsDeleteMarker)
		{
			// A deletion can't violate palette/class/attachment rules.
			continue;
		}

		if (ParsedAddOn.AddOnGuid.IsValid())
		{
			bool bAlreadySeen = false;
			SeenAddOnGuids.Add(ParsedAddOn.AddOnGuid, &bAlreadySeen);
			if (bAlreadySeen)
			{
				AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("DuplicateAddOnGuid"),
					FString::Printf(TEXT("Duplicate AddOn GUID under the same owner node: %s"), *ParsedAddOn.AddOnGuid.ToString()),
					OwnerNodeGuid, ParsedAddOn.AddOnGuid);
			}
		}

		// An existing addon update may omit Type because its live class is already authoritative.
		// New aliases are required to provide Type by converter validation, so an empty type here is
		// unambiguously a merge update and should not be treated as an unresolved class.
		if (ParsedAddOn.AddOnType.IsEmpty())
		{
			continue;
		}

		// Resolve without the placement check so this validator can name the real defect. The
		// fail-closed ResolveNodeClass returns nullptr for a non-Flow Blueprint class, which would
		// otherwise surface as ClassNotResolved - telling the author the type does not exist, when in
		// fact it exists and merely needs recreating through the right factory.
		UClass* AddOnClass = UFlowGraphImporter::ResolveNodeClassForQuery(ParsedAddOn.AddOnType);

		FString AddOnNonFlowBlueprintReason;
		if (UFlowGraphImporter::IsClassFromNonFlowBlueprint(AddOnClass, AddOnNonFlowBlueprintReason))
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassFromNonFlowBlueprint"),
				AddOnNonFlowBlueprintReason,
				OwnerNodeGuid, ParsedAddOn.AddOnGuid);
			continue;
		}

		if (!AddOnClass || !AddOnClass->IsChildOf(UFlowNodeAddOn::StaticClass()))
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassNotResolved"),
				FString::Printf(TEXT("AddOn class does not resolve to a UFlowNodeAddOn: '%s'"), *ParsedAddOn.AddOnType),
				OwnerNodeGuid, ParsedAddOn.AddOnGuid);
			// Can't do palette/recursion without a resolved class.
			continue;
		}

		// Surface abstract classes as a clean dry-run finding
		// instead of letting them reach NewObject and crash the editor session.
		if (AddOnClass->HasAnyClassFlags(CLASS_Abstract))
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassIsAbstract"),
				FString::Printf(TEXT("AddOn class is abstract and cannot be instantiated: '%s'"), *ParsedAddOn.AddOnType),
				OwnerNodeGuid, ParsedAddOn.AddOnGuid);
			continue;
		}

		if (TargetAsset && !TargetAsset->IsNodeOrAddOnClassAllowed(AddOnClass))
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassNotAllowed"),
				FString::Printf(TEXT("AddOn class '%s' is not allowed in asset class '%s'"),
					*AddOnClass->GetName(), *TargetAsset->GetClass()->GetName()),
				OwnerNodeGuid, ParsedAddOn.AddOnGuid);
		}

#if WITH_EDITOR
		// Attachment eligibility check. OwnerClass is null when a merge-update block omits Type: -
		// skip in that case, as the addon attaches to the real existing node unchanged.
		// CheckAcceptFlowNodeAddOnChild is queried on class-default objects. Only Reject fails;
		// Undetermined/TentativeAccept both pass.
		if (OwnerClass)
		{
			const UFlowNodeBase* ParentCDO = OwnerClass->GetDefaultObject<UFlowNodeBase>();
			const UFlowNodeAddOn* AddOnCDO = AddOnClass->GetDefaultObject<UFlowNodeAddOn>();
			if (ParentCDO && AddOnCDO)
			{
				const EFlowAddOnAcceptResult AcceptResult =
					ParentCDO->CheckAcceptFlowNodeAddOnChild(AddOnCDO, TArray<UFlowNodeAddOn*>());
				if (AcceptResult == EFlowAddOnAcceptResult::Reject)
				{
					AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("AddonNotAttachable"),
						FString::Printf(TEXT("AddOn '%s' cannot attach to '%s' (attachment rejected)"),
							*AddOnClass->GetName(), *OwnerClass->GetName()),
						OwnerNodeGuid, ParsedAddOn.AddOnGuid);
				}
			}
		}
#endif // WITH_EDITOR

		// Recurse into addon-of-addon children, scoping GUID uniqueness to that owner.
		ValidateAddOnsRecursive(TargetAsset, AddOnClass, OwnerNodeGuid, ParsedAddOn.AddOns, OutFindings);
	}
}

bool FFlowGraphValidation::ValidateDocument(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	TArray<FFlowValidationFinding>& OutFindings,
	const TSet<FGuid>* PostPlanNodeGuids)
{
	ValidateNodes(TargetAsset, ParsedNodes, OutFindings);
	ValidateConnections(TargetAsset, ParsedNodes, ParsedConnections, OutFindings, PostPlanNodeGuids);
	RunValidationHooks(TargetAsset, ParsedNodes, ParsedConnections, OutFindings);

	return HasNoErrors(OutFindings);
}

void FFlowGraphValidation::ValidateNodes(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	TArray<FFlowValidationFinding>& OutFindings)
{
	TSet<FGuid> SeenNodeGuids;
	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		if (ParsedNode.bIsDeleteMarker)
		{
			continue;
		}

		if (ParsedNode.NodeGuid.IsValid())
		{
			bool bAlreadySeen = false;
			SeenNodeGuids.Add(ParsedNode.NodeGuid, &bAlreadySeen);
			if (bAlreadySeen)
			{
				AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("DuplicateNodeGuid"),
					FString::Printf(TEXT("Duplicate node GUID in document: %s"), *ParsedNode.NodeGuid.ToString()),
					ParsedNode.NodeGuid);
			}
		}

		UClass* NodeClass = nullptr;
		if (!ParsedNode.NodeType.IsEmpty())
		{
			// Resolve without the placement check so this validator can name the real defect - see the
			// matching note in ValidateAddOnsRecursive.
			NodeClass = UFlowGraphImporter::ResolveNodeClassForQuery(ParsedNode.NodeType);

			FString NodeNonFlowBlueprintReason;
			if (UFlowGraphImporter::IsClassFromNonFlowBlueprint(NodeClass, NodeNonFlowBlueprintReason))
			{
				AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassFromNonFlowBlueprint"),
					NodeNonFlowBlueprintReason,
					ParsedNode.NodeGuid);

				// Match what the fail-closed resolver would have handed back, so every downstream
				// pin and palette check behaves exactly as it does on the apply path.
				NodeClass = nullptr;
			}
			else if (!NodeClass || !NodeClass->IsChildOf(UFlowNode::StaticClass()))
			{
				AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassNotResolved"),
					FString::Printf(TEXT("Node class does not resolve to a UFlowNode: '%s'"), *ParsedNode.NodeType),
					ParsedNode.NodeGuid);
			}
			else if (NodeClass->HasAnyClassFlags(CLASS_Abstract))
			{
				// Surface abstract classes as a clean dry-run finding
				// instead of letting them reach NewObject and crash the editor session.
				AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassIsAbstract"),
					FString::Printf(TEXT("Node class is abstract and cannot be instantiated: '%s'"), *ParsedNode.NodeType),
					ParsedNode.NodeGuid);
			}
			else if (TargetAsset && !TargetAsset->IsNodeOrAddOnClassAllowed(NodeClass))
			{
				AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassNotAllowed"),
					FString::Printf(TEXT("Node class '%s' is not allowed in asset class '%s'"),
						*NodeClass->GetName(), *TargetAsset->GetClass()->GetName()),
					ParsedNode.NodeGuid);
			}
			else if (TargetAsset && !ParsedNode.bIsNewAlias)
			{
				// ClassMismatch: an UpsertNode op's type is only legal on an existing node when it
				// matches that node's actual class - the reconciler cannot swap a node's class
				// while preserving its GUID, so a mismatch must be a hard error rather than a
				// silently ignored field.
				if (const UFlowNode* ExistingNode = TargetAsset->GetNode(ParsedNode.NodeGuid))
				{
					if (ExistingNode->GetClass() != NodeClass)
					{
						AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ClassMismatch"),
							FString::Printf(TEXT("Node type '%s' does not match the existing node's class '%s'"),
								*ParsedNode.NodeType, *ExistingNode->GetClass()->GetPathName()),
							ParsedNode.NodeGuid);
					}
				}
			}
		}

		ValidateAddOnsRecursive(TargetAsset, NodeClass, ParsedNode.NodeGuid, ParsedNode.AddOns, OutFindings);
	}
}

void FFlowGraphValidation::ValidateConnections(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	TArray<FFlowValidationFinding>& OutFindings,
	const TSet<FGuid>* PostPlanNodeGuids)
{
	TMap<TPair<FGuid, FName>, FString> SeenSourcePins;
	for (const FFlowGraphParsedConnection& Connection : ParsedConnections)
	{
		if (Connection.bIsDeleteMarker)
		{
			continue;
		}

		ValidateConnectionFanOut(TargetAsset, ParsedNodes, Connection, SeenSourcePins, OutFindings);

		if (PostPlanNodeGuids)
		{
			ValidateConnectionEndpoints(TargetAsset, ParsedNodes, Connection, *PostPlanNodeGuids, OutFindings);
			ValidateConnectionPinTypes(TargetAsset, ParsedNodes, Connection, OutFindings);
		}
	}
}

void FFlowGraphValidation::ValidateConnectionFanOut(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const FFlowGraphParsedConnection& Connection,
	TMap<TPair<FGuid, FName>, FString>& OutSeenSourcePins,
	TArray<FFlowValidationFinding>& OutFindings)
{
	// Data output pins may legally drive several inputs; only exec pins are one-target-per-pin.
	// An unresolvable type means the pin is missing or its type is undeterminable - in the missing
	// case ValidateConnectionEndpoints reports PinNotFound, so staying silent here avoids a second,
	// misleading error rather than losing the diagnosis.
	const FName SourceType = GetConnectionPinType(
		TargetAsset, ParsedNodes, Connection.SourceNodeGuid, Connection.SourcePinName, /*bIsOutputPin=*/true);
	if (SourceType == NAME_None || !FFlowPin::IsExecPinCategory(SourceType))
	{
		return;
	}

	const TPair<FGuid, FName> SourceKey(Connection.SourceNodeGuid, Connection.SourcePinName);
	const FString ThisTarget = FString::Printf(TEXT("%s.%s"),
		*Connection.TargetNodeGuid.ToString(), *Connection.TargetPinName.ToString());

	if (const FString* ExistingTarget = OutSeenSourcePins.Find(SourceKey))
	{
		if (*ExistingTarget != ThisTarget)
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("ExecFanOut"),
				FString::Printf(TEXT("Fan-out from a single output pin %s.%s: targets both %s and %s (one-target-per-output-pin)"),
					*Connection.SourceNodeGuid.ToString(), *Connection.SourcePinName.ToString(),
					**ExistingTarget, *ThisTarget),
				Connection.SourceNodeGuid, FGuid(), Connection.SourcePinName);
		}
	}
	else
	{
		OutSeenSourcePins.Add(SourceKey, ThisTarget);
	}
}

void FFlowGraphValidation::ValidateConnectionEndpoints(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const FFlowGraphParsedConnection& Connection,
	const TSet<FGuid>& PostPlanNodeGuids,
	TArray<FFlowValidationFinding>& OutFindings)
{
	const bool bSourceNodeExists = PostPlanNodeGuids.Contains(Connection.SourceNodeGuid);
	const bool bTargetNodeExists = PostPlanNodeGuids.Contains(Connection.TargetNodeGuid);

	if (!bSourceNodeExists)
	{
		AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("DanglingEndpoint"),
			FString::Printf(TEXT("Connection source node does not exist after the plan applies: %s"), *Connection.SourceNodeGuid.ToString()),
			Connection.SourceNodeGuid, FGuid(), Connection.SourcePinName);
	}
	else
	{
		const bool bSourcePinExists = DoesNodeHavePin(TargetAsset, ParsedNodes, Connection.SourceNodeGuid, Connection.SourcePinName, /*bIsOutputPin=*/true);
		if (!bSourcePinExists)
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("PinNotFound"),
				FString::Printf(TEXT("Connection references a source output pin that doesn't exist: %s.%s"),
					*Connection.SourceNodeGuid.ToString(), *Connection.SourcePinName.ToString()),
				Connection.SourceNodeGuid, FGuid(), Connection.SourcePinName);
		}
	}

	if (!bTargetNodeExists)
	{
		AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("DanglingEndpoint"),
			FString::Printf(TEXT("Connection target node does not exist after the plan applies: %s"), *Connection.TargetNodeGuid.ToString()),
			Connection.TargetNodeGuid, FGuid(), Connection.TargetPinName);
	}
	else
	{
		const bool bTargetPinExists = DoesNodeHavePin(TargetAsset, ParsedNodes, Connection.TargetNodeGuid, Connection.TargetPinName, /*bIsOutputPin=*/false);
		if (!bTargetPinExists)
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("PinNotFound"),
				FString::Printf(TEXT("Connection references a target input pin that doesn't exist: %s.%s"),
					*Connection.TargetNodeGuid.ToString(), *Connection.TargetPinName.ToString()),
				Connection.TargetNodeGuid, FGuid(), Connection.TargetPinName);
		}
	}
}

void FFlowGraphValidation::ValidateConnectionPinTypes(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const FFlowGraphParsedConnection& Connection,
	TArray<FFlowValidationFinding>& OutFindings)
{
	const bool bSourcePinExists = DoesNodeHavePin(TargetAsset, ParsedNodes, Connection.SourceNodeGuid, Connection.SourcePinName, /*bIsOutputPin=*/true);
	const bool bTargetPinExists = DoesNodeHavePin(TargetAsset, ParsedNodes, Connection.TargetNodeGuid, Connection.TargetPinName, /*bIsOutputPin=*/false);

	if (!bSourcePinExists || !bTargetPinExists)
	{
		return;
	}

	const FName SourceType = GetConnectionPinType(TargetAsset, ParsedNodes, Connection.SourceNodeGuid, Connection.SourcePinName, /*bIsOutputPin=*/true);
	const FName TargetType = GetConnectionPinType(TargetAsset, ParsedNodes, Connection.TargetNodeGuid, Connection.TargetPinName, /*bIsOutputPin=*/false);

	if (SourceType == NAME_None || TargetType == NAME_None)
	{
		return;
	}

	const bool bSourceExec = FFlowPin::IsExecPinCategory(SourceType);
	const bool bTargetExec = FFlowPin::IsExecPinCategory(TargetType);

	if (bSourceExec != bTargetExec)
	{
		AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("PinKindMismatch"),
			FString::Printf(TEXT("Connection joins an exec pin to a data pin: %s.%s [%s] -> %s.%s [%s]"),
				*Connection.SourceNodeGuid.ToString(), *Connection.SourcePinName.ToString(), *SourceType.ToString(),
				*Connection.TargetNodeGuid.ToString(), *Connection.TargetPinName.ToString(), *TargetType.ToString()),
			Connection.TargetNodeGuid, FGuid(), Connection.TargetPinName);
	}
	else if (!bSourceExec && TargetAsset)
	{
		const FFlowPinConnectionPolicy& Policy = TargetAsset->GetPinConnectionPolicy();
		const TSet<FName>& SupportedTypes = Policy.GetAllSupportedTypes();
		if (SupportedTypes.Contains(SourceType) && SupportedTypes.Contains(TargetType) &&
			!Policy.CanConnectPinTypeNames(SourceType, TargetType))
		{
			AddFinding(OutFindings, EFlowValidationSeverity::Error, TEXT("DataPinTypeMismatch"),
				FString::Printf(TEXT("Data pin type mismatch under the asset's pin connection policy: %s.%s [%s] -> %s.%s [%s]"),
					*Connection.SourceNodeGuid.ToString(), *Connection.SourcePinName.ToString(), *SourceType.ToString(),
					*Connection.TargetNodeGuid.ToString(), *Connection.TargetPinName.ToString(), *TargetType.ToString()),
				Connection.TargetNodeGuid, FGuid(), Connection.TargetPinName);
		}
	}
}

void FFlowGraphValidation::RunValidationHooks(
	const UFlowAsset* TargetAsset,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	TArray<FFlowValidationFinding>& OutFindings)
{
	if (!TargetAsset)
	{
		return;
	}

	for (const TPair<TSubclassOf<UFlowAsset>, TArray<FFlowValidationHookDelegate>>& RegistryEntry : GetHookRegistry())
	{
		if (!RegistryEntry.Key || !TargetAsset->GetClass()->IsChildOf(RegistryEntry.Key))
		{
			continue;
		}

		for (const FFlowValidationHookDelegate& Hook : RegistryEntry.Value)
		{
			if (Hook.IsBound())
			{
				Hook.Execute(TargetAsset, ParsedNodes, ParsedConnections, OutFindings);
			}
		}
	}
}
