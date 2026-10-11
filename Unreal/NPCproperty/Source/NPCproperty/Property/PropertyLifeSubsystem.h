#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Property/PropertyLifeTypes.h"
#include "Property/PropertyHearingComponent.h"
#include "PropertyLifeSubsystem.generated.h"

class APropertyArea;
class APropertyAIController;
class APropertyWitness;
class USoundBase;

UCLASS()
class NPCPROPERTY_API UPropertyLifeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	UPROPERTY(BlueprintReadOnly, Category="Clock") float Hour = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock") float SecondsPerHour = 60.f;
	UPROPERTY(BlueprintReadOnly, Category="Justice") TArray<FOwnedItemRecord> ReturnedGoods;
	UFUNCTION(BlueprintCallable, Category="Clock") void SetHour(float Value);
	UFUNCTION(BlueprintPure, Category="Justice") bool IsSettled(FGuid IncidentId) const;
	UFUNCTION(BlueprintCallable, Category="Justice") bool ReportToGuard(APropertyWitness* Witness, APropertyWitness* Guard);
	UFUNCTION(BlueprintPure, Category="Justice") int32 GetBountyAmount(FName PersonId) const;
	UFUNCTION(BlueprintPure, Category="Justice") TArray<FTheftIncident> GetBountyCases(FName PersonId) const;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Justice") TArray<FPropertyBounty> Bounties;
	UFUNCTION(BlueprintCallable, Category="Communication") bool Communicate(APropertyWitness* Speaker, APropertyWitness* Listener);
	UFUNCTION(BlueprintCallable, Category="Awareness") int32 EmitNoise(AActor* Source, FVector Location, float Radius, EPropertyNoiseKind Kind, USoundBase* Sound = nullptr);
	UFUNCTION(BlueprintCallable, Category="Awareness") void EmitDisturbance(FVector Location, float Loudness = 800.f);
	UFUNCTION(BlueprintCallable, Category="Persistence") bool SaveProgress(const FString& Slot = TEXT("PropertyTown"));
	UFUNCTION(BlueprintCallable, Category="Persistence") bool LoadProgress(const FString& Slot = TEXT("PropertyTown"));
	bool ClaimResponse(FName SuspectId, APropertyAIController* Controller);
	bool InspectPossessions(APropertyWitness* Inspector, AActor* Player);
	bool IsMissingItemResolved(FGuid ItemId) const;
	void ReleaseResponse(APropertyAIController* Controller);
	bool OwnsResponse(FName SuspectId, APropertyAIController* Controller) const;
	void Settle(const TArray<FTheftIncident>& Cases);
	const TSet<FGuid>& GetSettled() const { return Settled; }
private:
	bool IssueBounty(const FTheftIncident& Incident, APropertyWitness* Witness, APropertyWitness* Guard);
	UPROPERTY() TSet<FGuid> Settled;
	UPROPERTY() TMap<TWeakObjectPtr<APropertyAIController>, FName> Responders;
	bool bRestoring = false;
};
