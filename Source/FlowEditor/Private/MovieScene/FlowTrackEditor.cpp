#include "FlowTrackEditor.h"
#include "FlowSection.h"

#include "MovieScene/MovieSceneFlowRepeaterSection.h"
#include "MovieScene/MovieSceneFlowTrack.h"
#include "MovieScene/MovieSceneFlowTriggerSection.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/Package.h"
#include "ISequencerSection.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "Sections/MovieSceneEventSection.h"
#include "SequencerUtilities.h"
#include "MovieSceneSequenceEditor.h"

#define LOCTEXT_NAMESPACE "FFlowTrackEditor"

TSharedRef<ISequencerTrackEditor> FFlowTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FFlowTrackEditor(InSequencer));
}

TSharedRef<ISequencerSection> FFlowTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	if (SectionObject.IsA<UMovieSceneFlowTriggerSection>())
	{
		return MakeShared<FFlowTriggerSection>(SectionObject, GetSequencer());
	}

	if (SectionObject.IsA<UMovieSceneFlowRepeaterSection>())
	{
		return MakeShared<FFlowRepeaterSection>(SectionObject, GetSequencer());
	}

	return MakeShared<FSequencerSection>(SectionObject);
}

FFlowTrackEditor::FFlowTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
}

void FFlowTrackEditor::AddFlowSubMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewTriggerSection", "Flow Trigger"),
		LOCTEXT("AddNewTriggerSectionTooltip", "Adds a new section that can trigger a Flow event at a specific time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FFlowTrackEditor::HandleAddFlowTrackMenuEntryExecute, UMovieSceneFlowTriggerSection::StaticClass())
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewRepeaterSection", "Flow Repeater"),
		LOCTEXT("AddNewRepeaterSectionTooltip", "Adds a new section that triggers a Flow event every time it's evaluated"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FFlowTrackEditor::HandleAddFlowTrackMenuEntryExecute, UMovieSceneFlowRepeaterSection::StaticClass())
		)
	);
}

void FFlowTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();
	FMovieSceneSequenceEditor* SequenceEditor = FMovieSceneSequenceEditor::Find(RootMovieSceneSequence);

	if (SequenceEditor && SequenceEditor->SupportsEvents(RootMovieSceneSequence))
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("AddTrack", "Flow Track"),
			LOCTEXT("AddTooltip", "Adds a new flow track that can trigger events in the Flow graph."),
			FNewMenuDelegate::CreateRaw(this, &FFlowTrackEditor::AddFlowSubMenu),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Tracks.Event")
		);
	}
}

TSharedPtr<SWidget> FFlowTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	check(Track);

	const TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (!SequencerPtr.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<UMovieSceneTrack> WeakTrack = Track;
	const int32 RowIndex = Params.TrackInsertRowIndex;
	auto SubMenuCallback = [this, WeakTrack, RowIndex]
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		UMovieSceneTrack* TrackPtr = WeakTrack.Get();
		if (TrackPtr)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewTriggerSection", "Flow Trigger"),
				LOCTEXT("AddNewTriggerSectionTooltip", "Adds a new section that can trigger a Flow event at a specific time"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FFlowTrackEditor::CreateNewSection, TrackPtr, RowIndex + 1, UMovieSceneFlowTriggerSection::StaticClass(), true))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewRepeaterSection", "Flow Repeater"),
				LOCTEXT("AddNewRepeaterSectionTooltip", "Adds a new section that triggers a Flow event every time it's evaluated"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FFlowTrackEditor::CreateNewSection, TrackPtr, RowIndex + 1, UMovieSceneFlowRepeaterSection::StaticClass(), true))
			);
		}
		else
		{
			MenuBuilder.AddWidget(SNew(STextBlock).Text(LOCTEXT("InvalidTrack", "Track is no longer valid")), FText(), true);
		}

		return MenuBuilder.MakeWidget();
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			FSequencerUtilities::MakeAddButton(LOCTEXT("AddSection", "Section"), FOnGetContent::CreateLambda(SubMenuCallback), Params.NodeIsHovered, GetSequencer())
		];
}

bool FFlowTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return (Type == UMovieSceneFlowTrack::StaticClass());
}

bool FFlowTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	static UClass* LevelSequenceClass = FindObject<UClass>(ANY_PACKAGE, TEXT("LevelSequence"), true);
	return InSequence && LevelSequenceClass && InSequence->GetClass()->IsChildOf(LevelSequenceClass);
}

const FSlateBrush* FFlowTrackEditor::GetIconBrush() const
{
	return FEditorStyle::GetBrush("Sequencer.Tracks.Event");
}

void FFlowTrackEditor::HandleAddFlowTrackMenuEntryExecute(UClass* SectionType)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	if (FocusedMovieScene->IsReadOnly())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddFlowTrack_Transaction", "Add Flow Track"));
	FocusedMovieScene->Modify();

	TArray<UMovieSceneFlowTrack*> NewTracks;

	UMovieSceneFlowTrack* NewMasterTrack = FocusedMovieScene->AddMasterTrack<UMovieSceneFlowTrack>();
	NewTracks.Add(NewMasterTrack);
	if (GetSequencer().IsValid())
	{
		GetSequencer()->OnAddTrack(NewMasterTrack);
	}

	check(NewTracks.Num() != 0);

	for (UMovieSceneFlowTrack* NewTrack : NewTracks)
	{
		CreateNewSection(NewTrack, 0, SectionType, false);
		NewTrack->SetDisplayName(LOCTEXT("TrackName", "Flow Events"));
	}
}

void FFlowTrackEditor::CreateNewSection(UMovieSceneTrack* Track, int32 RowIndex, UClass* SectionType, bool bSelect) const
{
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (SequencerPtr.IsValid())
	{
		UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
		const FQualifiedFrameTime CurrentTime = SequencerPtr->GetLocalTime();

		FScopedTransaction Transaction(LOCTEXT("CreateNewFlowSectionTransactionText", "Add Flow Section"));

		UMovieSceneSection* NewSection = NewObject<UMovieSceneSection>(Track, SectionType);
		check(NewSection);

		int32 OverlapPriority = 0;
		for (UMovieSceneSection* Section : Track->GetAllSections())
		{
			if (Section->GetRowIndex() >= RowIndex)
			{
				Section->SetRowIndex(Section->GetRowIndex() + 1);
			}
			OverlapPriority = FMath::Max(Section->GetOverlapPriority() + 1, OverlapPriority);
		}

		Track->Modify();

		if (SectionType == UMovieSceneFlowTriggerSection::StaticClass())
		{
			NewSection->SetRange(TRange<FFrameNumber>::All());
		}
		else
		{
			TRange<FFrameNumber> NewSectionRange;

			if (CurrentTime.Time.FrameNumber < FocusedMovieScene->GetPlaybackRange().GetUpperBoundValue())
			{
				NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, FocusedMovieScene->GetPlaybackRange().GetUpperBoundValue());
			}
			else
			{
				const float DefaultLengthInSeconds = 5.f;
				NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, CurrentTime.Time.FrameNumber + (DefaultLengthInSeconds * SequencerPtr->GetFocusedTickResolution()).FloorToFrame());
			}

			NewSection->SetRange(NewSectionRange);
		}

		NewSection->SetOverlapPriority(OverlapPriority);
		NewSection->SetRowIndex(RowIndex);

		Track->AddSection(*NewSection);
		Track->UpdateEasing();

		if (bSelect)
		{
			SequencerPtr->EmptySelection();
			SequencerPtr->SelectSection(NewSection);
			SequencerPtr->ThrobSectionSelection();
		}

		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

#undef LOCTEXT_NAMESPACE
