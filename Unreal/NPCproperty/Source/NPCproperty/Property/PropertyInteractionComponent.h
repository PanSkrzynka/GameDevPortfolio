#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Property/PropertyTypes.h"
#include "PropertyInteractionComponent.generated.h"

class AOwnedPickup;

UENUM(BlueprintType)
enum class EPropertyInteractionResult : uint8
{
	Taken,
	Stolen,
	NoTarget,
	OutOfReach,
	Obstructed,
	InventoryFull,
	Unavailable
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPropertyInteractionFinished, EPropertyInteractionResult, Result, FOwnedItemRecord, Item);

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UPropertyInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPropertyInteractionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="1.0"))
	float Reach = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="1.0"))
	float AimDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FVector2D CrosshairPosition = FVector2D(.5f, .4f);

	UFUNCTION(BlueprintPure, Category="Interaction")
	FVector2D GetAimScreenPosition(FVector2D ViewportSize) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ClampMin="1"))
	int32 Capacity = 20;

	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FPropertyInteractionFinished OnInteractionFinished;

	UFUNCTION(BlueprintPure, Category="Interaction")
	AOwnedPickup* FindFocusedPickup() const;

	UFUNCTION(BlueprintCallable, Category="Interaction")
	EPropertyInteractionResult Interact();

	UFUNCTION(BlueprintCallable, Category="Interaction")
	EPropertyInteractionResult TryTake(AOwnedPickup* Pickup);

	UFUNCTION(BlueprintPure, Category="Inventory")
	TArray<FOwnedItemRecord> GetInventory() const { return Inventory; }

	int32 GetItemCount() const { return Inventory.Num(); }

private:
	friend class UPropertyLifeSubsystem;
	friend class APropertyAIController;
	UPROPERTY(Transient)
	TArray<FOwnedItemRecord> Inventory;

	bool bCommitting = false;
	EPropertyInteractionResult Finish(EPropertyInteractionResult Result, const FOwnedItemRecord& Item = FOwnedItemRecord());
};
