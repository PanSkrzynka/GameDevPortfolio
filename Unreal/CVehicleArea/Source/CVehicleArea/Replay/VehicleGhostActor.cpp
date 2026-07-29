#include "VehicleGhostActor.h"
#include "CVehicleArea.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "WheeledVehiclePawn.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#endif

AVehicleGhostActor::AVehicleGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	GhostMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GhostMesh"));
	SetRootComponent(GhostMesh);

	GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMesh->SetSimulatePhysics(false);
	GhostMesh->SetGenerateOverlapEvents(false);
	GhostMesh->SetCastShadow(false);
	GhostMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GhostMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GhostMesh->PrimaryComponentTick.bCanEverTick = true;
	GhostMesh->bPerBoneMotionBlur = false;

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void AVehicleGhostActor::CopyAppearanceFrom(const AWheeledVehiclePawn* SourceVehicle)
{
	if (!SourceVehicle || !GhostMesh)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Ghost '%s': no source vehicle."), *GetName());
		return;
	}

	const USkeletalMeshComponent* SourceMesh = SourceVehicle->GetMesh();
	if (!SourceMesh)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Ghost '%s': '%s' has no mesh component."),
			*GetName(), *SourceVehicle->GetName());
		return;
	}

	USkeletalMesh* SourceAsset = SourceMesh->GetSkeletalMeshAsset();
	if (!SourceAsset)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Ghost '%s': '%s' has no mesh asset."),
			*GetName(), *SourceVehicle->GetName());
		return;
	}

	GhostMesh->SetSkeletalMeshAsset(SourceAsset);
	GhostMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GhostMesh->SetComponentSpaceTransformsDoubleBuffering(false);
	GhostMesh->RefreshBoneTransforms();

	const int32 MaterialCount = SourceMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		GhostMesh->SetMaterial(MaterialIndex, SourceMesh->GetMaterial(MaterialIndex));
	}

	GhostMesh->SetVisibility(true, true);

	UE_LOG(LogCVehicleArea, Log, TEXT("Ghost '%s': mesh '%s', %d materials."),
		*GetName(), *SourceAsset->GetName(), MaterialCount);
}

void AVehicleGhostActor::SetRecording(const FVehicleLapRecording& InRecording)
{
	Recording = InRecording;
	PlaybackTime = 0.0f;
	Cursor = 0;

	if (Recording.IsValid())
	{
		UpdatePose(0.0f);
	}
}

void AVehicleGhostActor::StartPlayback()
{
	if (!Recording.IsValid())
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Ghost '%s': no recording."), *GetName());
		return;
	}

	bIsPlaying = true;
	PlaybackTime = 0.0f;
	Cursor = 0;

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	if (GhostMesh)
	{
		GhostMesh->SetVisibility(true, true);
		GhostMesh->SetHiddenInGame(false, true);
	}

	UpdatePose(0.0f);
}

void AVehicleGhostActor::StopPlayback()
{
	bIsPlaying = false;

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void AVehicleGhostActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsPlaying || !Recording.IsValid())
	{
		return;
	}

	PlaybackTime += DeltaTime * PlaybackSpeed;

	const float Duration = Recording.GetDuration();
	if (PlaybackTime > Duration)
	{
		if (!bLoop)
		{
			StopPlayback();
			return;
		}

		PlaybackTime = Duration > KINDA_SMALL_NUMBER ? FMath::Fmod(PlaybackTime, Duration) : 0.0f;
		Cursor = 0;
	}

	UpdatePose(PlaybackTime);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugMarker)
	{
		const FVector Location = GetActorLocation();
		DrawDebugBox(GetWorld(), Location, FVector(220.0f, 90.0f, 70.0f), GetActorQuat(), FColor::Cyan, false, -1.0f, 0, 4.0f);
		DrawDebugLine(GetWorld(), Location, Location + FVector(0.0f, 0.0f, 400.0f), FColor::Cyan, false, -1.0f, 0, 4.0f);
	}
#endif
}

void AVehicleGhostActor::UpdatePose(float Time)
{
	const TArray<FVehicleReplaySample>& Samples = Recording.Samples;
	if (Samples.Num() < 2)
	{
		return;
	}

	const float StartTime = Samples[0].TimeSeconds;
	const float AbsoluteTime = StartTime + Time;

	while (Cursor + 1 < Samples.Num() - 1 && Samples[Cursor + 1].TimeSeconds < AbsoluteTime)
	{
		++Cursor;
	}

	const FVehicleReplaySample& From = Samples[Cursor];
	const FVehicleReplaySample& To = Samples[Cursor + 1];

	const float SegmentDuration = To.TimeSeconds - From.TimeSeconds;
	const float Alpha = SegmentDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp((AbsoluteTime - From.TimeSeconds) / SegmentDuration, 0.0f, 1.0f)
		: 0.0f;

	const FTransform& FromTransform = From.Snapshot.Transform;
	const FTransform& ToTransform = To.Snapshot.Transform;

	FTransform Pose;
	Pose.SetLocation(FMath::Lerp(FromTransform.GetLocation(), ToTransform.GetLocation(), Alpha));
	Pose.SetRotation(FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), Alpha).GetNormalized());
	Pose.SetScale3D(FromTransform.GetScale3D());

	SetActorTransform(Pose, false, nullptr, ETeleportType::TeleportPhysics);
}
