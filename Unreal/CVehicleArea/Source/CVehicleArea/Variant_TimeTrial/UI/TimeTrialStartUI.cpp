// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeTrialStartUI.h"

void UTimeTrialStartUI::StartCountdown()
{

	BP_StartCountdown();
}

void UTimeTrialStartUI::FinishCountdown()
{

	OnCountdownFinished.Broadcast();
}
