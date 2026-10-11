#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PropertyDialogueProfile.generated.h"

UENUM(BlueprintType)
enum class EPropertyDialogueCue : uint8
{
	RoutineStatus,
	WarningStatus,
	ChaseStatus,
	InvestigatingStatus,
	TrespassConfrontStatus,
	ReturnStatus,
	QuestioningStatus,
	TheftConfrontStatus,
	WatchExitStatus,
	InspectRoomStatus,
	SweepStatus,
	LostSuspectStatus,
	RecoveringStatus,
	RaiseAlarm,
	HearAlarm,
	HearNoise,
	MissingItem,
	RequestSearch,
	AcceptExitWatch,
	AcceptSearch,
	ReportTheft,
	AcceptReport,
	GoodsFound,
	DeclineInspection,
	ReleaseSuspect,
	StartQuestioning,
	GoodsFoundFeedback,
	CleanInspectionFeedback,
	QuestioningEndedFeedback,
	ResistFeedback,
	MissingGoodsFeedback,
	CannotPayFeedback,
	PaidFeedback,
	ReturnedFeedback,
	DefeatedFeedback,
	QuestionHeading,
	ResistHeading,
	TheftHeading,
	ExplainChoice,
	InspectChoice,
	DeclineChoice,
	ReturnChoice,
	PayChoice,
	ResistChoice,
	SurrenderChoice,
	StrikeAction,
};

UCLASS(BlueprintType)
class NPCPROPERTY_API UPropertyDialogueProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(MultiLine=true))
    TMap<EPropertyDialogueCue, FText> Lines;
};
