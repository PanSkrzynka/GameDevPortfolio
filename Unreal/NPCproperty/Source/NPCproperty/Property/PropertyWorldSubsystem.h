#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PropertyWorldSubsystem.generated.h"

class APropertyArea;

UCLASS()
class NPCPROPERTY_API UPropertyWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	void RegisterArea(APropertyArea* Area);
	void UnregisterArea(APropertyArea* Area);
	void AdvanceAlerts(float DeltaTime);

	UFUNCTION(BlueprintPure, Category="Property")
	APropertyArea* ResolveArea(FVector Location) const;

private:
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APropertyArea>> Areas;
};
