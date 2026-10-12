#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Property/PropertyTypes.h"
#include "Property/AI/PropertyResponseProfile.h"
#include "Property/PropertyLifeTypes.h"
#include "PropertyAIController.generated.h"

class APropertyArea;
class UStateTreeAIComponent;
class UAISenseConfig_Sight;
class APropertyWitness;

UCLASS(Blueprintable)
class NPCPROPERTY_API APropertyAIController : public AAIController
{
	GENERATED_BODY()
public:
	APropertyAIController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTree;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Response")
	EPropertyResponseState DesiredResponse = EPropertyResponseState::Routine;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Response")
	EPropertyResponseState ActiveResponse = EPropertyResponseState::Routine;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Response")
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Response")
	FVector SearchDestination = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Response")
	FVector HomeLocation = FVector::ZeroVector;

	UFUNCTION(BlueprintPure, Category = "Response")
	AActor* GetSuspect() const
	{
		return Suspect.Get();
	}

	UFUNCTION(BlueprintPure, Category = "Response")
	bool HasVisibleSuspect() const;

	UFUNCTION(BlueprintPure, Category = "Response")
	bool IsEscortingTrespasser() const
	{
		return Suspect.IsValid() && !bPursuingOffender && !bQuestioning && !bResisting;
	}

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Response")
	bool bResisting = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Routine")
	FText CurrentActivity;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Routine")
	int32 RoutineEntry = INDEX_NONE;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Routine")
	int32 Waypoint = 0;
	UFUNCTION(BlueprintPure, Category = "Confrontation")
	int32 GetCompensation() const;
	UFUNCTION(BlueprintCallable, Category = "Confrontation")
	bool ResolveConfrontation(AActor* Player, EConfrontationChoice Choice);
	UFUNCTION(BlueprintPure, Category = "Response")
	bool IsGuard() const;
	UFUNCTION(BlueprintPure, Category = "Response")
	bool CanEnforceAt(FVector Location) const;
	bool CanEnforceArea(const APropertyArea* Area) const;
	void RespondToCrimeReport();
	bool Investigate(FVector Location, bool bAlarm = false);
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Search")
	EPropertySearchRole SearchRole = EPropertySearchRole::None;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Search")
	TWeakObjectPtr<APropertyArea> SearchArea;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Search")
	int32 SearchSlot = INDEX_NONE;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Questioning")
	bool bQuestioning = false;
	UFUNCTION(BlueprintCallable, Category = "Questioning")
	bool TryQuestion(AActor* Player);
	void AssignSearch(APropertyArea* Area, FVector Location);

	void ResetAfterLoad(const FPropertySavedNPC& Snapshot);
	FRotator GetHomeRotation() const
	{
		return HomeRotation;
	}
	float GetRemainingRecovery() const;
	void HandleDefeat();
	void TargetDefeated();

	void EnterResponse(EPropertyResponseState State);
	void AdvanceResponse(float DeltaTime);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<APropertyWitness> Resident;

	UPROPERTY(Transient)
	TObjectPtr<UPropertyResponseProfile> Profile;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> Suspect;

	FTimerHandle AwarenessTimer;
	FRotator HomeRotation;
	float ResponseStartedAt = 0.f;
	float PursuitStartedAt = 0.f;
	float SuspectLastSeenAt = 0.f;
	float IgnoreNewResponsesUntil = 0.f;
	float NextMovementUpdateAt = 0.f;
	float ResponseUpdateElapsed = 0.f;
	int32 SearchPointIndex = 0;
	float SearchStartedAt = -1.f;
	float SearchPauseEndsAt = -1.f;
	bool bPursuingOffender = false;
	bool bPathRequestFailed = false;
	bool bInvestigatingLocation = false;
	float NextEvidenceReportAt = 0.f;
	float NextAttackAt = 0.f;
	float PatrolWaitUntil = 0.f;
	float RecoveryEndsAt = 0.f;
	float NextNoiseInvestigationAt = 0.f;
	float NextQuestionAt = 0.f;
	float ScanBaseYaw = 0.f;
	float ScanStartedAt = 0.f;
	bool bShouldFaceHomeDirection = false;
	void LookAround(float CurrentTime);
	void AdvanceRoutine(float CurrentTime);
	void ShareEvidence(bool bGuardsOnly = false);
	bool StartResponse(AActor* Actor, bool bCrime);
	TArray<FTheftIncident> GetActionableCases(FName PersonId) const;
	void VisitActionableCases(FName PersonId, TFunctionRef<bool(const FTheftIncident&)> Visitor) const;
	bool HasActionableCases(FName PersonId) const;
	int32 CountActionableCases(FName PersonId) const;
	void AdvanceTrespassFollow(float CurrentTime, float TimeInState);
	void AdvanceWarning(float CurrentTime, float TimeInState);

	UFUNCTION()
	void HandleTheft(FTheftIncident Incident);

	void EvaluateAwareness();
	bool IsProtectedTrespasser(AActor* Actor) const;
	void RequestReturn();
	void MoveToSearchPoint();
	void Cleanup();
};
