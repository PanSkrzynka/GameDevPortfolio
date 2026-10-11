#include "Property/OwnershipComponent.h"
#include "Property/PropertyWorldSubsystem.h"
#include "Property/PropertyArea.h"
#include "Engine/World.h"

FPropertyIdentity UOwnershipComponent::GetLegalOwner() const
{
	if (OwnershipMode == EPropertyOwnershipMode::ExplicitOwner) return LegalOwner;
	if (OwnershipMode == EPropertyOwnershipMode::InheritRoom && GetOwner() && GetWorld())
	{
		const auto* Registry = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>();
		const auto* Area = Registry ? Registry->ResolveArea(GetOwner()->GetActorLocation()) : nullptr;
		if (Area) return Area->LegalOwner;
	}
	return {};
}

bool UOwnershipComponent::CanTake(const FPropertyIdentity& Visitor) const
{
	const FPropertyIdentity ResolvedOwner = GetLegalOwner();
	const bool bUnowned = ResolvedOwner.PersonId.IsNone() && ResolvedOwner.FactionId.IsNone();
	return bUnowned || TakePermission.Allows(Visitor, ResolvedOwner);
}
