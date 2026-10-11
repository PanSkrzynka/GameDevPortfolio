#include "Property/PropertyIdentityComponent.h"
#include "GameFramework/Actor.h"

FPropertyIdentity UPropertyIdentityComponent::GetIdentity(const AActor* Actor)
{
	if (IsValid(Actor))
	{
		if (const auto* Component = Actor->FindComponentByClass<UPropertyIdentityComponent>())
		{
			return Component->Identity;
		}
	}
	return FPropertyIdentity();
}
