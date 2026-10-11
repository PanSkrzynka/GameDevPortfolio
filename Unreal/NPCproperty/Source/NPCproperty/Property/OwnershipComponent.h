#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Property/PropertyTypes.h"
#include "OwnershipComponent.generated.h"

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UOwnershipComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ownership")
	EPropertyOwnershipMode OwnershipMode = EPropertyOwnershipMode::InheritRoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ownership", meta=(EditCondition="OwnershipMode == EPropertyOwnershipMode::ExplicitOwner", EditConditionHides))
	FPropertyIdentity LegalOwner;

	UFUNCTION(BlueprintPure, Category="Ownership")
	FPropertyIdentity GetLegalOwner() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ownership")
	FPropertyPermission TakePermission;

	UFUNCTION(BlueprintPure, Category="Ownership")
	bool CanTake(const FPropertyIdentity& Visitor) const;
};
