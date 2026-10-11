#include "Property/PropertyInteractionComponent.h"
#include "Property/OwnedPickup.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/CrimeWorldSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UPropertyInteractionComponent::UPropertyInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FVector2D UPropertyInteractionComponent::GetAimScreenPosition(FVector2D ViewportSize) const
{
	return FVector2D(ViewportSize.X * FMath::Clamp(CrosshairPosition.X, 0.f, 1.f), ViewportSize.Y * FMath::Clamp(CrosshairPosition.Y, 0.f, 1.f));
}

AOwnedPickup* UPropertyInteractionComponent::FindFocusedPickup() const
{
	if (!GetWorld() || !IsValid(GetOwner())) return nullptr;
	FVector ViewLocation;
	FVector ViewDirection;
	FRotator ViewRotation;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (const auto* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
	{
		int32 Width = 0;
		int32 Height = 0;
		Controller->GetViewportSize(Width, Height);
		const FVector2D Aim = GetAimScreenPosition(FVector2D(Width, Height));
		if (Width <= 0 || Height <= 0 || !Controller->DeprojectScreenPositionToWorld(Aim.X, Aim.Y, ViewLocation, ViewDirection)) return nullptr;
	}
	else
	{
		if (Pawn && Pawn->GetController()) Pawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
		else GetOwner()->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		ViewDirection = ViewRotation.Vector();
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PropertyFocus), false, GetOwner());
	GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + ViewDirection * AimDistance, ECC_Visibility, Params);
	AOwnedPickup* Pickup = Cast<AOwnedPickup>(Hit.GetActor());
	return IsValid(Pickup) && Pickup->IsAvailable() ? Pickup : nullptr;
}

EPropertyInteractionResult UPropertyInteractionComponent::Interact()
{
	return TryTake(FindFocusedPickup());
}

EPropertyInteractionResult UPropertyInteractionComponent::Finish(EPropertyInteractionResult Result, const FOwnedItemRecord& Item)
{
	OnInteractionFinished.Broadcast(Result, Item);
	return Result;
}

EPropertyInteractionResult UPropertyInteractionComponent::TryTake(AOwnedPickup* Pickup)
{

	if (bCommitting) return EPropertyInteractionResult::Unavailable;
	TGuardValue<bool> Guard(bCommitting, true);
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !GetWorld()) return Finish(EPropertyInteractionResult::Unavailable);
	if (!IsValid(Pickup)) return Finish(EPropertyInteractionResult::NoTarget);
	if (!Pickup->IsAvailable() || Pickup->GetWorld() != GetWorld()) return Finish(EPropertyInteractionResult::Unavailable);
	if (FVector::DistSquared(GetOwner()->GetActorLocation(), Pickup->GetActorLocation()) > FMath::Square(Reach)) return Finish(EPropertyInteractionResult::OutOfReach);
	if (Inventory.Num() >= Capacity) return Finish(EPropertyInteractionResult::InventoryFull);

	FVector EyeLocation;
	FRotator EyeRotation;
	GetOwner()->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PropertyReach), false, GetOwner());
	if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, Pickup->GetActorLocation(), ECC_Visibility, Params)
		&& Hit.GetActor() != Pickup) return Finish(EPropertyInteractionResult::Obstructed);

	const FPropertyIdentity Taker = UPropertyIdentityComponent::GetIdentity(GetOwner());
	FOwnedItemRecord Item;
	if (!Pickup->Consume(Taker, Item)) return Finish(EPropertyInteractionResult::Unavailable);
	Inventory.Add(Item);

	if (Item.bStolen)
	{
		GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->RecordTheft(Item, GetOwner(), Pickup);
	}
	if (IsValid(Pickup)) Pickup->Destroy();
	return Finish(Item.bStolen ? EPropertyInteractionResult::Stolen : EPropertyInteractionResult::Taken, Item);
}
