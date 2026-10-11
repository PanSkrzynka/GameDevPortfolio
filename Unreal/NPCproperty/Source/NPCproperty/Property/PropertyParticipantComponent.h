#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Property/PropertyLifeTypes.h"
#include "PropertyParticipantComponent.generated.h"

class APropertyAIController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPropertyPlayerFeedback, FText, Message);

UCLASS(ClassGroup=(Property), meta=(BlueprintSpawnableComponent))
class NPCPROPERTY_API UPropertyParticipantComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPropertyParticipantComponent();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") int32 Gold = 300;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") float Health = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") FText Feedback;
	UPROPERTY(BlueprintAssignable, Category="Status") FPropertyPlayerFeedback OnFeedback;
	UFUNCTION(BlueprintPure, Category="Confrontation") APropertyAIController* GetConfrontation() const;
	UFUNCTION(BlueprintCallable, Category="Confrontation") bool Choose(EConfrontationChoice Choice);
	UFUNCTION(BlueprintCallable, Category="Combat") void Attack();
	UFUNCTION(BlueprintCallable, Category="Awareness") void Distract();
	UFUNCTION(BlueprintCallable, Category="Clock") void AdvanceTime();
	UFUNCTION(BlueprintCallable, Category="Persistence") void Save();
	UFUNCTION(BlueprintCallable, Category="Persistence") void Load();
	void Notify(const FString& Message);
	void Restore(int32 InGold, float InHealth);
	bool Spend(int32 Amount);
	void Recover();
protected:
	virtual void BeginPlay() override;
private:
	float NextAttack = 0.f;
	float NextNoise = 0.f;
	UFUNCTION() void TakeDamage(AActor* Actor, float Damage, const UDamageType* Type, AController* Instigator, AActor* Causer);
};
