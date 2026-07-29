#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleReplayTypes.h"
#include "VehicleGhostActor.generated.h"

class USkeletalMeshComponent;
class AWheeledVehiclePawn;

UCLASS()
class CVEHICLEAREA_API AVehicleGhostActor : public AActor
{
	GENERATED_BODY()

public:

	AVehicleGhostActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost")
	bool bLoop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "0.05", ClampMax = "4.0"))
	float PlaybackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost")
	bool bDrawDebugMarker = true;

	UFUNCTION(BlueprintCallable, Category = "Ghost")
	void CopyAppearanceFrom(const AWheeledVehiclePawn* SourceVehicle);

	void SetRecording(const FVehicleLapRecording& InRecording);

	UFUNCTION(BlueprintCallable, Category = "Ghost")
	void StartPlayback();

	UFUNCTION(BlueprintCallable, Category = "Ghost")
	void StopPlayback();

	UFUNCTION(BlueprintPure, Category = "Ghost")
	bool IsPlaying() const { return bIsPlaying; }

	UFUNCTION(BlueprintPure, Category = "Ghost")
	float GetPlaybackTime() const { return PlaybackTime; }

	UFUNCTION(BlueprintPure, Category = "Ghost")
	float GetRecordedLapTime() const { return Recording.LapTime; }

	UFUNCTION(BlueprintPure, Category = "Ghost")
	USkeletalMeshComponent* GetGhostMesh() const { return GhostMesh; }

	virtual void Tick(float DeltaTime) override;

private:

	void UpdatePose(float Time);

	UPROPERTY(VisibleAnywhere, Category = "Ghost", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> GhostMesh;

	UPROPERTY(Transient)
	FVehicleLapRecording Recording;

	bool bIsPlaying = false;

	float PlaybackTime = 0.0f;

	int32 Cursor = 0;
};
