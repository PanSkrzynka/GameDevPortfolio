#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Property/PropertyTypes.h"
#include "Property/PropertyLifeTypes.h"
#include "PropertyArea.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class NPCPROPERTY_API APropertyArea : public AActor
{
	GENERATED_BODY()
public:
	APropertyArea();
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Alert") float AlertScore = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Alert") float QuietSeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alert") float AlertDecayDelay = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alert") float AlertDecayRate = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Search", meta=(MakeEditWidget=true)) FVector ExitOffset = FVector(0, -450, 0);
    UPROPERTY() TSet<FGuid> AlertedItems;
    UPROPERTY() TSet<FGuid> ReportedItems;
    UFUNCTION(BlueprintPure, Category="Alert") EPropertyAlertLevel GetAlertLevel() const;
    UFUNCTION(BlueprintCallable, Category="Alert") void RegisterIncident(FGuid ItemId, bool bWitnessed = false);
    UFUNCTION(BlueprintCallable, Category="Alert") void RegisterGuardReport(FGuid ItemId);
    void AdvanceAlert(float DeltaTime);
    UFUNCTION(BlueprintPure, Category="Search") FVector GetExitLocation() const;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Property")
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Property")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Property")
	FPropertyIdentity LegalOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Property")
	FPropertyPermission EntryPermission;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Property")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Property|Room Assignment")
	TArray<FName> AssignedNPCs;

	UFUNCTION(BlueprintPure, Category="Property")
	bool IsAssigned(FName PersonId) const;

	UFUNCTION(BlueprintPure, Category="Property")
	bool ContainsPoint(FVector WorldPoint) const;

	UFUNCTION(BlueprintPure, Category="Property")
	bool CanEnter(const FPropertyIdentity& Visitor) const;

	double GetBoundsVolume() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
