#include "Property/PropertyPlayerAnimInstance.h"
#include "GameFramework/Character.h"

void UPropertyPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const auto* Character = Cast<ACharacter>(TryGetPawnOwner());
	CrouchAlpha = FMath::FInterpTo(CrouchAlpha, Character && Character->bIsCrouched ? 1.f : 0.f, DeltaSeconds, 10.f);
}
