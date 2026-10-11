#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PropertyHearingComponent.generated.h"

UENUM(BlueprintType)
enum class EPropertyNoiseKind : uint8
{
	Footstep,
	Landing,
	Impact,
	Distraction,
	Alarm
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPropertySoundHeard, FVector, Location, EPropertyNoiseKind, Kind, float, Suspicion);

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UPropertyHearingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPropertyHearingComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing") float HearingRange = 2400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing", meta=(ClampMin="0.1", ClampMax="4.0")) float HearingDistanceMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing") float OccludedMultiplier = .35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing") float InvestigationThreshold = .3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing") float DecayPerSecond = .08f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing", meta=(ClampMin="0.1")) float ReactionCooldown = .8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing", meta=(ClampMin="0.0")) float FootstepSensitivity = .8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearing", meta=(ClampMin="0.0")) float ImpactSensitivity = 1.2f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Hearing") float Suspicion = 0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Hearing") FVector LastHeardLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Hearing") EPropertyNoiseKind LastHeardKind = EPropertyNoiseKind::Footstep;
	UPROPERTY(BlueprintAssignable, Category="Hearing") FPropertySoundHeard OnSoundHeard;
	void Hear(FVector Location, float Strength, EPropertyNoiseKind Kind);
	void ResetHearing();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	float NextReactionAt = 0.f;
};
