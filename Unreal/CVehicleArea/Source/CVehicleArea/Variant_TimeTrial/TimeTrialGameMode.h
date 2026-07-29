// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimeTrialGameMode.generated.h"

class ATimeTrialTrackGate;

UCLASS(abstract)
class ATimeTrialGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category="Time Trial")
	FName FinishTag;

	UPROPERTY(EditAnywhere, Category="Time Trial")
	int32 Laps = 3;

	TObjectPtr<ATimeTrialTrackGate> FinishLineMarker;

protected:

	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfLocalPlayers = 1;

	int32 CurrentPlayerStartAssignment = 0;

protected:

	virtual void BeginPlay() override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:

	ATimeTrialTrackGate* GetFinishLine() const;

	int32 GetLaps() const { return Laps; };

};
