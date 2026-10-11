#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Property/PropertyTypes.h"
#include "NPCKnowledgeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTheftWitnessed, FTheftIncident, Incident);

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UNPCKnowledgeComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNPCKnowledgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Witness")
	bool bAlert = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Witness", meta=(ClampMin="0.0"))
	float SightRange = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Witness", meta=(ClampMin="0.0", ClampMax="180.0"))
	float HalfVisionAngle = 70.f;

	UPROPERTY(BlueprintAssignable, Category="Witness")
	FTheftWitnessed OnTheftWitnessed;

	UFUNCTION(BlueprintPure, Category="Witness")
	TArray<FTheftIncident> GetKnownThefts() const { return KnownThefts; }
	const TArray<FTheftIncident>& GetKnownTheftsView() const { return KnownThefts; }
	int32 CountKnownThefts(FName PersonId) const;

	UFUNCTION(BlueprintPure, Category="Witness")
	bool CanWitness(AActor* Offender, AActor* Evidence) const;

	UFUNCTION(BlueprintPure, Category="Witness")
	bool CanSeeActor(AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category="Witness")
	AActor* GetObservedOffender(FGuid IncidentId) const;

	UFUNCTION(BlueprintPure, Category="Witness")
	TArray<FTheftIncident> GetUnsettledCases(FName PersonId) const;

	UFUNCTION(BlueprintPure, Category="Witness")
	FName GetEvidenceSource(FGuid IncidentId) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	friend class UPropertyLifeSubsystem;
	friend class UCrimeWorldSubsystem;
	void Remember(const FTheftIncident& Incident, AActor* Offender);

	UPROPERTY(Transient)
	TMap<FGuid, TWeakObjectPtr<AActor>> ObservedOffenders;

	UPROPERTY(Transient)
	TArray<FTheftIncident> KnownThefts;

	UPROPERTY(Transient)
	TMap<FGuid, FName> EvidenceSources;
};
