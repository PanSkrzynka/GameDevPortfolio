#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Property/PropertyTypes.h"
#include "OwnedPickup.generated.h"

class UOwnershipComponent;
class UStaticMeshComponent;
class UPropertyInteractionComponent;

UCLASS(Blueprintable)
class NPCPROPERTY_API AOwnedPickup : public AActor
{
	GENERATED_BODY()
public:
	AOwnedPickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UOwnershipComponent> Ownership;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FText DisplayName;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Item")
	FGuid ItemId;

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsAvailable() const { return bAvailable; }
	virtual void PostActorCreated() override;
#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif

protected:
	virtual void BeginPlay() override;

private:
	friend class UPropertyInteractionComponent;

	bool Consume(const FPropertyIdentity& Taker, FOwnedItemRecord& OutItem);
	bool bAvailable = true;
};
