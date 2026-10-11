#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "NPCpropertyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USoundBase;
class UInputAction;
class UPropertyIdentityComponent;
class UPropertyInteractionComponent;
class UPropertyParticipantComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class ANPCpropertyCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Property")
	TObjectPtr<UPropertyParticipantComponent> PropertyParticipant;
	void ReturnGoods();
	void PayCompensation();
	void Resist();
	void Submit();
	void Attack();
	void Distract();
	void AdvanceTime();
	void SaveProgress();
	void LoadProgress();
	UFUNCTION(Exec) void Showcase(int32 Station);
	void NextShowcase();
	void PreviousShowcase();
	void StepShowcase(int32 Direction);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Property")
	TObjectPtr<UPropertyIdentityComponent> PropertyIdentity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Property")
	TObjectPtr<UPropertyInteractionComponent> PropertyInteraction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoInteract();

	ANPCpropertyCharacter();
	UFUNCTION(BlueprintCallable, Category="Stealth") void ToggleCrouch();
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Debug") bool bPropertyDebug = false;
	void TogglePropertyDebug() { bPropertyDebug = !bPropertyDebug; }
	void StartCrouch();
	void EndCrouch();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stealth") TObjectPtr<USoundBase> FootstepSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stealth") TObjectPtr<USoundBase> ImpactSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth") float StandingStepRadius = 650.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth") float CrouchedStepRadius = 140.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stealth") float LastNoiseRadius = 0.f;
	UFUNCTION(BlueprintPure, Category="Stealth") float GetFootstepRadius() const;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void BeginPlay() override;
private:
	FVector PreviousStepLocation = FVector::ZeroVector;
	float StepDistance = 0.f;
public:


protected:

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

public:

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

