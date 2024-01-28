// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "MovieScene/MovieSceneFlowTriggerSection.h"

#include "Channels/MovieSceneChannelProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneFlowTriggerSection)

UMovieSceneFlowTriggerSection::UMovieSceneFlowTriggerSection(const FObjectInitializer& ObjInit)
	: Super(ObjInit)
{
	bSupportsInfiniteRange = true;
	UMovieSceneSection::SetRange(TRange<FFrameNumber>::All());

#if WITH_EDITOR
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(StringChannel, FMovieSceneChannelMetaData(), TMovieSceneExternalValue<FString>::Make());
#else
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(StringChannel);
#endif
}
