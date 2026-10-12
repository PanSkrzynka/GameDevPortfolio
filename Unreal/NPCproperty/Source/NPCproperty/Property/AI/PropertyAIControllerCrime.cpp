#include "Property/AI/PropertyAIController.h"
#include "Property/AI/PropertyAIConstants.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyIdentityComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyInspectionComponent.h"
#include "Property/PropertyParticipantComponent.h"
#include "Property/PropertyInteractionComponent.h"
#include "Components/TextRenderComponent.h"

using namespace PropertyAIConstants;

int32 APropertyAIController::GetCompensation() const
{
	if (!Resident || !Suspect.IsValid())
	{
		return 0;
	}
	const FName PersonId = UPropertyIdentityComponent::GetIdentity(Suspect.Get()).PersonId;
	const int32 UnsettledTheftCount = CountActionableCases(PersonId);
	if (UnsettledTheftCount == 0)
	{
		return 0;
	}
	if (IsGuard())
	{
		return UnsettledTheftCount * CompensationPerTheft;
	}
	const int32 KnownTheftCount = Resident->Knowledge->CountKnownThefts(PersonId);
	return UnsettledTheftCount * CompensationPerTheft + FMath::Max(0, KnownTheftCount - 1) * RepeatOffenceSurcharge;
}

bool APropertyAIController::ResolveConfrontation(AActor* Player, EConfrontationChoice Choice)
{
	if (Choice > EConfrontationChoice::Submit)
	{
		return false;
	}
	if (!IsValid(Player) || Player != Suspect.Get() || !HasAuthority() || !Resident || !HasVisibleSuspect() ||
	    ActiveResponse != EPropertyResponseState::Confront || DesiredResponse != EPropertyResponseState::Confront ||
	    FVector::DistSquared(Resident->GetActorLocation(), Player->GetActorLocation()) >
	        FMath::Square(Profile->ConfrontDistance * ConfrontBreakOffDistanceScale))
	{
		return false;
	}
	if (!GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->OwnsResponse(
	        UPropertyIdentityComponent::GetIdentity(Player).PersonId, this))
	{
		return false;
	}
	auto* Participant = Player->FindComponentByClass<UPropertyParticipantComponent>();
	auto* Inventory = Player->FindComponentByClass<UPropertyInteractionComponent>();
	if (!Participant || !Inventory || Inventory->bCommitting)
	{
		return false;
	}
	if (IsEscortingTrespasser())
	{
		return false;
	}
	if (bQuestioning)
	{
		if (Choice == EConfrontationChoice::Submit)
		{
			return false;
		}
		NextQuestionAt = GetWorld()->GetTimeSeconds() + QuestioningCooldownSeconds;
		if (Choice == EConfrontationChoice::PayCompensation &&
		    GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->InspectPossessions(Resident, Player))
		{
			Resident->Say(Resident->GetDialogue(EPropertyDialogueCue::GoodsFound));
			bQuestioning = false;
			bPursuingOffender = true;
			Resident->ReceiveResponseChanged(EPropertyResponseState::Confront);
			Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::GoodsFoundFeedback).ToString());
			return true;
		}
		const FName Person = UPropertyIdentityComponent::GetIdentity(Player).PersonId;
		for (auto& Report : Resident->Inspection->Reports)
		{
			if (Report.PossiblePerson == Person)
			{
				Report.PossiblePerson = NAME_None;
			}
		}
		Resident->SayCue(Choice == EConfrontationChoice::Resist ? EPropertyDialogueCue::DeclineInspection
		                                                        : EPropertyDialogueCue::ReleaseSuspect);
		Participant->Notify(Resident
		                        ->GetDialogue(Choice == EConfrontationChoice::PayCompensation
		                                          ? EPropertyDialogueCue::CleanInspectionFeedback
		                                          : EPropertyDialogueCue::QuestioningEndedFeedback)
		                        .ToString());
		RequestReturn();
		return true;
	}
	TGuardValue<bool> Transaction(Inventory->bCommitting, true);
	if (Choice == EConfrontationChoice::Resist)
	{
		if (bResisting)
		{
			return false;
		}
		bResisting = true;
		NextAttackAt = GetWorld()->GetTimeSeconds() + AttackIntervalSeconds;
		Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::ResistFeedback).ToString());
		return true;
	}
	const auto TheftCases = GetActionableCases(UPropertyIdentityComponent::GetIdentity(Player).PersonId);
	TSet<FGuid> StolenItemIds;
	for (const auto& TheftIncident : TheftCases)
	{
		StolenItemIds.Add(TheftIncident.Item.ItemId);
	}
	if (Choice == EConfrontationChoice::ReturnGoods)
	{
		for (const FGuid& ItemId : StolenItemIds)
		{
			if (!Inventory->Inventory.ContainsByPredicate([&](const FOwnedItemRecord& Item)
			                                              { return Item.ItemId == ItemId; }))
			{
				Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::MissingGoodsFeedback).ToString());
				return false;
			}
		}
	}
	const int32 CompensationCost = GetCompensation();
	if (Choice == EConfrontationChoice::PayCompensation && !Participant->Spend(CompensationCost))
	{
		Participant->Notify(FText::Format(Resident->GetDialogue(EPropertyDialogueCue::CannotPayFeedback),
		                                  FFormatNamedArguments{{TEXT("Amount"), FText::AsNumber(CompensationCost)}})
		                        .ToString());
		return false;
	}
	auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	if (Choice == EConfrontationChoice::PayCompensation)
	{
		for (auto& Item : Inventory->Inventory)
		{
			if (StolenItemIds.Contains(Item.ItemId))
			{
				Item.bStolen = false;
				Item.LegalOwner = UPropertyIdentityComponent::GetIdentity(Player);
			}
		}
	}
	else
	{
		for (const auto& Item : Inventory->Inventory)
		{
			if (StolenItemIds.Contains(Item.ItemId))
			{
				auto Returned = Item;
				Returned.bStolen = false;
				LifeSubsystem->ReturnedGoods.Add(Returned);
			}
		}
		Inventory->Inventory.RemoveAll([&](const FOwnedItemRecord& Item)
		                               { return StolenItemIds.Contains(Item.ItemId); });
		if (Choice == EConfrontationChoice::Submit)
		{
			Participant->Spend(FMath::Min(Participant->Gold, CompensationCost));
		}
	}
	LifeSubsystem->Settle(TheftCases);
	Participant->Notify(Resident
	                        ->GetDialogue(Choice == EConfrontationChoice::PayCompensation
	                                          ? EPropertyDialogueCue::PaidFeedback
	                                          : EPropertyDialogueCue::ReturnedFeedback)
	                        .ToString());
	RequestReturn();
	return true;
}

void APropertyAIController::TargetDefeated()
{
	AActor* Target = Suspect.Get();
	if (ResolveConfrontation(Target, EConfrontationChoice::Submit))
	{
		if (auto* Participant = Target->FindComponentByClass<UPropertyParticipantComponent>())
		{
			Participant->Recover();
			Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::DefeatedFeedback).ToString());
		}
	}
}

void APropertyAIController::HandleDefeat()
{
	RequestReturn();
	StopMovement();
	RecoveryEndsAt = GetWorld()->GetTimeSeconds() + DefeatRecoverySeconds;
	IgnoreNewResponsesUntil = RecoveryEndsAt + Profile->ReengagementDelay;
	Resident->StatusLabel->SetText(Resident->GetDialogue(EPropertyDialogueCue::RecoveringStatus));
}

void APropertyAIController::RespondToCrimeReport()
{
	if (!Resident || !Profile || !Resident->Knowledge->bAlert || Resident->Participant->Health <= 0.f)
	{
		return;
	}
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < RecoveryEndsAt || CurrentTime < IgnoreNewResponsesUntil ||
	    (Suspect.IsValid() && bPursuingOffender))
	{
		return;
	}
	for (TActorIterator<APawn> PlayerIterator(GetWorld()); PlayerIterator; ++PlayerIterator)
	{
		if (!PlayerIterator->IsPlayerControlled() || !Resident->Knowledge->CanSeeActor(*PlayerIterator))
		{
			continue;
		}
		if (HasActionableCases(UPropertyIdentityComponent::GetIdentity(*PlayerIterator).PersonId) &&
		    StartResponse(*PlayerIterator, true))
		{
			return;
		}
	}
}

void APropertyAIController::ShareEvidence(bool bGuardsOnly)
{
	auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	if (Resident->Knowledge->GetKnownTheftsView().IsEmpty() && Resident->Inspection->Reports.IsEmpty() &&
	    (!IsGuard() || LifeSubsystem->Bounties.IsEmpty()))
	{
		return;
	}
	for (TActorIterator<APropertyWitness> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		if (*ActorIterator == Resident)
		{
			continue;
		}
		const auto* ListenerController = Cast<APropertyAIController>(ActorIterator->GetController());
		if (bGuardsOnly && (!ListenerController || !ListenerController->IsGuard()))
		{
			continue;
		}
		LifeSubsystem->Communicate(Resident, *ActorIterator);
	}
}

TArray<FTheftIncident> APropertyAIController::GetActionableCases(FName PersonId) const
{
	TArray<FTheftIncident> TheftCases;
	VisitActionableCases(PersonId,
	                     [&](const FTheftIncident& TheftIncident)
	                     {
		                     TheftCases.Add(TheftIncident);
		                     return true;
	                     });
	return TheftCases;
}

void APropertyAIController::VisitActionableCases(FName PersonId,
                                                 TFunctionRef<bool(const FTheftIncident&)> Visitor) const
{
	if (!Resident || PersonId.IsNone())
	{
		return;
	}
	const auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	const auto& KnownThefts = Resident->Knowledge->GetKnownTheftsView();
	for (const auto& TheftIncident : KnownThefts)
	{
		if (TheftIncident.Offender.PersonId == PersonId && !LifeSubsystem->IsSettled(TheftIncident.IncidentId) &&
		    CanEnforceAt(TheftIncident.Location) && !Visitor(TheftIncident))
		{
			return;
		}
	}
	if (!IsGuard())
	{
		return;
	}
	for (const auto& Bounty : LifeSubsystem->Bounties)
	{
		const auto& TheftIncident = Bounty.Incident;
		if (TheftIncident.Offender.PersonId != PersonId || LifeSubsystem->IsSettled(TheftIncident.IncidentId))
		{
			continue;
		}
		if (KnownThefts.ContainsByPredicate([&](const FTheftIncident& KnownIncident)
		                                    { return KnownIncident.IncidentId == TheftIncident.IncidentId; }))
		{
			continue;
		}
		if (!Visitor(TheftIncident))
		{
			return;
		}
	}
}

bool APropertyAIController::HasActionableCases(FName PersonId) const
{
	bool bHasUnsettledTheft = false;
	VisitActionableCases(PersonId,
	                     [&](const FTheftIncident&)
	                     {
		                     bHasUnsettledTheft = true;
		                     return false;
	                     });
	return bHasUnsettledTheft;
}

int32 APropertyAIController::CountActionableCases(FName PersonId) const
{
	int32 TheftCount = 0;
	VisitActionableCases(PersonId,
	                     [&](const FTheftIncident&)
	                     {
		                     ++TheftCount;
		                     return true;
	                     });
	return TheftCount;
}

bool APropertyAIController::TryQuestion(AActor* Player)
{
	const auto* QuestionedPawn = Cast<APawn>(Player);
	if (!Resident || !QuestionedPawn || !QuestionedPawn->IsPlayerControlled() ||
	    !Resident->Knowledge->CanSeeActor(Player) || Suspect.IsValid() ||
	    GetWorld()->GetTimeSeconds() < NextQuestionAt ||
	    FVector::DistSquared(Resident->GetActorLocation(), Player->GetActorLocation()) >
	        FMath::Square(Profile->ConfrontDistance))
	{
		return false;
	}
	const FName Person = UPropertyIdentityComponent::GetIdentity(Player).PersonId;
	const auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	const bool bSuspected = Resident->Inspection->Reports.ContainsByPredicate(
	    [&](const FPropertyMissingReport& Report)
	    {
		    return Report.PossiblePerson == Person && !Person.IsNone() && Report.Age <= MissingReportLifetimeSeconds &&
		           !LifeSubsystem->IsMissingItemResolved(Report.ItemId) && CanEnforceAt(Report.Location);
	    });
	if (!bSuspected || !StartResponse(Player, false))
	{
		return false;
	}
	bQuestioning = true;
	NextQuestionAt = GetWorld()->GetTimeSeconds() + QuestioningCooldownSeconds;
	DesiredResponse = EPropertyResponseState::Confront;
	Resident->Say(Resident->GetDialogue(EPropertyDialogueCue::StartQuestioning));
	return true;
}
