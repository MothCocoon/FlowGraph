#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"

class SFlowGraphNode_Out : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SFlowGraphNode_Out){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UFlowGraphNode_Out* InNode);

protected:
	// SGraphNode interface
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	// End of SGraphNode interface
};
