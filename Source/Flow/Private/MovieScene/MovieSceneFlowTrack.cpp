#include "MovieScene/MovieSceneFlowTrack.h"
#include "MovieScene/MovieSceneFlowRepeaterSection.h"
#include "MovieScene/MovieSceneFlowTemplate.h"
#include "MovieScene/MovieSceneFlowTriggerSection.h"

#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "IMovieSceneTracksModule.h"

#define LOCTEXT_NAMESPACE "MovieSceneFlowTrack"

void UMovieSceneFlowTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}

bool UMovieSceneFlowTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass->IsChildOf(UMovieSceneFlowSectionBase::StaticClass());
}

UMovieSceneSection* UMovieSceneFlowTrack::CreateNewSection()
{
	return NewObject<UMovieSceneFlowTriggerSection>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UMovieSceneFlowTrack::GetAllSections() const
{
	return Sections;
}

bool UMovieSceneFlowTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}

bool UMovieSceneFlowTrack::IsEmpty() const
{
	return (Sections.Num() == 0);
}

void UMovieSceneFlowTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}

void UMovieSceneFlowTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}

FMovieSceneEvalTemplatePtr UMovieSceneFlowTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	if (const UMovieSceneFlowTriggerSection* TriggerSection = Cast<const UMovieSceneFlowTriggerSection>(&InSection))
	{
		return FMovieSceneFlowTriggerTemplate(*TriggerSection, *this);
	}

	if (const UMovieSceneFlowRepeaterSection* RepeaterSection = Cast<const UMovieSceneFlowRepeaterSection>(&InSection))
	{
		return FMovieSceneFlowRepeaterTemplate(*RepeaterSection, *this);
	}

	return FMovieSceneEvalTemplatePtr();
}

void UMovieSceneFlowTrack::PostCompile(FMovieSceneEvaluationTrack& Track, const FMovieSceneTrackCompilerArgs& Args) const
{
	switch (EventPosition)
	{
		case EFireEventsAtPosition::AtStartOfEvaluation:
			Track.SetEvaluationGroup(IMovieSceneTracksModule::GetEvaluationGroupName(EBuiltInEvaluationGroup::PreEvaluation));
			break;

		case EFireEventsAtPosition::AtEndOfEvaluation:
			Track.SetEvaluationGroup(IMovieSceneTracksModule::GetEvaluationGroupName(EBuiltInEvaluationGroup::PostEvaluation));
			break;

		default:
			Track.SetEvaluationGroup(IMovieSceneTracksModule::GetEvaluationGroupName(EBuiltInEvaluationGroup::SpawnObjects));
			Track.SetEvaluationPriority(UMovieSceneSpawnTrack::GetEvaluationPriority() - 100);
			break;
	}

	Track.SetEvaluationMethod(EEvaluationMethod::Swept);
}

FMovieSceneTrackSegmentBlenderPtr UMovieSceneFlowTrack::GetTrackSegmentBlender() const
{
	// This is a temporary measure to alleviate some issues with event tracks with finite ranges.
	// By filling empty space between sections, we're essentially always making this track evaluate
	// which allows it to sweep sections correctly when the play-head moves from a finite section
	// to empty space. This doesn't address the issue of the play-head moving from inside a sub-sequence
	// to outside, but that specific issue is even more nuanced and complicated to address.

	struct FMovieSceneFlowTrackSegmentBlender : FMovieSceneTrackSegmentBlender
	{
		FMovieSceneFlowTrackSegmentBlender()
		{
			bCanFillEmptySpace = true;
			bAllowEmptySegments = true;
		}

		virtual TOptional<FMovieSceneSegment> InsertEmptySpace(const TRange<FFrameNumber>& Range, const FMovieSceneSegment* PreviousSegment, const FMovieSceneSegment* NextSegment) const override
		{
			return FMovieSceneSegment(Range);
		}
	};

	return FMovieSceneFlowTrackSegmentBlender();
}

#if WITH_EDITORONLY_DATA

FText UMovieSceneFlowTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("TrackName", "Flow Events");
}

#endif

#undef LOCTEXT_NAMESPACE
