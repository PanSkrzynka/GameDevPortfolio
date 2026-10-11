#include "TimeTrialStartUI.h"

void UTimeTrialStartUI::StartCountdown()
{

	BP_StartCountdown();
}

void UTimeTrialStartUI::FinishCountdown()
{

	OnCountdownFinished.Broadcast();
}
