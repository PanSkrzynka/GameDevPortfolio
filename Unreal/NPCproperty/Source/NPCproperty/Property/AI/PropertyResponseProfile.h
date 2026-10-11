#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PropertyResponseProfile.generated.h"

UENUM(BlueprintType)
enum class EPropertyResponseState : uint8
{
	Routine,
	Warn,
	Chase,
	Search,
	Confront,
	Return
};

UCLASS(BlueprintType)
class NPCPROPERTY_API UPropertyResponseProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jurisdiction")
	bool bProtectAllProperty = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="1.0")) float TurnRate = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0.0", ClampMax="90.0")) float LookAroundAngle = 55.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0.1")) float LookAroundSpeed = .7f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.1")) float NoiseRetargetCooldown = 1.f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.1"))
	float WarningSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.1"))
	float LostSightSeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.5"))
	float SearchSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.1"))
	float SearchPointPauseSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="1.0"))
	float MaxPursuitSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.1"))
	float ConfrontSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0"))
	float ReengagementDelay = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="1.0"))
	float WalkSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="1.0"))
	float ChaseSpeed = 440.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="1.0"))
	float ConfrontDistance = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="1.0"))
	float SearchRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="1.0"))
	float LeashDistance = 2600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="100.0"))
	float ChaseBreakOffDistance = 3000.f;
};
