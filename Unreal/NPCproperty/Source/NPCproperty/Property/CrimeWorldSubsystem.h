#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Property/PropertyTypes.h"
#include "CrimeWorldSubsystem.generated.h"

class UNPCKnowledgeComponent;
class UPropertyInteractionComponent;

UCLASS()
class NPCPROPERTY_API UCrimeWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	void RegisterWitness(UNPCKnowledgeComponent* Witness);
	void UnregisterWitness(UNPCKnowledgeComponent* Witness);
	const TArray<FTheftIncident>& GetIncidents() const { return Incidents; }

private:
	friend class UPropertyLifeSubsystem;
	friend class UPropertyInteractionComponent;
	void RecordTheft(const FOwnedItemRecord& Item, AActor* Offender, AActor* Evidence);

	UPROPERTY(Transient)
	TArray<FTheftIncident> Incidents;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UNPCKnowledgeComponent>> Witnesses;
};
