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

	virtual void OnInstancedTemplateAdded(UFlowAsset* AssetTemplate) override;
	virtual void OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate) const override;

	void OnRuntimeMessageAdded(const UFlowAsset* AssetTemplate, const TSharedRef<FTokenizedMessage>& Message) const;

	virtual void OnBeginPIE(const bool bIsSimulating);
	virtual void OnResumePIE(const bool bIsSimulating);
	virtual void OnEndPIE(const bool bIsSimulating);

	virtual void PauseSession() override;
};
