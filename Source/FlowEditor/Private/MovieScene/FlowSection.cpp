// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "MovieScene/FlowSection.h"
#include "MovieScene/MovieSceneFlowRepeaterSection.h"
#include "MovieScene/MovieSceneFlowTriggerSection.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "MovieSceneTrack.h"
#include "Rendering/DrawElements.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Sections/MovieSceneEventSection.h"
#include "SequencerSectionPainter.h"
#include "SequencerTimeSliderController.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
#include "CommonMovieSceneTools.h"
#else
#include "TimeToPixel.h"
#endif

#define LOCTEXT_NAMESPACE "FlowSection"

bool FFlowSectionBase::IsSectionSelected() const
{
	const TSharedPtr<ISequencer> SequencerPtr = Sequencer.Pin();

	TArray<UMovieSceneTrack*> SelectedTracks;
	SequencerPtr->GetSelectedTracks(SelectedTracks);

	const UMovieSceneSection* Section = WeakSection.Get();
	UMovieSceneTrack* Track = Section ? CastChecked<UMovieSceneTrack>(Section->GetOuter()) : nullptr;
	return Track && SelectedTracks.Contains(Track);
}

void FFlowSectionBase::PaintEventName(FSequencerSectionPainter& Painter, int32 LayerId, const FString& InEventString, float PixelPos, bool bIsEventValid) const
{
	static constexpr float BoxOffsetPx = 10.f;
	static const TCHAR* WarningString = TEXT("\xf071");

	const FSlateFontInfo FontAwesomeFont = FAppStyle::Get().GetFontStyle("FontAwesome.10");
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FLinearColor DrawColor = FAppStyle::GetSlateColor("SelectionColor").GetColor(FWidgetStyle());

	TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	// Setup the warning size. Static since it won't ever change
	static FVector2D WarningSize = FontMeasureService->Measure(WarningString, FontAwesomeFont);
	const FMargin WarningPadding = (bIsEventValid || InEventString.Len() == 0) ? FMargin(0.f) : FMargin(0.f, 0.f, 4.f, 0.f);
	const FMargin BoxPadding = FMargin(4.0f, 2.0f);

	const FVector2D TextSize = FontMeasureService->Measure(InEventString, SmallLayoutFont);
	const FVector2D IconSize = bIsEventValid ? FVector2D::ZeroVector : WarningSize;
	const FVector2D PaddedIconSize = IconSize + WarningPadding.GetDesiredSize();
	const FVector2D BoxSize = FVector2D(TextSize.X + PaddedIconSize.X, FMath::Max(TextSize.Y, PaddedIconSize.Y)) + BoxPadding.GetDesiredSize();

	// Flip the text position if getting near the end of the view range
	bool bDrawLeft = (Painter.SectionGeometry.Size.X - PixelPos) < (BoxSize.X + 22.f) - BoxOffsetPx;
	float BoxPositionX = bDrawLeft ? PixelPos - BoxSize.X - BoxOffsetPx : PixelPos + BoxOffsetPx;
	if (BoxPositionX < 0.f)
	{
		BoxPositionX = 0.f;
	}

	FVector2D BoxOffset = FVector2D(BoxPositionX, Painter.SectionGeometry.Size.Y * .5f - BoxSize.Y * .5f);
	FVector2D IconOffset = FVector2D(BoxPadding.Left, BoxSize.Y * .5f - IconSize.Y * .5f);
	FVector2D TextOffset = FVector2D(IconOffset.X + PaddedIconSize.X, BoxSize.Y * .5f - TextSize.Y * .5f);

	// Draw the background box
	FSlateDrawElement::MakeBox(
		Painter.DrawElements,
		LayerId + 1,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		Painter.SectionGeometry.ToPaintGeometry(BoxOffset, BoxSize),
#else
		Painter.SectionGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(1.0f, TransformPoint(1.0f, UE::Slate::CastToVector2f(BoxOffset)))),
#endif
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.5f)
	);

	if (!bIsEventValid)
	{
		// Draw a warning icon for unbound repeaters
		FSlateDrawElement::MakeText(
			Painter.DrawElements,
			LayerId + 2,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			Painter.SectionGeometry.ToPaintGeometry(BoxOffset + IconOffset, IconSize),
#else
			Painter.SectionGeometry.ToPaintGeometry(IconSize, FSlateLayoutTransform(1.0f, TransformPoint(1.0f, UE::Slate::CastToVector2f(BoxOffset + IconOffset)))),
#endif
			WarningString,
			FontAwesomeFont,
			Painter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
			FAppStyle::GetWidgetStyle<FTextBlockStyle>("Log.Warning").ColorAndOpacity.GetSpecifiedColor()
		);
	}

	FSlateDrawElement::MakeText(
		Painter.DrawElements,
		LayerId + 2,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		Painter.SectionGeometry.ToPaintGeometry(BoxOffset + TextOffset, TextSize),
#else
		Painter.SectionGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(1.0f, TransformPoint(1.0f, UE::Slate::CastToVector2f(BoxOffset + TextOffset)))),
#endif
		InEventString,
		SmallLayoutFont,
		Painter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
		DrawColor
	);
}

int32 FFlowSection::OnPaintSection(FSequencerSectionPainter& Painter) const
{
	const int32 LayerId = Painter.PaintSectionBackground();
	const UMovieSceneEventSection* EventSection = Cast<UMovieSceneEventSection>(WeakSection.Get());
	if (!EventSection || !IsSectionSelected())
	{
		return LayerId;
	}

	const FTimeToPixel& TimeToPixelConverter = Painter.GetTimeConverter();

	for (int32 KeyIndex = 0; KeyIndex < EventSection->GetEventData().GetKeyTimes().Num(); ++KeyIndex)
	{
		FFrameNumber EventTime = EventSection->GetEventData().GetKeyTimes()[KeyIndex];
		FEventPayload EventData = EventSection->GetEventData().GetKeyValues()[KeyIndex];

		if (EventSection->GetRange().Contains(EventTime))
		{
			FString EventString = EventData.EventName.ToString();
			if (!EventString.IsEmpty())
			{
				const float PixelPos = TimeToPixelConverter.FrameToPixel(EventTime);
				PaintEventName(Painter, LayerId, EventString, PixelPos);
			}
		}
	}

	return LayerId + 3;
}

int32 FFlowTriggerSection::OnPaintSection(FSequencerSectionPainter& Painter) const
{
	const int32 LayerId = Painter.PaintSectionBackground();

	UMovieSceneFlowTriggerSection* EventTriggerSection = Cast<UMovieSceneFlowTriggerSection>(WeakSection.Get());
	if (!EventTriggerSection || !IsSectionSelected())
	{
		return LayerId;
	}

	const FTimeToPixel& TimeToPixelConverter = Painter.GetTimeConverter();

	const TArrayView<const FFrameNumber> Times = EventTriggerSection->StringChannel.GetData().GetTimes();
	const TArrayView<FString> Events = EventTriggerSection->StringChannel.GetData().GetValues();

	const TRange<FFrameNumber> EventSectionRange = EventTriggerSection->GetRange();
	for (int32 KeyIndex = 0; KeyIndex < Times.Num(); ++KeyIndex)
	{
		FFrameNumber EventTime = Times[KeyIndex];
		if (EventSectionRange.Contains(EventTime))
		{
			const FString EventString = Events.IsValidIndex(KeyIndex) ? Events[KeyIndex] : FString();
			const float PixelPos = TimeToPixelConverter.FrameToPixel(EventTime);
			PaintEventName(Painter, LayerId, EventString, PixelPos, true);
		}
	}

	return LayerId + 3;
}

int32 FFlowRepeaterSection::OnPaintSection(FSequencerSectionPainter& Painter) const
{
	const int32 LayerId = Painter.PaintSectionBackground();

	const UMovieSceneFlowRepeaterSection* EventRepeaterSection = Cast<UMovieSceneFlowRepeaterSection>(WeakSection.Get());
	if (!EventRepeaterSection)
	{
		return LayerId;
	}

	const FString EventString = EventRepeaterSection->EventName;
	const float TextOffsetX = EventRepeaterSection->GetRange().GetLowerBound().IsClosed() ? FMath::Max(0.f, Painter.GetTimeConverter().FrameToPixel(EventRepeaterSection->GetRange().GetLowerBoundValue())) : 0.f;
	PaintEventName(Painter, LayerId, EventString, TextOffsetX, true);

	return LayerId + 1;
}

#undef LOCTEXT_NAMESPACE
