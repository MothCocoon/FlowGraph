// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "MovieSceneFlowSectionBase.h"
#include "MovieSceneFlowRepeaterSection.generated.h"

/**
 * Flow section that will trigger its event exactly once, every time it is evaluated.
 */
UCLASS()
class FLOW_API UMovieSceneFlowRepeaterSection : public UMovieSceneFlowSectionBase
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	virtual TArrayView<FString> GetAllEntryPoints() override { return MakeArrayView(&EventName, 1); }
#endif

	/** The event that should be triggered each time this section is evaluated */
	UPROPERTY(EditAnywhere, Category = "Flow")
	FString EventName;
};
