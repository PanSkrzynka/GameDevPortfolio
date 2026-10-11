#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Property/PropertyTypes.h"
#include "PropertyIdentityComponent.generated.h"

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UPropertyIdentityComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Property")
	FPropertyIdentity Identity;

	UFUNCTION(BlueprintPure, Category="Property", meta=(DefaultToSelf="Actor"))
	static FPropertyIdentity GetIdentity(const AActor* Actor);
};
