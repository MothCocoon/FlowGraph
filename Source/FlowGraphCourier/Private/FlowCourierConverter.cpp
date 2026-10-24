// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowCourierConverter.h"
#include "FlowGraphImporter.h"
#include "FlowLogChannels.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "UObject/UnrealType.h"

namespace
{
	void AddIssue(TArray<FFlowCourierIssue>& OutIssues, EFlowValidationSeverity Severity, const FString& Path, const FString& Code, const FString& Message)
	{
		FFlowCourierIssue& Issue = OutIssues.AddDefaulted_GetRef();
		Issue.Severity = Severity;
		Issue.Path = Path;
		Issue.Code = Code;
		Issue.Message = Message;
	}

	bool TryParseGuid(const FString& GuidString, FGuid& OutGuid)
	{
		return !GuidString.IsEmpty() && FGuid::Parse(GuidString, OutGuid);
	}

	bool IsEndpointSet(const FFlowCourierEndpoint& Endpoint)
	{
		return !Endpoint.NodeGuid.IsEmpty() || !Endpoint.NodeAlias.IsEmpty();
	}

	bool IsEndpointAmbiguous(const FFlowCourierEndpoint& Endpoint)
	{
		return !Endpoint.NodeGuid.IsEmpty() && !Endpoint.NodeAlias.IsEmpty();
	}

	// Parsed graph nodes store pin declarations as strings. Render the Courier pin fields in the
	// declaration form accepted by ParsePinDecl, preserving pin names exactly, including spaces.
	FString RenderPinDeclBridge(const FFlowCourierPin& Pin)
	{
		if (Pin.Type.IsEmpty())
		{
			return Pin.Name;
		}
		if (Pin.SubCategoryPath.IsEmpty())
		{
			return FString::Printf(TEXT("%s [%s]"), *Pin.Name, *Pin.Type);
		}
		return FString::Printf(TEXT("%s [%s:%s]"), *Pin.Name, *Pin.Type, *Pin.SubCategoryPath);
	}
}

bool FFlowCourierConverter::ParseDocument(
	const FString& JsonText,
	FFlowCourierDocument& OutDocument,
	TArray<FFlowCourierIssue>& OutIssues,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (JsonText.IsEmpty())
	{
		OutErrorMessage = TEXT("Courier document text is empty");
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutErrorMessage = TEXT("Courier document is not valid JSON");
		return false;
	}

	FText FailReason;
	if (!FJsonObjectConverter::JsonObjectToUStruct(RootObject.ToSharedRef(), &OutDocument, 0, 0, false, &FailReason))
	{
		OutErrorMessage = FailReason.ToString();
		return false;
	}

	if (OutDocument.FormatVersion != 2)
	{
		OutErrorMessage = FString::Printf(TEXT("Unsupported formatVersion %d (expected 2). There is no compatibility shim for other versions."), OutDocument.FormatVersion);
		AddIssue(OutIssues, EFlowValidationSeverity::Error, TEXT("formatVersion"), TEXT("BadFormatVersion"), OutErrorMessage);
		return false;
	}

	CheckUnknownFields(RootObject, FFlowCourierDocument::StaticStruct(), FString(), OutIssues);
	ValidateOpStructure(OutDocument, OutIssues);

	if (OutDocument.Mode == EFlowCourierMode::Full && !OutDocument.ScopeNodeGuids.IsEmpty())
	{
		AddIssue(OutIssues, EFlowValidationSeverity::Error, TEXT("scopeNodeGuids"), TEXT("ScopeWithFullMode"),
			TEXT("scopeNodeGuids is not legal in Full mode - the whole document is already authoritative over every node."));
	}

	return true;
}

FString FFlowCourierConverter::GetJsonKeyForProperty(const FProperty* Property)
{
	FString Key = Property->GetAuthoredName();
	if (!Key.IsEmpty())
	{
		Key[0] = FChar::ToLower(Key[0]);
	}
	return Key;
}

void FFlowCourierConverter::CheckUnknownFields(
	const TSharedPtr<FJsonObject>& JsonObject,
	UStruct* Struct,
	const FString& JsonPath,
	TArray<FFlowCourierIssue>& OutIssues)
{
	if (!JsonObject.IsValid() || !Struct)
	{
		return;
	}

	TMap<FString, FProperty*> KeyToProperty;
	for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
	{
		KeyToProperty.Add(GetJsonKeyForProperty(*PropIt), *PropIt);
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : JsonObject->Values)
	{
		const FString FieldPath = JsonPath.IsEmpty() ? Field.Key : JsonPath + TEXT(".") + Field.Key;

		FProperty* const* FoundProperty = KeyToProperty.Find(Field.Key);
		if (!FoundProperty)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, FieldPath, TEXT("UnknownField"),
				FString::Printf(TEXT("Unrecognized field '%s'."), *Field.Key));
			continue;
		}

		// Properties (TMap<FString,FString>) carries arbitrary designer property names, not part
		// of the Courier schema - never checked for unknown keys.
		if ((*FoundProperty)->IsA<FMapProperty>())
		{
			continue;
		}

		if (!Field.Value.IsValid())
		{
			continue;
		}

		if (const FStructProperty* StructProp = CastField<FStructProperty>(*FoundProperty))
		{
			if (Field.Value->Type == EJson::Object)
			{
				CheckUnknownFields(Field.Value->AsObject(), StructProp->Struct, FieldPath, OutIssues);
			}
		}
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*FoundProperty))
		{
			if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner))
			{
				if (Field.Value->Type == EJson::Array)
				{
					int32 ElementIndex = 0;
					for (const TSharedPtr<FJsonValue>& Element : Field.Value->AsArray())
					{
						if (Element.IsValid() && Element->Type == EJson::Object)
						{
							CheckUnknownFields(Element->AsObject(), InnerStructProp->Struct,
								FString::Printf(TEXT("%s[%d]"), *FieldPath, ElementIndex), OutIssues);
						}
						++ElementIndex;
					}
				}
			}
		}
	}

}

void FFlowCourierConverter::ValidateOpStructure(const FFlowCourierDocument& Document, TArray<FFlowCourierIssue>& OutIssues)
{
	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const FFlowCourierOp& Op = Document.Ops[Index];
		const FString OpPath = FString::Printf(TEXT("ops[%d]"), Index);

		const bool bHasGuid = !Op.Guid.IsEmpty();
		const bool bHasNewAlias = !Op.NewAlias.IsEmpty();
		const bool bHasParentGuid = !Op.ParentGuid.IsEmpty();
		const bool bHasParentAlias = !Op.ParentAlias.IsEmpty();

		const bool bIsConnectionKind = Op.Kind == EFlowCourierOpKind::AddConnection || Op.Kind == EFlowCourierOpKind::RemoveConnection;
		const bool bCanCreate = Op.Kind == EFlowCourierOpKind::UpsertNode || Op.Kind == EFlowCourierOpKind::UpsertAddon;
		const bool bIsAddonKind = Op.Kind == EFlowCourierOpKind::UpsertAddon || Op.Kind == EFlowCourierOpKind::DeleteAddon;
		const bool bIsNodeUpsert = Op.Kind == EFlowCourierOpKind::UpsertNode;

		// Identity: guid/newAlias.
		if (!bIsConnectionKind)
		{
			if (bHasGuid == bHasNewAlias)
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("IdentityAmbiguous"),
					TEXT("Exactly one of guid/newAlias must be set."));
			}
			else if (bHasNewAlias && !bCanCreate)
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".newAlias"), TEXT("FieldNotLegalForKind"),
					TEXT("newAlias is only legal on UpsertNode/UpsertAddon."));
			}
		}
		else if (bHasGuid || bHasNewAlias)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("FieldNotLegalForKind"),
				TEXT("guid/newAlias are not legal on a connection op."));
		}

		// Parent identity: parentGuid/parentAlias.
		if (bIsAddonKind)
		{
			if (bHasParentGuid == bHasParentAlias)
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("ParentAmbiguous"),
					TEXT("Exactly one of parentGuid/parentAlias must be set."));
			}
		}
		else if (bHasParentGuid || bHasParentAlias)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("FieldNotLegalForKind"),
				TEXT("parentGuid/parentAlias are only legal on UpsertAddon/DeleteAddon."));
		}

		// type: legal on Upsert*, required when creating.
		if (!Op.Type.IsEmpty() && !bCanCreate)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".type"), TEXT("FieldNotLegalForKind"),
				TEXT("type is only legal on UpsertNode/UpsertAddon."));
		}
		else if (Op.Type.IsEmpty() && bCanCreate && bHasNewAlias)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".type"), TEXT("MissingRequiredField"),
				TEXT("type is required when creating a node/addon."));
		}

		// properties/inputPins/outputPins: legal on Upsert* only.
		if ((!Op.Properties.IsEmpty() || !Op.InputPins.IsEmpty() || !Op.OutputPins.IsEmpty()) && !bCanCreate)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("FieldNotLegalForKind"),
				TEXT("properties/inputPins/outputPins are only legal on UpsertNode/UpsertAddon."));
		}

		// bHasPosition/position/comment/bReplaceAddons: legal on UpsertNode only.
		if ((Op.bHasPosition || !Op.Comment.IsEmpty() || Op.bReplaceAddons) && !bIsNodeUpsert)
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("FieldNotLegalForKind"),
				TEXT("bHasPosition/position/comment/bReplaceAddons are only legal on UpsertNode."));
		}

		// source/target: required and unambiguous on connection ops, illegal elsewhere.
		if (bIsConnectionKind)
		{
			if (!IsEndpointSet(Op.Source) || IsEndpointAmbiguous(Op.Source))
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".source"), TEXT("IdentityAmbiguous"),
					TEXT("Exactly one of source.nodeGuid/source.nodeAlias must be set."));
			}
			if (!IsEndpointSet(Op.Target) || IsEndpointAmbiguous(Op.Target))
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".target"), TEXT("IdentityAmbiguous"),
					TEXT("Exactly one of target.nodeGuid/target.nodeAlias must be set."));
			}
		}
		else if (IsEndpointSet(Op.Source) || IsEndpointSet(Op.Target))
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath, TEXT("FieldNotLegalForKind"),
				TEXT("source/target are only legal on AddConnection/RemoveConnection."));
		}
	}
}

void FFlowCourierConverter::ResolveIdentities(
	const FFlowCourierDocument& Document,
	TMap<FString, FGuid>& OutAliasMap,
	TArray<FGuid>& OutResolvedSelfGuid,
	TArray<FGuid>& OutResolvedParentGuid,
	TArray<FFlowCourierIssue>& OutIssues)
{
	const int32 OpCount = Document.Ops.Num();
	OutResolvedSelfGuid.Init(FGuid(), OpCount);
	OutResolvedParentGuid.Init(FGuid(), OpCount);

	// Pass 1: mint a GUID for every NewAlias up front. Node and addon aliases share one namespace -
	// an addon's ParentAlias may reference a node's NewAlias or another addon's - and minting
	// before resolving means reference order in the document never matters.
	for (int32 Index = 0; Index < OpCount; ++Index)
	{
		const FString& Alias = Document.Ops[Index].NewAlias;
		if (Alias.IsEmpty())
		{
			continue;
		}
		if (OutAliasMap.Contains(Alias))
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].newAlias"), Index), TEXT("DuplicateAlias"),
				FString::Printf(TEXT("Alias '%s' is defined more than once in this document."), *Alias));
			continue;
		}
		OutAliasMap.Add(Alias, FGuid::NewGuid());
	}

	// Pass 2: resolve every op's own identity and parent identity against the completed alias map.
	for (int32 Index = 0; Index < OpCount; ++Index)
	{
		const FFlowCourierOp& Op = Document.Ops[Index];
		const FString OpPath = FString::Printf(TEXT("ops[%d]"), Index);

		if (!Op.NewAlias.IsEmpty())
		{
			OutResolvedSelfGuid[Index] = OutAliasMap.FindRef(Op.NewAlias);
		}
		else if (!Op.Guid.IsEmpty())
		{
			FGuid Parsed;
			if (TryParseGuid(Op.Guid, Parsed))
			{
				OutResolvedSelfGuid[Index] = Parsed;
			}
			else
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".guid"), TEXT("BadGuid"),
					FString::Printf(TEXT("'%s' is not a valid GUID."), *Op.Guid));
			}
		}

		if (!Op.ParentAlias.IsEmpty())
		{
			if (const FGuid* Resolved = OutAliasMap.Find(Op.ParentAlias))
			{
				OutResolvedParentGuid[Index] = *Resolved;
			}
			else
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".parentAlias"), TEXT("UnresolvedAlias"),
					FString::Printf(TEXT("Alias '%s' is not defined by any op in this document."), *Op.ParentAlias));
			}
		}
		else if (!Op.ParentGuid.IsEmpty())
		{
			FGuid Parsed;
			if (TryParseGuid(Op.ParentGuid, Parsed))
			{
				OutResolvedParentGuid[Index] = Parsed;
			}
			else
			{
				AddIssue(OutIssues, EFlowValidationSeverity::Error, OpPath + TEXT(".parentGuid"), TEXT("BadGuid"),
					FString::Printf(TEXT("'%s' is not a valid GUID."), *Op.ParentGuid));
			}
		}
	}
}

TArray<FFlowGraphParsedNodeAddOn> FFlowCourierConverter::BuildAddonChildren(
	const FFlowCourierDocument& Document,
	const TArray<FGuid>& ResolvedSelfGuid,
	const TArray<FGuid>& ResolvedParentGuid,
	const FGuid& OwnerGuid,
	TSet<FGuid>& AncestorGuids,
	TArray<FFlowCourierIssue>& OutIssues)
{
	TArray<FFlowGraphParsedNodeAddOn> Children;

	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const FFlowCourierOp& Op = Document.Ops[Index];
		const bool bIsAddonOp = Op.Kind == EFlowCourierOpKind::UpsertAddon || Op.Kind == EFlowCourierOpKind::DeleteAddon;
		if (!bIsAddonOp || ResolvedParentGuid[Index] != OwnerGuid)
		{
			continue;
		}

		const FGuid& SelfGuid = ResolvedSelfGuid[Index];
		if (!SelfGuid.IsValid())
		{
			continue;
		}
		if (AncestorGuids.Contains(SelfGuid))
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].parentGuid"), Index), TEXT("AddonCycle"),
				FString::Printf(TEXT("Addon '%s' forms a cycle in its parent chain."), *SelfGuid.ToString()));
			continue;
		}

		FFlowGraphParsedNodeAddOn& AddOn = Children.AddDefaulted_GetRef();
		AddOn.AddOnGuid = SelfGuid;

		if (Op.Kind == EFlowCourierOpKind::DeleteAddon)
		{
			AddOn.bIsDeleteMarker = true;
			continue;
		}

		AddOn.AddOnType = Op.Type;
		AddOn.Properties = Op.Properties;
		AncestorGuids.Add(SelfGuid);
		AddOn.AddOns = BuildAddonChildren(Document, ResolvedSelfGuid, ResolvedParentGuid, SelfGuid, AncestorGuids, OutIssues);
		AncestorGuids.Remove(SelfGuid);
	}

	return Children;
}

void FFlowCourierConverter::ConvertToParsedGraph(
	const FFlowCourierDocument& Document,
	TArray<FFlowGraphParsedNode>& OutNodes,
	TArray<FFlowGraphParsedConnection>& OutConnections,
	TArray<FGuid>& OutScopedNodeGuids,
	TMap<FString, FGuid>& OutAliasMap,
	TArray<FFlowCourierIssue>& OutIssues)
{
	OutNodes.Reset();
	OutConnections.Reset();
	OutScopedNodeGuids.Reset();
	OutAliasMap.Reset();

	TArray<FGuid> ResolvedSelfGuid;
	TArray<FGuid> ResolvedParentGuid;
	ResolveIdentities(Document, OutAliasMap, ResolvedSelfGuid, ResolvedParentGuid, OutIssues);

	for (int32 Index = 0; Index < Document.ScopeNodeGuids.Num(); ++Index)
	{
		FGuid Parsed;
		if (TryParseGuid(Document.ScopeNodeGuids[Index], Parsed))
		{
			OutScopedNodeGuids.Add(Parsed);
		}
		else
		{
			AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("scopeNodeGuids[%d]"), Index), TEXT("BadGuid"),
				FString::Printf(TEXT("'%s' is not a valid GUID."), *Document.ScopeNodeGuids[Index]));
		}
	}

	TSet<FGuid> ParsedNodeGuids;

	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const FFlowCourierOp& Op = Document.Ops[Index];
		const FGuid& SelfGuid = ResolvedSelfGuid[Index];

		switch (Op.Kind)
		{
			case EFlowCourierOpKind::UpsertNode:
			{
				if (!SelfGuid.IsValid())
				{
					continue;
				}

				FFlowGraphParsedNode& Node = OutNodes.AddDefaulted_GetRef();
				ParsedNodeGuids.Add(SelfGuid);
				Node.NodeGuid = SelfGuid;
				Node.NodeType = Op.Type;
				Node.Properties = Op.Properties;

				for (const FFlowCourierPin& Pin : Op.InputPins)
				{
					Node.InputPins.Add(RenderPinDeclBridge(Pin));
				}
				for (const FFlowCourierPin& Pin : Op.OutputPins)
				{
					Node.OutputPins.Add(RenderPinDeclBridge(Pin));
				}

				Node.bHasPos = Op.bHasPosition;
				Node.Pos = Op.Position;
				Node.bHasComment = !Op.Comment.IsEmpty();
				Node.NodeComment = Op.Comment;
				Node.bIsNewAlias = !Op.NewAlias.IsEmpty();
				Node.Alias = Op.NewAlias;
				Node.bIsFullBlock = Op.bReplaceAddons;
				TSet<FGuid> AncestorGuids;
				AncestorGuids.Add(SelfGuid);
				Node.AddOns = BuildAddonChildren(Document, ResolvedSelfGuid, ResolvedParentGuid, SelfGuid, AncestorGuids, OutIssues);
				break;
			}

			case EFlowCourierOpKind::DeleteNode:
			{
				if (!SelfGuid.IsValid())
				{
					continue;
				}

				FFlowGraphParsedNode& Node = OutNodes.AddDefaulted_GetRef();
				Node.NodeGuid = SelfGuid;
				Node.bIsDeleteMarker = true;
				break;
			}

			case EFlowCourierOpKind::UpsertAddon:
			case EFlowCourierOpKind::DeleteAddon:
				// Folded into the owning node's (or ancestor addon's) nested AddOns list by
				// BuildAddonChildren when that owner is converted above - nothing to append here.
				break;

			case EFlowCourierOpKind::AddConnection:
			case EFlowCourierOpKind::RemoveConnection:
			{
				FGuid SourceGuid, TargetGuid;
				bool bResolved = true;

				if (!Op.Source.NodeAlias.IsEmpty())
				{
					if (const FGuid* Found = OutAliasMap.Find(Op.Source.NodeAlias))
					{
						SourceGuid = *Found;
					}
					else
					{
						AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].source.nodeAlias"), Index), TEXT("UnresolvedAlias"),
							FString::Printf(TEXT("Alias '%s' is not defined by any op in this document."), *Op.Source.NodeAlias));
						bResolved = false;
					}
				}
				else if (!TryParseGuid(Op.Source.NodeGuid, SourceGuid))
				{
					AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].source.nodeGuid"), Index), TEXT("BadGuid"),
						FString::Printf(TEXT("'%s' is not a valid GUID."), *Op.Source.NodeGuid));
					bResolved = false;
				}

				if (!Op.Target.NodeAlias.IsEmpty())
				{
					if (const FGuid* Found = OutAliasMap.Find(Op.Target.NodeAlias))
					{
						TargetGuid = *Found;
					}
					else
					{
						AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].target.nodeAlias"), Index), TEXT("UnresolvedAlias"),
							FString::Printf(TEXT("Alias '%s' is not defined by any op in this document."), *Op.Target.NodeAlias));
						bResolved = false;
					}
				}
				else if (!TryParseGuid(Op.Target.NodeGuid, TargetGuid))
				{
					AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].target.nodeGuid"), Index), TEXT("BadGuid"),
						FString::Printf(TEXT("'%s' is not a valid GUID."), *Op.Target.NodeGuid));
					bResolved = false;
				}

				if (!bResolved)
				{
					continue;
				}

				FFlowGraphParsedConnection& Connection = OutConnections.AddDefaulted_GetRef();
				Connection.SourceNodeGuid = SourceGuid;
				Connection.SourcePinName = FName(*Op.Source.Pin);
				Connection.TargetNodeGuid = TargetGuid;
				Connection.TargetPinName = FName(*Op.Target.Pin);
				Connection.bIsDeleteMarker = (Op.Kind == EFlowCourierOpKind::RemoveConnection);
				break;
			}
		}
	}

	TSet<FGuid> AddOnGuids;
	TMap<FGuid, FGuid> AddOnParentGuids;
	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const EFlowCourierOpKind Kind = Document.Ops[Index].Kind;
		if ((Kind == EFlowCourierOpKind::UpsertAddon || Kind == EFlowCourierOpKind::DeleteAddon)
			&& ResolvedSelfGuid[Index].IsValid())
		{
			AddOnGuids.Add(ResolvedSelfGuid[Index]);
			AddOnParentGuids.Add(ResolvedSelfGuid[Index], ResolvedParentGuid[Index]);
		}
	}

	TSet<FGuid> AddOnOwnerGuids;
	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const EFlowCourierOpKind Kind = Document.Ops[Index].Kind;
		if ((Kind == EFlowCourierOpKind::UpsertAddon || Kind == EFlowCourierOpKind::DeleteAddon)
			&& ResolvedParentGuid[Index].IsValid()
			&& !AddOnGuids.Contains(ResolvedParentGuid[Index]))
		{
			AddOnOwnerGuids.Add(ResolvedParentGuid[Index]);
		}
	}

	for (const FGuid& OwnerGuid : AddOnOwnerGuids)
	{
		if (ParsedNodeGuids.Contains(OwnerGuid))
		{
			continue;
		}

		FFlowGraphParsedNode& Placeholder = OutNodes.AddDefaulted_GetRef();
		Placeholder.NodeGuid = OwnerGuid;
		TSet<FGuid> AncestorGuids;
		AncestorGuids.Add(OwnerGuid);
		Placeholder.AddOns = BuildAddonChildren(Document, ResolvedSelfGuid, ResolvedParentGuid, OwnerGuid, AncestorGuids, OutIssues);
	}

	TSet<FGuid> CheckedAddOnGuids;
	for (int32 Index = 0; Index < Document.Ops.Num(); ++Index)
	{
		const FGuid& SelfGuid = ResolvedSelfGuid[Index];
		if (!AddOnGuids.Contains(SelfGuid) || CheckedAddOnGuids.Contains(SelfGuid))
		{
			continue;
		}

		TSet<FGuid> AncestorGuids;
		FGuid CurrentGuid = SelfGuid;
		while (const FGuid* ParentGuid = AddOnParentGuids.Find(CurrentGuid))
		{
			if (CheckedAddOnGuids.Contains(CurrentGuid))
			{
				break;
			}
			if (AncestorGuids.Contains(CurrentGuid))
			{
				if (!OutIssues.ContainsByPredicate([&CurrentGuid](const FFlowCourierIssue& Issue)
				{
					return Issue.Code == TEXT("AddonCycle") && Issue.Message.Contains(CurrentGuid.ToString());
				}))
				{
					AddIssue(OutIssues, EFlowValidationSeverity::Error, FString::Printf(TEXT("ops[%d].parentGuid"), Index), TEXT("AddonCycle"),
						FString::Printf(TEXT("Addon '%s' forms a cycle in its parent chain."), *CurrentGuid.ToString()));
				}
				break;
			}
			AncestorGuids.Add(CurrentGuid);
			CurrentGuid = *ParentGuid;
		}
		CheckedAddOnGuids.Append(AncestorGuids);
	}
}
