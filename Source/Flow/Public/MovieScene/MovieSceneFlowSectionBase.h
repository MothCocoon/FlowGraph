// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneFlowSectionBase.generated.h"

/**
 * Base class for flow sections
 */
UCLASS()
class FLOW_API UMovieSceneFlowSectionBase : public UMovieSceneSection
{
	GENERATED_BODY()

public:
	virtual TArrayView<FString> GetAllEntryPoints() { return TArrayView<FString>(); }
};
