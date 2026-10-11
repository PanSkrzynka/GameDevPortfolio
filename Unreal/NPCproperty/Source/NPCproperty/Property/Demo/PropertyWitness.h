#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Property/PropertyTypes.h"
#include "Property/PropertyDialogueProfile.h"
#include "Property/PropertyLifeTypes.h"
#include "Property/PropertyHearingComponent.h"
#include "Property/AI/PropertyResponseProfile.h"
#include "PropertyWitness.generated.h"

class UPropertyIdentityComponent;
class UNPCKnowledgeComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UArrowComponent;
class UStateTree;
class UPropertyRoutine;
class UPropertyParticipantComponent;
class UAnimSequenceBase;
class UAnimMontage;
class UPropertyHearingComponent;
class USoundBase;
class UPropertyInspectionComponent;

UCLASS(Blueprintable)
class NPCPROPERTY_API APropertyWitness : public ACharacter
{
	GENERATED_BODY()
public:
	APropertyWitness();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TObjectPtr<UPropertyDialogueProfile> DialogueProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(MultiLine=true)) TMap<EPropertyDialogueCue, FText> DialogueOverrides;
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Dialogue") FText GetDialogue(EPropertyDialogueCue Cue) const;
    virtual FText GetDialogue_Implementation(EPropertyDialogueCue Cue) const;
    UFUNCTION(BlueprintCallable, Category="Dialogue") void SayCue(EPropertyDialogueCue Cue);

	UFUNCTION(BlueprintPure, Category="Response") FText GetResponseDescription(EPropertyResponseState State) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Response")
	EPropertyDialogueCue SelectResponseCue(EPropertyResponseState State, EPropertySearchRole SearchAssignment, bool bQuestioning, bool bHasCrime, bool bHasSuspect) const;
	virtual EPropertyDialogueCue SelectResponseCue_Implementation(EPropertyResponseState State, EPropertySearchRole SearchAssignment, bool bQuestioning, bool bHasCrime, bool bHasSuspect) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Response")
	float SelectResponseSpeed(EPropertyResponseState State, float WalkSpeed, float ChaseSpeed) const;
	virtual float SelectResponseSpeed_Implementation(EPropertyResponseState State, float WalkSpeed, float ChaseSpeed) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Communication")
	EPropertyDialogueCue SelectNoiseCue(EPropertyNoiseKind Kind) const;
	virtual EPropertyDialogueCue SelectNoiseCue_Implementation(EPropertyNoiseKind Kind) const;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Awareness") TObjectPtr<UPropertyInspectionComponent> Inspection;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Awareness") TObjectPtr<UPropertyHearingComponent> Hearing;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Communication") TObjectPtr<UTextRenderComponent> SpeechLabel;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Communication") TObjectPtr<USoundBase> AlarmSound;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Communication") FText LastSpeech;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Communication") float SpeechUntil = 0.f;
	UFUNCTION(BlueprintCallable, Category="Communication") void Say(FText Message);
	UFUNCTION(BlueprintCallable, Category="Communication") bool RaiseAlarm(FVector EvidenceLocation);
	UFUNCTION(BlueprintImplementableEvent, Category="Communication") void ReceiveSpeech(const FText& Message);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimSequenceBase> AttackAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimSequenceBase> HitReactionAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimSequenceBase> KnockdownAnimation;

	UFUNCTION(BlueprintCallable, Category="Animation")
	bool PlayAttackAnimation();

	void PlayDamageAnimation();
	void RestoreAnimationPose();

private:
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> KnockdownMontage;
	void PlayKnockdown(bool bRestore);

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
	TObjectPtr<UPropertyRoutine> Routine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
	TObjectPtr<UPropertyParticipantComponent> Participant;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Response")
	TObjectPtr<UPropertyResponseProfile> ResponseProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Response")
	TObjectPtr<UStateTree> ResponseTree;

	UFUNCTION(BlueprintImplementableEvent, Category="Response")
	void ReceiveResponseChanged(EPropertyResponseState State);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Witness")
	TObjectPtr<UPropertyIdentityComponent> Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Witness")
	TObjectPtr<UNPCKnowledgeComponent> Knowledge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Witness")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Witness")
	TObjectPtr<UTextRenderComponent> StatusLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Witness")
	TObjectPtr<UArrowComponent> FacingArrow;

	UFUNCTION(BlueprintImplementableEvent, Category="Witness")
	void ReceiveTheftWitnessed(const FTheftIncident& Incident);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTheft(FTheftIncident Incident);
	FTimerHandle SpeechTimer;
	float NextAlarm = 0.f;
	void ClearSpeech();
};

