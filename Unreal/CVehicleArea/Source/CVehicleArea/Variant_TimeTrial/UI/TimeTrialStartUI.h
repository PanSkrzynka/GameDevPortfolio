// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeTrialStartUI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCountdownFinishedDelegate);

UCLASS(abstract)
class UTimeTrialStartUI : public UUserWidget
{
	GENERATED_BODY()

public:

	void StartCountdown();

protected:

	UFUNCTION(BlueprintImplementableEvent, Category="Countdown", meta = (DisplayName = "Start Countdown"))
	void BP_StartCountdown();

	UFUNCTION(BlueprintCallable, Category="Countdown")
	void FinishCountdown();

public:

	FCountdownFinishedDelegate OnCountdownFinished;

};
