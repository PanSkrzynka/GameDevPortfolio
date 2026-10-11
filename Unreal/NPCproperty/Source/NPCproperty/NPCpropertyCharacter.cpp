#include "NPCpropertyCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "NPCproperty.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/PropertyInteractionComponent.h"
#include "InputCoreTypes.h"
#include "Property/PropertyParticipantComponent.h"
#include "Property/PropertyLifeSubsystem.h"

ANPCpropertyCharacter::ANPCpropertyCharacter()
{
	PropertyParticipant = CreateDefaultSubobject<UPropertyParticipantComponent>(TEXT("PropertyParticipant"));
	PropertyIdentity = CreateDefaultSubobject<UPropertyIdentityComponent>(TEXT("PropertyIdentity"));
	PropertyIdentity->Identity.PersonId = TEXT("Player");
	PropertyInteraction = CreateDefaultSubobject<UPropertyInteractionComponent>(TEXT("PropertyInteraction"));

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SocketOffset = FVector(0.f, 85.f, 45.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

}

void ANPCpropertyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &ANPCpropertyCharacter::NextShowcase);
	PlayerInputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &ANPCpropertyCharacter::PreviousShowcase);
	PlayerInputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ANPCpropertyCharacter::TogglePropertyDebug);
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &ANPCpropertyCharacter::ToggleCrouch);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &ANPCpropertyCharacter::StartCrouch);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Released, this, &ANPCpropertyCharacter::EndCrouch);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ANPCpropertyCharacter::ReturnGoods);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ANPCpropertyCharacter::PayCompensation);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ANPCpropertyCharacter::Resist);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ANPCpropertyCharacter::Submit);
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ANPCpropertyCharacter::Attack);
	PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &ANPCpropertyCharacter::Distract);
	PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ANPCpropertyCharacter::AdvanceTime);
	PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ANPCpropertyCharacter::SaveProgress);
	PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ANPCpropertyCharacter::LoadProgress);
	if (!InteractAction) PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ANPCpropertyCharacter::DoInteract);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		if (InteractAction) EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ANPCpropertyCharacter::DoInteract);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANPCpropertyCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ANPCpropertyCharacter::Look);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANPCpropertyCharacter::Look);
	}
	else
	{
		UE_LOG(LogNPCproperty, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANPCpropertyCharacter::ReturnGoods() { PropertyParticipant->Choose(EConfrontationChoice::ReturnGoods); }
void ANPCpropertyCharacter::PayCompensation() { PropertyParticipant->Choose(EConfrontationChoice::PayCompensation); }
void ANPCpropertyCharacter::Resist() { PropertyParticipant->Choose(EConfrontationChoice::Resist); }
void ANPCpropertyCharacter::Submit() { PropertyParticipant->Choose(EConfrontationChoice::Submit); }
void ANPCpropertyCharacter::Attack() { PropertyParticipant->Attack(); }
void ANPCpropertyCharacter::Distract() { PropertyParticipant->Distract(); }
void ANPCpropertyCharacter::AdvanceTime() { PropertyParticipant->AdvanceTime(); }
void ANPCpropertyCharacter::SaveProgress() { PropertyParticipant->Save(); }
void ANPCpropertyCharacter::LoadProgress() { PropertyParticipant->Load(); }

void ANPCpropertyCharacter::DoInteract()
{
	PropertyInteraction->Interact();
}

void ANPCpropertyCharacter::Move(const FInputActionValue& Value)
{

	FVector2D MovementVector = Value.Get<FVector2D>();

	DoMove(MovementVector.X, MovementVector.Y);
}

void ANPCpropertyCharacter::Look(const FInputActionValue& Value)
{

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ANPCpropertyCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{

		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ANPCpropertyCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{

		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ANPCpropertyCharacter::DoJumpStart()
{

	Jump();
}

void ANPCpropertyCharacter::DoJumpEnd()
{

	StopJumping();
}

void ANPCpropertyCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(60.f);
	GetCharacterMovement()->MaxWalkSpeedCrouched = 150.f;
	PreviousStepLocation = GetActorLocation();
}

void ANPCpropertyCharacter::ToggleCrouch()
{
	if (bIsCrouched || GetCharacterMovement()->bWantsToCrouch) UnCrouch();
	else Crouch();
}

void ANPCpropertyCharacter::StartCrouch() { Crouch(); }
void ANPCpropertyCharacter::EndCrouch() { UnCrouch(); }

float ANPCpropertyCharacter::GetFootstepRadius() const
{
	return bIsCrouched ? CrouchedStepRadius : StandingStepRadius;
}

void ANPCpropertyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector Location = GetActorLocation();
	const float Distance = FVector::Dist2D(Location, PreviousStepLocation);
	PreviousStepLocation = Location;
	LastNoiseRadius = FMath::Max(0.f, LastNoiseRadius - 350.f * DeltaSeconds);
	if (!HasAuthority() || PropertyParticipant->Health <= 0.f || !GetCharacterMovement()->IsMovingOnGround() || GetVelocity().Size2D() < 10.f || Distance > 150.f)
	{
		StepDistance = 0.f;
		return;
	}
	StepDistance += Distance;
	if (StepDistance < (bIsCrouched ? 90.f : 140.f)) return;
	StepDistance = 0.f;
	LastNoiseRadius = GetFootstepRadius();
	GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->EmitNoise(this, Location - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 10.f), LastNoiseRadius, EPropertyNoiseKind::Footstep, FootstepSound);
}

void ANPCpropertyCharacter::Landed(const FHitResult& Hit)
{
	const float Strength = FMath::Clamp(FMath::Abs(GetVelocity().Z) / 800.f, .3f, 1.2f);
	Super::Landed(Hit);
	if (!HasAuthority()) return;
	LastNoiseRadius = 900.f * Strength;
	GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->EmitNoise(this, Hit.ImpactPoint + FVector(0, 0, 10.f), LastNoiseRadius, EPropertyNoiseKind::Landing, ImpactSound);
}

void ANPCpropertyCharacter::Showcase(int32 Station)
{
    const FName Tag(*FString::Printf(TEXT("Showcase%d"), Station));
    for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
    {
        if (!It->ActorHasTag(Tag)) continue;
        if (!TeleportTo(It->GetActorLocation(), It->GetActorRotation())) return;
        GetCharacterMovement()->StopMovementImmediately();
        if (GetController()) GetController()->SetControlRotation(It->GetActorRotation());
        PreviousStepLocation = GetActorLocation();
        StepDistance = 0.f;
        PropertyParticipant->Notify(FString::Printf(TEXT("Showcase %d / Page Up: previous / Page Down: next"), Station));
        return;
    }
}

void ANPCpropertyCharacter::StepShowcase(int32 Direction)
{
    int32 Nearest = 0;
    int32 Count = 0;
    double Distance = TNumericLimits<double>::Max();
    for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
    {
        if (!It->ActorHasTag(TEXT("PropertyShowcase"))) continue;
        ++Count;
        const double Candidate = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (Candidate >= Distance) continue;
        Distance = Candidate;
        for (const FName Tag : It->Tags)
        {
            const FString Value = Tag.ToString();
            if (Value.StartsWith(TEXT("Showcase"))) Nearest = FCString::Atoi(*Value.Mid(8));
        }
    }
    if (Count > 0) Showcase((Nearest - 1 + Direction + Count) % Count + 1);
}

void ANPCpropertyCharacter::NextShowcase()
{
    StepShowcase(1);
}

void ANPCpropertyCharacter::PreviousShowcase()
{
    StepShowcase(-1);
}
