#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Property/PropertyInteractionComponent.h"
#include "PropertySandboxHUD.generated.h"

class UPropertyParticipantComponent;

UCLASS()
class NPCPROPERTY_API APropertySandboxHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPropertyInteractionComponent> BoundInteraction;

	UPROPERTY(Transient)
	TObjectPtr<UPropertyParticipantComponent> BoundParticipant;

	FString Feedback;
	float FeedbackUntil = 0.f;
	UFUNCTION()
	void HandleInteraction(EPropertyInteractionResult Result, FOwnedItemRecord Item);

	UFUNCTION()
	void HandleFeedback(FText Message);
};
