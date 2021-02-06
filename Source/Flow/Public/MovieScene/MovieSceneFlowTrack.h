#pragma once

#include "Tracks/MovieSceneEventTrack.h"
#include "MovieSceneFlowTrack.generated.h"

struct FMovieSceneEvaluationTrack;

/**
 * Implements a movie scene track that triggers events in the Flow System during playback.
 */
UCLASS()
class FLOW_API UMovieSceneFlowTrack : public UMovieSceneNameableTrack
{
	GENERATED_BODY()

public:
	UMovieSceneFlowTrack()
		: bFireEventsWhenForwards(true)
		, bFireEventsWhenBackwards(true)
		, EventPosition(EFireEventsAtPosition::AfterSpawn)
	{
#if WITH_EDITOR
		TrackTint = FColor(91, 75, 57);
#endif
	}

	// UMovieSceneTrack interface
	virtual void AddSection(UMovieSceneSection& Section) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual bool IsEmpty() const override;
	virtual void RemoveAllAnimationData() override;
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;
	virtual void PostCompile(FMovieSceneEvaluationTrack& Track, const FMovieSceneTrackCompilerArgs& Args) const override;
	virtual bool SupportsMultipleRows() const override { return true; }
	virtual FMovieSceneTrackSegmentBlenderPtr GetTrackSegmentBlender() const override;

#if WITH_EDITOR
	virtual FText GetDefaultDisplayName() const override;
#endif

	/** If events should be fired when passed playing the sequence forwards. */
	UPROPERTY(EditAnywhere, Category=TrackEvent)
	uint32 bFireEventsWhenForwards:1;

	/** If events should be fired when passed playing the sequence backwards. */
	UPROPERTY(EditAnywhere, Category=TrackEvent)
	uint32 bFireEventsWhenBackwards:1;

	/** Defines where in the evaluation to trigger events */
	UPROPERTY(EditAnywhere, Category=TrackEvent)
	EFireEventsAtPosition EventPosition;

private:
	/** The track's sections. */
	UPROPERTY()
	TArray<UMovieSceneSection*> Sections;
};
