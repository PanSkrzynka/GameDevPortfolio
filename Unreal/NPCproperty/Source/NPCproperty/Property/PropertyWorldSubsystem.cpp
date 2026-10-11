#include "Property/PropertyWorldSubsystem.h"
#include "Property/PropertyArea.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UPropertyWorldSubsystem::RegisterArea(APropertyArea* Area)
{
	if (IsValid(Area)) Areas.AddUnique(Area);
}

void UPropertyWorldSubsystem::UnregisterArea(APropertyArea* Area)
{
	Areas.Remove(Area);
}

APropertyArea* UPropertyWorldSubsystem::ResolveArea(FVector Location) const
{
	APropertyArea* Best = nullptr;
	double BestVolume = 0.0;
	const auto Consider = [&](APropertyArea* Area)
	{
		if (!IsValid(Area) || !Area->ContainsPoint(Location)) return;
		const double Volume = Area->GetBoundsVolume();
		const bool bEqualPriority = Best && Area->Priority == Best->Priority;
		if (!Best || Area->Priority > Best->Priority
			|| (bEqualPriority && Volume < BestVolume)
			|| (bEqualPriority && Volume == BestVolume
				&& Area->GetPathName() < Best->GetPathName()))
		{
			Best = Area;
			BestVolume = Volume;
		}
	};
	for (const auto& Entry : Areas) Consider(Entry.Get());
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		for (TActorIterator<APropertyArea> It(GetWorld()); It; ++It) Consider(*It);
	}
	return Best;
}

void UPropertyWorldSubsystem::AdvanceAlerts(float DeltaTime)
{
	for (const auto& Entry : Areas)
		if (auto* Area = Entry.Get()) Area->AdvanceAlert(DeltaTime);
}
