// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IMessageLogListing.h"

#include "FlowAsset.h"

UENUM()
enum class EFlowLogType : uint8
{
	Runtime,
	Validation
};

/**
 * Scope wrapper for the message log. Ensures we don't leak logs that we dont need (i.e. those that have no messages)
 * Replicated after FScopedBlueprintMessageLog
 */
class FLOWEDITOR_API FFlowMessageLogListing
{
public:
	FFlowMessageLogListing(const UFlowAsset* InFlowAsset, const EFlowLogType Type);
	~FFlowMessageLogListing();
	
public:
	TSharedRef<IMessageLogListing> Log;
	FName LogName;

private:
	static TSharedRef<IMessageLogListing> RegisterLogListing(const UFlowAsset* InFlowAsset, const EFlowLogType Type);
	static FName GetListingName(const UFlowAsset* InFlowAsset, const EFlowLogType Type);

public:
	static TSharedRef<IMessageLogListing> GetLogListing(const UFlowAsset* InFlowAsset, const EFlowLogType Type);
	static FString GetLogLabel(const EFlowLogType Type);
};
