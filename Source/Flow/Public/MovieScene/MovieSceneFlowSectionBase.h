#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneFlowSectionBase.generated.h"

DECLARE_DELEGATE_TwoParams(FFlowEventExecution, UObject* /*EventReceiver*/, const FString& /*EventName*/)

/**
 * Base class for flow sections.
 */
UCLASS()
class FLOW_API UMovieSceneFlowSectionBase : public UMovieSceneSection
{
	GENERATED_BODY()

public:
	virtual TArrayView<FString> GetAllEntryPoints() { return TArrayView<FString>(); }

	FFlowEventExecution OnEventExecuted;
};
