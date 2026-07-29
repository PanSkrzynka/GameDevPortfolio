#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RacingLine.h"
#include "RaceDirectorSubsystem.generated.h"

class APawn;
class AVehicleAIController;

UENUM(BlueprintType)
enum class ERaceState : uint8
{

	Idle,

	Countdown,

	Racing,

	Finished
};

USTRUCT(BlueprintType)
struct FRaceCompetitor
{
	GENERATED_BODY()

	TWeakObjectPtr<APawn> Vehicle;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	bool bIsPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	int32 LapsCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Race", meta = (Units = "s"))
	float LastLapTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Race", meta = (Units = "s"))
	float BestLapTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Race", meta = (Units = "s"))
	float TotalTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	int32 Position = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	float DistanceAlongLine = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	bool bFinished = false;

	float LapStartTime = 0.0f;

	int32 CurrentSector = 0;

	TArray<bool> VisitedSectors;

	int32 LastPointIndex = INDEX_NONE;

	float GetRaceProgress(float LineLength) const
	{
		const float LapFraction = LineLength > KINDA_SMALL_NUMBER ? DistanceAlongLine / LineLength : 0.0f;
		return LapsCompleted + LapFraction;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaceStateChanged, ERaceState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCompetitorLapCompleted, const FString&, DisplayName, float, LapTime, int32, LapsCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCompetitorFinished, const FString&, DisplayName, int32, FinishPosition);

UCLASS()
class CVEHICLEAREA_API URaceDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race", meta = (ClampMin = "3", ClampMax = "64"))
	int32 SectorCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float MinSectorFraction = 0.85f;

	UPROPERTY(BlueprintAssignable, Category = "Race")
	FOnRaceStateChanged OnRaceStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Race")
	FOnCompetitorLapCompleted OnCompetitorLapCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Race")
	FOnCompetitorFinished OnCompetitorFinished;

	void SetReferenceLine(const FRacingLine& InLine);

	UFUNCTION(BlueprintPure, Category = "Race")
	bool HasReferenceLine() const { return ReferenceLine.IsValid(); }

	const FRacingLine& GetReferenceLine() const { return ReferenceLine; }

	UFUNCTION(BlueprintCallable, Category = "Race")
	bool RegisterCompetitor(APawn* Vehicle, const FString& DisplayName, bool bIsPlayer);

	UFUNCTION(BlueprintCallable, Category = "Race")
	void UnregisterCompetitor(APawn* Vehicle);

	UFUNCTION(BlueprintCallable, Category = "Race")
	void ClearSession();

	UFUNCTION(BlueprintCallable, Category = "Race")
	bool StartRace(int32 InLapCount, float CountdownSeconds = 3.0f);

	UFUNCTION(BlueprintCallable, Category = "Race")
	void EndRace();

	UFUNCTION(BlueprintPure, Category = "Race")
	ERaceState GetRaceState() const { return RaceState; }

	UFUNCTION(BlueprintPure, Category = "Race")
	float GetCountdownRemaining() const { return CountdownRemaining; }

	UFUNCTION(BlueprintPure, Category = "Race")
	float GetRaceElapsedTime() const;

	UFUNCTION(BlueprintPure, Category = "Race")
	int32 GetLapCount() const { return LapCount; }

	UFUNCTION(BlueprintCallable, Category = "Race")
	const TArray<FRaceCompetitor>& GetStandings() const { return Competitors; }

	const FRaceCompetitor* FindCompetitor(const APawn* Vehicle) const;

	UFUNCTION(BlueprintPure, Category = "Race")
	static FString FormatLapTime(float Seconds);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

protected:

	void SetRaceState(ERaceState NewState);

	void SetCompetitorsDriving(bool bDriving);

	void UpdateCompetitorProgress(FRaceCompetitor& Competitor, float WorldTime);

	void HandleLapCompletion(FRaceCompetitor& Competitor, float WorldTime);

	void UpdateStandings();

	void RemoveStaleCompetitors();

	static AVehicleAIController* GetAIControllerFor(const FRaceCompetitor& Competitor);

private:

	FRacingLine ReferenceLine;

	UPROPERTY(Transient)
	TArray<FRaceCompetitor> Competitors;

	ERaceState RaceState = ERaceState::Idle;

	int32 LapCount = 3;

	float CountdownRemaining = 0.0f;

	float RaceStartTime = 0.0f;

	int32 FinishedCount = 0;
};
