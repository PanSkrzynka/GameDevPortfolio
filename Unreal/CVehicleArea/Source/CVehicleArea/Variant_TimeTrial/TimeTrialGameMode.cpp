// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeTrialGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "TimeTrialTrackGate.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"

void ATimeTrialGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> ActorList;

	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ATimeTrialTrackGate::StaticClass(), FinishTag, ActorList);

	if (ActorList.Num() > 0)
	{

		FinishLineMarker = Cast<ATimeTrialTrackGate>(ActorList[0]);
	}

	for (int32 i = 2; i <= NumberOfLocalPlayers; ++i)
	{
		UGameplayStatics::CreatePlayer(GetWorld(), -1, true);
	}

}

AActor* ATimeTrialGameMode::ChoosePlayerStart_Implementation(AController* Player)
{

	FName PlayerTag = FName(*FString::Printf(TEXT("Player%d"), CurrentPlayerStartAssignment));

	TArray<AActor*> PlayerStarts;

	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	++CurrentPlayerStartAssignment;

	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	if (!PlayerStarts.IsEmpty())
	{
		return PlayerStarts[ FMath::RandRange(0, PlayerStarts.Num() - 1) ];
	}

	return nullptr;
}

ATimeTrialTrackGate* ATimeTrialGameMode::GetFinishLine() const
{
	return FinishLineMarker;
}
