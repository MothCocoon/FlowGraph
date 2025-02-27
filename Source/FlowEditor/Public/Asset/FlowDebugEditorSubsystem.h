// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Logging/TokenizedMessage.h"

#include "Debugger/FlowDebuggerSubsystem.h"
#include "FlowDebugEditorSubsystem.generated.h"

class UFlowAsset;
class FFlowMessageLog;

/**
 * Editor-only extension of debugger subsystem. Supports Message Log.
 */
UCLASS()
class FLOWEDITOR_API UFlowDebugEditorSubsystem : public UFlowDebuggerSubsystem
{
	GENERATED_BODY()

public:
	UFlowDebugEditorSubsystem();

protected:
	TMap<TWeakObjectPtr<UFlowAsset>, TSharedPtr<class IMessageLogListing>> RuntimeLogs;

	void OnInstancedTemplateAdded(UFlowAsset* FlowAsset);
	void OnInstancedTemplateRemoved(UFlowAsset* FlowAsset) const;

	void OnRuntimeMessageAdded(const UFlowAsset* FlowAsset, const TSharedRef<FTokenizedMessage>& Message) const;

	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

public:
	virtual void PausePlaySession() override;
	virtual bool IsPlaySessionPaused() override;
};
