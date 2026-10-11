#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Property/PropertyLifeTypes.h"
#include "PropertyInspectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPropertyObjectMissing, FPropertyObjectMemory, Object);

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UPropertyInspectionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPropertyInspectionComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection", meta=(ClampMin="100")) float InspectionRange = 1000.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Inspection") TArray<FPropertyObjectMemory> Memories;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Inspection") TArray<FPropertyMissingReport> Reports;
    UPROPERTY(BlueprintAssignable, Category="Inspection") FPropertyObjectMissing OnObjectMissing;
    UFUNCTION(BlueprintCallable, Category="Inspection") void InspectObjects();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    bool CanInspectLocation(FVector Location) const;
};
