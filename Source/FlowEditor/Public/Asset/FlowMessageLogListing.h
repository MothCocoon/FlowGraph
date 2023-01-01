// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IMessageLogListing.h"

#include "FlowAsset.h"

/**
 * Scope wrapper for the message log. Ensures we don't leak logs that we dont need (i.e. those that have no messages)
 * Replicated after FScopedBlueprintMessageLog
 */
class FLOWEDITOR_API FFlowMessageLogListing
{
public:
	FFlowMessageLogListing(const UFlowAsset* InFlowAsset);
	~FFlowMessageLogListing();
	
public:
	TSharedRef<IMessageLogListing> Log;
	FName LogName;

private:
	static TSharedRef<IMessageLogListing> RegisterLogListing(const UFlowAsset* InFlowAsset);
	static FName GetListingName(const UFlowAsset* InFlowAsset);

public:
	static TSharedRef<IMessageLogListing> GetLogListing(const UFlowAsset* InFlowAsset);
};
