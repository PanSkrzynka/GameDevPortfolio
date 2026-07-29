// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeTrialTrackGate.generated.h"

class UBoxComponent;

UCLASS(abstract)
class ATimeTrialTrackGate : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionBox;

protected:

	UPROPERTY(EditAnywhere, Category="Track Gate")
	bool bIsFinishLine = false;

	UPROPERTY(EditAnywhere, Category="Track Gate")
	ATimeTrialTrackGate* NextMarker;

public:

	ATimeTrialTrackGate();

protected:

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

public:

	ATimeTrialTrackGate* GetNextMarker() const;
};
