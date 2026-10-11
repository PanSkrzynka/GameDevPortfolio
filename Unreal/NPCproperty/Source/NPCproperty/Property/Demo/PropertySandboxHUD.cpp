#include "Property/Demo/PropertySandboxHUD.h"
#include "NPCpropertyCharacter.h"
#include "DrawDebugHelpers.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ArrowComponent.h"
#include "Property/PropertyInspectionComponent.h"
#include "Property/PropertyHearingComponent.h"
#include "Property/PropertyWorldSubsystem.h"
#include "Property/PropertyArea.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/OwnershipComponent.h"
#include "Property/OwnedPickup.h"
#include "Property/CrimeWorldSubsystem.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/AI/PropertyAIController.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Property/PropertyParticipantComponent.h"
#include "Property/PropertyLifeSubsystem.h"

namespace
{
    void DrawPropertyDebug(UWorld* World, APawn* Player)
    {
        for (TActorIterator<APropertyArea> It(World); It; ++It)
        {
            DrawDebugString(World, It->GetActorLocation() + FVector(0, 0, 250), It->DisplayName.ToString() + TEXT(" | ") + StaticEnum<EPropertyAlertLevel>()->GetNameStringByValue(static_cast<int64>(It->GetAlertLevel())), nullptr, FColor::Orange, 0.f);
            DrawDebugSphere(World, It->GetExitLocation(), 55, 12, FColor::Orange, false, 0.f);
            DrawDebugBox(World, It->Bounds->GetComponentLocation(), It->Bounds->GetScaledBoxExtent(), It->Bounds->GetComponentQuat(), FColor(160, 80, 255), false, 0.f, 0, 2.f);
        }
        for (TActorIterator<APropertyWitness> It(World); It; ++It)
        {
            if (FVector::DistSquared(Player->GetActorLocation(), It->GetActorLocation()) > FMath::Square(2200.f)) continue;
            const auto* AI = Cast<APropertyAIController>(It->GetController());
            if (!AI) continue;
            FVector Eye;
            FRotator Facing;
            It->GetActorEyesViewPoint(Eye, Facing);
            DrawDebugCone(World, Eye, Facing.Vector(), It->Knowledge->SightRange, FMath::DegreesToRadians(It->Knowledge->HalfVisionAngle), .12f, 12, FColor::Green, false, 0.f);
            DrawDebugCircle(World, It->GetActorLocation() + FVector(0, 0, -80), It->Hearing->HearingRange, 48, FColor::Cyan, false, 0.f, 0, 1.f, FVector::ForwardVector, FVector::RightVector, false);
            DrawDebugString(World, It->GetActorLocation() + FVector(0, 0, 220), FString::Printf(TEXT("%s | %s | suspicion %.0f%% | objects %d"), *It->Identity->Identity.PersonId.ToString(), *StaticEnum<EPropertyResponseState>()->GetNameStringByValue(static_cast<int64>(AI->ActiveResponse)), It->Hearing->Suspicion * 100.f, It->Inspection->Memories.Num()), nullptr, FColor::White, 0.f, true);
            if (It->Routine && It->Routine->Entries.IsValidIndex(AI->RoutineEntry))
            {
                const auto& Points = It->Routine->Entries[AI->RoutineEntry].Offsets;
                for (int32 Index = 0; Index < Points.Num(); ++Index)
                {
                    const FVector Point = AI->HomeLocation + Points[Index];
                    DrawDebugSphere(World, Point, 30, 8, Index == AI->Waypoint ? FColor::Orange : FColor::Blue, false, 0.f);
                    DrawDebugLine(World, Point, AI->HomeLocation + Points[(Index + 1) % Points.Num()], FColor::Blue, false, 0.f, 0, 3.f);
                    DrawDebugString(World, Point + FVector(0, 0, 45), FString::FromInt(Index + 1), nullptr, FColor::Orange, 0.f);
                }
            }
            DrawDebugString(World, It->GetActorLocation() + FVector(0, 0, 255), FString::Printf(TEXT("Role: %s | reports %d | %s"), *StaticEnum<EPropertySearchRole>()->GetNameStringByValue(static_cast<int64>(AI->SearchRole)), It->Inspection->Reports.Num(), AI->bQuestioning ? TEXT("QUESTIONING") : TEXT("")), nullptr, FColor::Yellow, 0.f);
            if (AI->ActiveResponse == EPropertyResponseState::Search || AI->GetSuspect())
            {
                DrawDebugSphere(World, AI->LastKnownLocation, 45, 12, FColor::Red, false, 0.f);
                DrawDebugDirectionalArrow(World, It->GetActorLocation(), AI->ActiveResponse == EPropertyResponseState::Search ? AI->SearchDestination : AI->LastKnownLocation, 50, FColor::Yellow, false, 0.f, 0, 3.f);
            }
            for (const auto& Memory : It->Inspection->Memories)
            {
                const FColor Color = Memory.bMissing ? FColor::Red : FColor::Emerald;
                DrawDebugBox(World, Memory.Location, FVector(22), Color, false, 0.f, 0, 2.f);
                DrawDebugString(World, Memory.Location + FVector(0, 0, 50), (Memory.bMissing ? TEXT("MISSING: ") : TEXT("REMEMBERED: ")) + Memory.Name.ToString(), nullptr, Color, 0.f);
            }
        }
    }
}

void APropertySandboxHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !PlayerOwner->GetPawn()) return;
    APawn* Player = PlayerOwner->GetPawn();
    auto* Interaction = Player->FindComponentByClass<UPropertyInteractionComponent>();
    if (!Interaction) return;
    if (BoundInteraction != Interaction)
    {
        if (BoundInteraction) BoundInteraction->OnInteractionFinished.RemoveDynamic(this, &APropertySandboxHUD::HandleInteraction);
        BoundInteraction = Interaction;
        Interaction->OnInteractionFinished.AddDynamic(this, &APropertySandboxHUD::HandleInteraction);
    }
    auto* Participant = Player->FindComponentByClass<UPropertyParticipantComponent>();
    if (BoundParticipant != Participant)
    {
        if (BoundParticipant) BoundParticipant->OnFeedback.RemoveDynamic(this, &APropertySandboxHUD::HandleFeedback);
        BoundParticipant = Participant;
        Feedback.Empty();
        FeedbackUntil = 0.f;
        if (Participant) Participant->OnFeedback.AddDynamic(this, &APropertySandboxHUD::HandleFeedback);
    }

    const float Scale = FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 720.f);
    const float Width = Canvas->ClipX / Scale;
    const float Height = Canvas->ClipY / Scale;
    const float Now = GetWorld()->GetTimeSeconds();
    const FLinearColor Paper(.88f, .84f, .74f);
    const FLinearColor Muted(.57f, .55f, .49f);
    const FLinearColor Warning(.78f, .42f, .28f);
    const FLinearColor Ink(.025f, .023f, .019f, .82f);
    const auto Text = [&](const FString& Value, float X, float Y, FLinearColor Color, float Size = 1.f, bool bCentered = false)
    {
        float TextWidth = 0.f;
        float TextHeight = 0.f;
        GetTextSize(Value, TextWidth, TextHeight, nullptr, Size * Scale);
        DrawText(Value, FLinearColor(0.f, 0.f, 0.f, Color.A * .8f), X * Scale - (bCentered ? TextWidth * .5f : 0.f) + Scale, (Y + 1.f) * Scale, nullptr, Size * Scale);
        DrawText(Value, Color, X * Scale - (bCentered ? TextWidth * .5f : 0.f), Y * Scale, nullptr, Size * Scale);
    };
    const auto Rect = [&](float X, float Y, float W, float H, FLinearColor Color)
    {
        DrawRect(Color, X * Scale, Y * Scale, W * Scale, H * Scale);
    };
    const auto Wrap = [&](const FString& Value, float MaxWidth)
    {
        TArray<FString> Words;
        TArray<FString> Lines;
        Value.ParseIntoArrayWS(Words);
        FString Line;
        for (const FString& Word : Words)
        {
            const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
            float W = 0.f;
            float H = 0.f;
            GetTextSize(Candidate, W, H, nullptr, Scale);
            if (!Line.IsEmpty() && W > MaxWidth * Scale)
            {
                Lines.Add(Line);
                Line = Word;
            }
            else Line = Candidate;
        }
        if (!Line.IsEmpty()) Lines.Add(Line);
        return Lines;
    };

    const auto* Character = Cast<ANPCpropertyCharacter>(Player);
    const bool bDebug = Character && Character->bPropertyDebug;
    const FPropertyIdentity Visitor = UPropertyIdentityComponent::GetIdentity(Player);
    const auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Player->GetActorLocation());
    Text(Area ? Area->DisplayName.ToString() : TEXT("Courtyard"), Width * .5f, 30.f, Paper, 1.35f, true);
    Rect(Width * .5f - 28.f, 59.f, 56.f, 1.f, Muted);
    if (Area && !Area->CanEnter(Visitor)) Text(TEXT("Trespassing"), Width * .5f, 70.f, Warning, 1.f, true);
	const int32 Bounty = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->GetBountyAmount(Visitor.PersonId);
	if (Bounty > 0) Text(FString::Printf(TEXT("Wanted  /  %d gold"), Bounty), Width * .5f, 94.f, Warning, 1.f, true);

    if (Participant)
    {
        Text(TEXT("Health"), 32.f, Height - 77.f, Muted);
        Rect(32.f, Height - 51.f, 164.f, 6.f, Ink);
        Rect(33.f, Height - 50.f, 162.f * FMath::Clamp(Participant->Health / 100.f, 0.f, 1.f), 4.f, FLinearColor(.55f, .22f, .16f));
        Text(FString::Printf(TEXT("%d gold   /   %d of %d slots"), Participant->Gold, Interaction->GetItemCount(), Interaction->Capacity), 32.f, Height - 33.f, Paper);
    }
    if (Character && Character->bIsCrouched) Text(TEXT("Crouched"), 32.f, Height - 105.f, Paper);
    Text(TEXT("F6  Controls & debug"), Width - 180.f, Height - 33.f, Muted, .85f);

    auto* Confrontation = Participant ? Participant->GetConfrontation() : nullptr;
    const float DialogueWidth = 530.f;
    const float DialogueX = (Width - DialogueWidth) * .5f;
    const float DialogueY = Height - 236.f;
    if (Confrontation)
    {
        Rect(DialogueX, DialogueY, DialogueWidth, 172.f, Ink);
        Rect(DialogueX, DialogueY, 2.f, 172.f, Muted);
        const auto* Speaker = Cast<APropertyWitness>(Confrontation->GetPawn());
        const auto Dialogue = [&](EPropertyDialogueCue Cue)
        {
            return Speaker ? FText::Format(Speaker->GetDialogue(Cue), FFormatNamedArguments{{TEXT("Amount"), FText::AsNumber(Confrontation->GetCompensation())}}).ToString() : FString();
        };
        const FString Name = Speaker ? FName::NameToDisplayString(Speaker->Identity->Identity.PersonId.ToString(), false) : TEXT("Guard");
        Text(Name, DialogueX + 20.f, DialogueY + 14.f, Muted);
        if (Confrontation->IsEscortingTrespasser())
        {
            Text(Dialogue(EPropertyDialogueCue::TrespassConfrontStatus), DialogueX + 20.f, DialogueY + 40.f, Paper, 1.15f);
        }
        else if (Confrontation->bQuestioning)
        {
            Text(Dialogue(EPropertyDialogueCue::QuestionHeading), DialogueX + 20.f, DialogueY + 40.f, Paper, 1.15f);
            Text(TEXT("1   ") + Dialogue(EPropertyDialogueCue::ExplainChoice), DialogueX + 20.f, DialogueY + 78.f, Paper);
            Text(TEXT("2   ") + Dialogue(EPropertyDialogueCue::InspectChoice), DialogueX + 20.f, DialogueY + 106.f, Paper);
            Text(TEXT("3   ") + Dialogue(EPropertyDialogueCue::DeclineChoice), DialogueX + 20.f, DialogueY + 134.f, Paper);
        }
        else
        {
            Text(Confrontation->bResisting ? Dialogue(EPropertyDialogueCue::ResistHeading) : Dialogue(EPropertyDialogueCue::TheftHeading), DialogueX + 20.f, DialogueY + 40.f, Paper, 1.15f);
            Text(TEXT("1   ") + Dialogue(EPropertyDialogueCue::ReturnChoice), DialogueX + 20.f, DialogueY + 78.f, Paper);
            Text(TEXT("2   ") + Dialogue(EPropertyDialogueCue::PayChoice), DialogueX + 280.f, DialogueY + 78.f, Paper);
            Text(TEXT("3   ") + Dialogue(EPropertyDialogueCue::ResistChoice), DialogueX + 20.f, DialogueY + 106.f, Paper);
            Text(TEXT("4   ") + Dialogue(EPropertyDialogueCue::SurrenderChoice), DialogueX + 280.f, DialogueY + 106.f, Paper);
            if (Confrontation->bResisting) Text(TEXT("F   ") + Dialogue(EPropertyDialogueCue::StrikeAction), DialogueX + 20.f, DialogueY + 140.f, Warning);
        }
    }

    APropertyWitness* LatestSpeaker = nullptr;
    for (TActorIterator<APropertyWitness> It(GetWorld()); It; ++It)
    {
        It->StatusLabel->SetHiddenInGame(!bDebug);
        It->FacingArrow->SetHiddenInGame(!bDebug);
        It->SpeechLabel->SetHiddenInGame(true);
        if (Now >= It->SpeechUntil || It->LastSpeech.IsEmpty() || FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(1000.f)) continue;
        if (!LatestSpeaker || It->SpeechUntil > LatestSpeaker->SpeechUntil) LatestSpeaker = *It;
    }
    if (LatestSpeaker)
    {
        const auto Lines = Wrap(LatestSpeaker->LastSpeech.ToString(), 610.f);
        const float Bottom = Confrontation ? DialogueY - 18.f : Height - 96.f;
        const float Top = Bottom - Lines.Num() * 23.f - 26.f;
        Rect(Width * .5f - 325.f, Top - 8.f, 650.f, Bottom - Top + 16.f, Ink);
        Text(FName::NameToDisplayString(LatestSpeaker->Identity->Identity.PersonId.ToString(), false), Width * .5f, Top, Muted, .9f, true);
        for (int32 Index = 0; Index < Lines.Num(); ++Index) Text(Lines[Index], Width * .5f, Top + 26.f + Index * 23.f, Paper, 1.f, true);
    }
    if (Now < FeedbackUntil)
    {
        FLinearColor Color = Paper;
        Color.A = FMath::Clamp(FeedbackUntil - Now, 0.f, 1.f);
        const auto Lines = Wrap(Feedback, 560.f);
        for (int32 Index = 0; Index < Lines.Num(); ++Index) Text(Lines[Index], Width * .5f, 115.f + Index * 22.f, Color, 1.f, true);
    }

    const FVector2D Aim = Interaction->GetAimScreenPosition(FVector2D(Canvas->ClipX, Canvas->ClipY)) / Scale;
    Rect(Aim.X - 1.f, Aim.Y - 1.f, 2.f, 2.f, Paper);
    if (auto* Pickup = Interaction->FindFocusedPickup())
    {
        const bool bInReach = FVector::DistSquared(Player->GetActorLocation(), Pickup->GetActorLocation()) <= FMath::Square(Interaction->Reach);
        const bool bLegal = Pickup->Ownership->CanTake(Visitor);
        Text(Pickup->DisplayName.ToString(), Aim.X, Aim.Y + 22.f, Paper, 1.15f, true);
        Text(bInReach ? (bLegal ? TEXT("E   Take") : TEXT("E   Steal")) : TEXT("Move closer"), Aim.X, Aim.Y + 49.f, bLegal ? Muted : Warning, 1.f, true);
    }

    if (bDebug)
    {
        DrawPropertyDebug(GetWorld(), Player);
        Rect(20.f, 20.f, 390.f, 415.f, Ink);
        Text(TEXT("DEVELOPER VIEW   /   F6 to close"), 34.f, 32.f, Paper);
        const TArray<FString> Controls = {
            TEXT("WASD Move   /   Mouse Look   /   Space Jump"),
            TEXT("E Take   /   C or Ctrl Crouch   /   H Throw stone"),
            TEXT("F Strike   /   1-4 Dialogue"),
            TEXT("F5 Save   /   F9 Load   /   T Advance six hours"),
            TEXT("Sight: green   /   Hearing: cyan   /   Rooms: purple"),
            TEXT("Patrol: blue   /   Search: yellow   /   Missing: red")
        };
        float Y = 62.f;
        for (const auto& Line : Controls) { Text(Line, 34.f, Y, Paper, .9f); Y += 23.f; }
        const float Hour = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->Hour;
        Text(FString::Printf(TEXT("Time %02d:%02d   /   Crimes %d"), FMath::FloorToInt(Hour), FMath::FloorToInt(FMath::Frac(Hour) * 60.f), GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->GetIncidents().Num()), 34.f, Y + 10.f, Muted);
        Y += 36.f;
        if (Character) Text(FString::Printf(TEXT("Footsteps %.1fm   /   Last noise %.1fm"), Character->GetFootstepRadius() / 100.f, Character->LastNoiseRadius / 100.f), 34.f, Y, Muted);
        Y += 26.f;
        if (Area) Text(FString::Printf(TEXT("Room: %s (%.0f)"), *StaticEnum<EPropertyAlertLevel>()->GetNameStringByValue(static_cast<int64>(Area->GetAlertLevel())), Area->AlertScore), 34.f, Y, Muted);
        Y += 26.f;
        for (TActorIterator<APropertyWitness> It(GetWorld()); It && Y < 410.f; ++It)
        {
            if (FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(1800.f)) continue;
            const auto* ResponseController = Cast<APropertyAIController>(It->GetController());
            if (!ResponseController) continue;
            Text(It->Identity->Identity.PersonId.ToString() + TEXT(": ") + StaticEnum<EPropertyResponseState>()->GetNameStringByValue(static_cast<int64>(ResponseController->ActiveResponse)), 34.f, Y, Paper, .9f);
            Y += 22.f;
        }
    }
}

void APropertySandboxHUD::HandleFeedback(FText Message)
{
    Feedback = Message.ToString();
    FeedbackUntil = GetWorld()->GetTimeSeconds() + 4.f;
}

void APropertySandboxHUD::HandleInteraction(EPropertyInteractionResult Result, FOwnedItemRecord Item)
{
	switch (Result)
	{
	case EPropertyInteractionResult::Taken: Feedback = TEXT("Picked up ") + Item.DisplayName.ToString(); break;
	case EPropertyInteractionResult::Stolen: Feedback = TEXT("Stole ") + Item.DisplayName.ToString(); break;
	case EPropertyInteractionResult::OutOfReach: Feedback = TEXT("Move closer to the item."); break;
	case EPropertyInteractionResult::Obstructed: Feedback = TEXT("Something blocks your reach."); break;
	case EPropertyInteractionResult::InventoryFull: Feedback = TEXT("Inventory full."); break;
	case EPropertyInteractionResult::NoTarget: Feedback = TEXT("Aim the crosshair at an item."); break;
	default: Feedback = TEXT("This item is no longer available."); break;
	}
	FeedbackUntil = GetWorld()->GetTimeSeconds() + 4.f;
}
