#include "Property/PropertyTypes.h"

bool FPropertyPermission::Allows(const FPropertyIdentity& Visitor, const FPropertyIdentity& Owner) const
{
	return bPublic
		|| (!Visitor.PersonId.IsNone() && (Visitor.PersonId == Owner.PersonId || InvitedPeople.Contains(Visitor.PersonId)))
		|| (!Visitor.FactionId.IsNone() && AllowedFactions.Contains(Visitor.FactionId));
}
