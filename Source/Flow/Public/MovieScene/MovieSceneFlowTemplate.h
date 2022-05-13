// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Evaluation/MovieSceneEvalTemplate.h"

#include "MovieSceneFlowRepeaterSection.h"
#include "MovieSceneFlowTrack.h"
#include "MovieSceneFlowTriggerSection.h"
#include "MovieSceneFlowTemplate.generated.h"

USTRUCT()
struct FMovieSceneFlowTemplateBase : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FMovieSceneFlowTemplateBase()
		: bFireEventsWhenForwards(false)
		, bFireEventsWhenBackwards(false)
	{
	}

	FMovieSceneFlowTemplateBase(const UMovieSceneFlowTrack& InTrack, const UMovieSceneFlowSectionBase& InSection)
		: bFireEventsWhenForwards(InTrack.bFireEventsWhenForwards)
		, bFireEventsWhenBackwards(InTrack.bFireEventsWhenBackwards)
	{
	}

protected:
	UPROPERTY()
	uint32 bFireEventsWhenForwards : 1;

	UPROPERTY()
	uint32 bFireEventsWhenBackwards : 1;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
};

USTRUCT()
struct FMovieSceneFlowTriggerTemplate : public FMovieSceneFlowTemplateBase
{
	GENERATED_BODY()

	FMovieSceneFlowTriggerTemplate() {}
	FMovieSceneFlowTriggerTemplate(const UMovieSceneFlowTriggerSection& Section, const UMovieSceneFlowTrack& Track);

	UPROPERTY()
	TArray<FFrameNumber> EventTimes;

	UPROPERTY()
	TArray<FString> EventNames;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};

USTRUCT()
struct FMovieSceneFlowRepeaterTemplate : public FMovieSceneFlowTemplateBase
{
	GENERATED_BODY()

	FMovieSceneFlowRepeaterTemplate() {}
	FMovieSceneFlowRepeaterTemplate(const UMovieSceneFlowRepeaterSection& Section, const UMovieSceneFlowTrack& Track);

	UPROPERTY()
	FString EventName;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};
