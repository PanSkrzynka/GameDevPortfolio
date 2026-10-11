#include "Property/CrimeWorldSubsystem.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyIdentityComponent.h"
#include "Engine/World.h"

void UCrimeWorldSubsystem::RegisterWitness(UNPCKnowledgeComponent* Witness)
{
	if (IsValid(Witness)) Witnesses.AddUnique(Witness);
}

void UCrimeWorldSubsystem::UnregisterWitness(UNPCKnowledgeComponent* Witness)
{
	Witnesses.Remove(Witness);
}

void UCrimeWorldSubsystem::RecordTheft(const FOwnedItemRecord& Item, AActor* Offender, AActor* Evidence)
{
	if (!Item.bStolen || !IsValid(Offender) || !IsValid(Evidence)) return;
	FTheftIncident Incident;
	Incident.IncidentId = FGuid::NewGuid();
	Incident.Item = Item;
	Incident.Offender = UPropertyIdentityComponent::GetIdentity(Offender);
	Incident.Location = Evidence->GetActorLocation();
	Incident.WorldTime = GetWorld()->GetTimeSeconds();
	Incidents.Add(Incident);

	TArray<TWeakObjectPtr<UNPCKnowledgeComponent>> Observers;
	for (const auto& Entry : Witnesses)
	{
		if (auto* Witness = Entry.Get(); IsValid(Witness) && Witness->CanWitness(Offender, Evidence)) Observers.Add(Witness);
	}
	for (const auto& Entry : Observers)
	{
		if (auto* Witness = Entry.Get(); IsValid(Witness)) Witness->Remember(Incident, Offender);
	}
}
