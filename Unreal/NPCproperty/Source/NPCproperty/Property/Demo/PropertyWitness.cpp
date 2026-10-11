#include "Property/Demo/PropertyWitness.h"
#include "Property/PropertyInspectionComponent.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Property/AI/PropertyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Property/PropertyParticipantComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Property/PropertyHearingComponent.h"
#include "Property/PropertyLifeSubsystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

APropertyWitness::APropertyWitness()
{
	PrimaryActorTick.bCanEverTick = false;
	Inspection = CreateDefaultSubobject<UPropertyInspectionComponent>(TEXT("Inspection"));
	Hearing = CreateDefaultSubobject<UPropertyHearingComponent>(TEXT("Hearing"));
	SpeechLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SpeechLabel"));
	SpeechLabel->SetupAttachment(RootComponent);
	SpeechLabel->SetRelativeLocation(FVector(0, 0, 175));
	SpeechLabel->SetHorizontalAlignment(EHTA_Center);
	SpeechLabel->SetWorldSize(16.f);
	SpeechLabel->SetText(FText::GetEmpty());
	SpeechLabel->SetTextRenderColor(FColor(255, 220, 150));
	Participant = CreateDefaultSubobject<UPropertyParticipantComponent>(TEXT("Participant"));
	AIControllerClass = APropertyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 240, 0);
	Identity = CreateDefaultSubobject<UPropertyIdentityComponent>(TEXT("Identity"));
	Identity->Identity.PersonId = TEXT("Merchant");
	Knowledge = CreateDefaultSubobject<UNPCKnowledgeComponent>(TEXT("Knowledge"));
	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(RootComponent);
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderBody->SetRelativeScale3D(FVector(.6f, .6f, 1.7f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded()) PlaceholderBody->SetStaticMesh(Cylinder.Object);
	StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusLabel"));
	StatusLabel->SetupAttachment(RootComponent);
	StatusLabel->SetRelativeLocation(FVector(0, 0, 130));
	StatusLabel->SetHorizontalAlignment(EHTA_Center);
	StatusLabel->SetWorldSize(22.f);
	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(RootComponent);
	FacingArrow->SetRelativeLocation(FVector(35, 0, 60));
	FacingArrow->SetHiddenInGame(false);
	FacingArrow->ArrowColor = FColor::Yellow;
	FacingArrow->ArrowSize = 2.f;
}

void APropertyWitness::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Knowledge->OnTheftWitnessed.AddDynamic(this, &APropertyWitness::HandleTheft);
}

void APropertyWitness::HandleTheft(FTheftIncident Incident)
{
	ReceiveTheftWitnessed(Incident);
}

bool APropertyWitness::PlayAttackAnimation()
{
	auto* Animation = GetMesh()->GetAnimInstance();
	return Participant->Health > 0.f && Animation && AttackAnimation
		&& Animation->PlaySlotAnimationAsDynamicMontage(AttackAnimation, TEXT("DefaultSlot"), .08f, .15f, 1.2f) != nullptr;
}

void APropertyWitness::PlayKnockdown(bool bRestore)
{
	auto* Animation = GetMesh()->GetAnimInstance();
	if (!Animation || !KnockdownAnimation) return;
	KnockdownMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(KnockdownAnimation, TEXT("DefaultSlot"), bRestore ? 0.f : .1f, .25f);
	if (!KnockdownMontage) return;
	KnockdownMontage->bEnableAutoBlendOut = false;
	Animation->Montage_Play(KnockdownMontage);
	if (bRestore) Animation->Montage_SetPosition(KnockdownMontage, FMath::Max(0.f, KnockdownMontage->GetPlayLength() - .01f));
}

void APropertyWitness::PlayDamageAnimation()
{
	if (Participant->Health <= 0.f)
	{
		PlayKnockdown(false);
		return;
	}
	if (auto* Animation = GetMesh()->GetAnimInstance(); Animation && HitReactionAnimation)
	{
		Animation->PlaySlotAnimationAsDynamicMontage(HitReactionAnimation, TEXT("DefaultSlot"), .06f, .15f);
	}
}

void APropertyWitness::RestoreAnimationPose()
{
	if (auto* Animation = GetMesh()->GetAnimInstance()) Animation->Montage_Stop(.2f);
	KnockdownMontage = nullptr;
	if (Participant->Health <= 0.f) PlayKnockdown(true);
}

void APropertyWitness::Say(FText Message)
{
	if (Participant->Health <= 0.f || Message.IsEmpty()) return;
	LastSpeech = Message;
	SpeechUntil = GetWorld()->GetTimeSeconds() + 4.f;
	SpeechLabel->SetText(Message);
	GetWorldTimerManager().SetTimer(SpeechTimer, this, &APropertyWitness::ClearSpeech, 4.f, false);
	ReceiveSpeech(Message);
}

void APropertyWitness::ClearSpeech()
{
	SpeechLabel->SetText(FText::GetEmpty());
}

bool APropertyWitness::RaiseAlarm(FVector EvidenceLocation)
{
	auto* ResponseController = Cast<APropertyAIController>(GetController());
	if (!HasAuthority() || !ResponseController || !Knowledge->bAlert || Participant->Health <= 0.f || !ResponseController->CanEnforceAt(EvidenceLocation) || GetWorld()->GetTimeSeconds() < NextAlarm) return false;
	NextAlarm = GetWorld()->GetTimeSeconds() + 8.f;
	SayCue(EPropertyDialogueCue::RaiseAlarm);
	GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->EmitNoise(this, GetActorLocation(), 2200.f, EPropertyNoiseKind::Alarm, AlarmSound);
	return true;
}

FText APropertyWitness::GetDialogue_Implementation(EPropertyDialogueCue Cue) const
{
    if (const auto* Override = DialogueOverrides.Find(Cue)) return *Override;
    if (DialogueProfile)
        if (const auto* Line = DialogueProfile->Lines.Find(Cue)) return *Line;
    return FText::GetEmpty();
}

void APropertyWitness::SayCue(EPropertyDialogueCue Cue)
{
    Say(GetDialogue(Cue));
}

FText APropertyWitness::GetResponseDescription(EPropertyResponseState State) const
{
    const auto* AI = Cast<APropertyAIController>(GetController());
    return GetDialogue(SelectResponseCue(State, AI ? AI->SearchRole : EPropertySearchRole::None,
        AI && AI->bQuestioning, AI && AI->GetCompensation() > 0, AI && AI->GetSuspect()));
}

EPropertyDialogueCue APropertyWitness::SelectResponseCue_Implementation(EPropertyResponseState State, EPropertySearchRole SearchAssignment, bool bQuestioning, bool bHasCrime, bool bHasSuspect) const
{
    switch (State)
    {
    case EPropertyResponseState::Warn: return EPropertyDialogueCue::WarningStatus;
    case EPropertyResponseState::Chase: return EPropertyDialogueCue::ChaseStatus;
    case EPropertyResponseState::Search: return EPropertyDialogueCue::InvestigatingStatus;
    case EPropertyResponseState::Confront: return EPropertyDialogueCue::TrespassConfrontStatus;
    case EPropertyResponseState::Return: return EPropertyDialogueCue::ReturnStatus;
    default: return EPropertyDialogueCue::RoutineStatus;
    }
}

float APropertyWitness::SelectResponseSpeed_Implementation(EPropertyResponseState State, float WalkSpeed, float ChaseSpeed) const
{
    return State == EPropertyResponseState::Chase ? ChaseSpeed : WalkSpeed;
}

EPropertyDialogueCue APropertyWitness::SelectNoiseCue_Implementation(EPropertyNoiseKind Kind) const
{
    return Kind == EPropertyNoiseKind::Alarm ? EPropertyDialogueCue::HearAlarm : EPropertyDialogueCue::HearNoise;
}

