// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeTrialTrackGate.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "TimeTrialPlayerController.h"

ATimeTrialTrackGate::ATimeTrialTrackGate()
{
 	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);

	CollisionBox->SetBoxExtent(FVector(1000.0f));
	CollisionBox->SetLineThickness(32.0f);
	CollisionBox->bHiddenInGame = false;
	CollisionBox->SetCollisionProfileName(FName("OverlapAllDynamic"));

}

void ATimeTrialTrackGate::NotifyActorBeginOverlap(AActor* OtherActor)
{

	if (ATimeTrialPlayerController* PC = Cast<ATimeTrialPlayerController>(OtherActor->GetInstigatorController()))
	{

		if (PC->GetTargetGate() == this)
		{

			PC->SetTargetGate(NextMarker);

			if (bIsFinishLine)
			{
				PC->IncrementLapCount();
			}
		}
	}
}

ATimeTrialTrackGate* ATimeTrialTrackGate::GetNextMarker() const
{
	return NextMarker;
}
