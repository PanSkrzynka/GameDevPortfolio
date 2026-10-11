#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PropertyPlayerAnimInstance.generated.h"

UCLASS(Blueprintable)
class NPCPROPERTY_API UPropertyPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category="Stealth") float CrouchAlpha = 0.f;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
