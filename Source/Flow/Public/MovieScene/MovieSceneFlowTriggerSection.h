#pragma once

#include "Channels/MovieSceneStringChannel.h"

#include "MovieSceneFlowSectionBase.h"
#include "MovieSceneFlowTriggerSection.generated.h"

/**
 * Flow section that triggers specific timed events.
 */
UCLASS()
class FLOW_API UMovieSceneFlowTriggerSection : public UMovieSceneFlowSectionBase
{
	GENERATED_BODY()

public:
	UMovieSceneFlowTriggerSection(const FObjectInitializer& ObjInit);

#if WITH_EDITORONLY_DATA
	virtual TArrayView<FString> GetAllEntryPoints() override { return StringChannel.GetData().GetValues(); }
#endif

	/** The channel that defines this section's timed events */
	UPROPERTY()
	FMovieSceneStringChannel StringChannel;
};
