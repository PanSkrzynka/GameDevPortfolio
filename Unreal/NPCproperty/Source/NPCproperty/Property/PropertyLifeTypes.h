#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Engine/DataAsset.h"
#include "Property/PropertyTypes.h"
#include "PropertyLifeTypes.generated.h"

UENUM(BlueprintType)
enum class EConfrontationChoice : uint8
{
	ReturnGoods,
	PayCompensation,
	Resist,
	Submit
};

USTRUCT(BlueprintType)
struct NPCPROPERTY_API FPropertyRoutineEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float StartHour = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EndHour = 24.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Activity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> Offsets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1")) float WaitSeconds = 5.f;
};

UCLASS(BlueprintType)
class NPCPROPERTY_API UPropertyRoutine : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPropertyRoutineEntry> Entries;
};

UENUM(BlueprintType)
enum class EPropertyAlertLevel : uint8 { Calm, Suspicious, Searching, Lockdown };

UENUM(BlueprintType)
enum class EPropertySearchRole : uint8 { None, InspectRoom, WatchExit, SweepNearby };

USTRUCT(BlueprintType)
struct FPropertyMissingReport
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid ItemId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Source;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName PossiblePerson;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Age = 0.f;
};

USTRUCT(BlueprintType)
struct FPropertyObjectMemory
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid ItemId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bMissing = false;
    UPROPERTY() FName NearbyPerson;
    UPROPERTY() float ObservationAge = 1000.f;
};

USTRUCT()
struct FPropertySavedPickup
{
	GENERATED_BODY()
	UPROPERTY() FSoftClassPath ActorClass;
	UPROPERTY() FTransform Transform;
	UPROPERTY() FOwnedItemRecord Item;
	UPROPERTY() FPropertyPermission Permission;
	UPROPERTY() EPropertyOwnershipMode OwnershipMode = EPropertyOwnershipMode::ExplicitOwner;
	UPROPERTY() FPropertyIdentity OwnerOverride;
};

USTRUCT()
struct FPropertySavedNPC
{
	GENERATED_BODY()
	UPROPERTY() FName PersonId;
	UPROPERTY() FTransform Transform;
	UPROPERTY() FVector Home = FVector::ZeroVector;
	UPROPERTY() FRotator HomeRotation = FRotator::ZeroRotator;
	UPROPERTY() TArray<FTheftIncident> Memories;
	UPROPERTY() TMap<FGuid, FName> Sources;
	UPROPERTY() TArray<FPropertyObjectMemory> ObjectMemories;
	UPROPERTY() TArray<FPropertyMissingReport> MissingReports;
	UPROPERTY() int32 RoutineEntry = INDEX_NONE;
	UPROPERTY() int32 Waypoint = 0;
	UPROPERTY() float Health = 100.f;
	UPROPERTY() float RecoverySeconds = 0.f;
};

USTRUCT()
struct FPropertySavedArea
{
	GENERATED_BODY()
	UPROPERTY() FName ActorName;
	UPROPERTY() float AlertScore = 0.f;
	UPROPERTY() float QuietSeconds = 0.f;
	UPROPERTY() TSet<FGuid> AlertedItems;
	UPROPERTY() TSet<FGuid> ReportedItems;
	UPROPERTY() TArray<FName> AssignedNPCs;
	UPROPERTY() FPropertyIdentity Owner;
	UPROPERTY() FPropertyPermission Permission;
};

USTRUCT(BlueprintType)
struct FPropertyBounty
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FTheftIncident Incident;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Reporter;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ReceivingGuard;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Amount = 100;
};

UCLASS()
class NPCPROPERTY_API UPropertySaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() int32 Version = 4;
	UPROPERTY() FString Map;
	UPROPERTY() FTransform PlayerTransform;
	UPROPERTY() bool bPlayerCrouched = false;
	UPROPERTY() TArray<FOwnedItemRecord> Inventory;
	UPROPERTY() int32 Gold = 300;
	UPROPERTY() float Health = 100.f;
	UPROPERTY() float Hour = 12.f;
	UPROPERTY() TArray<FPropertySavedPickup> Pickups;
	UPROPERTY() TArray<FPropertySavedNPC> NPCs;
	UPROPERTY() TArray<FPropertySavedArea> Areas;
	UPROPERTY() TArray<FTheftIncident> Incidents;
	UPROPERTY() TSet<FGuid> Settled;
	UPROPERTY() TArray<FOwnedItemRecord> ReturnedGoods;
	UPROPERTY() TArray<FPropertyBounty> Bounties;
};
