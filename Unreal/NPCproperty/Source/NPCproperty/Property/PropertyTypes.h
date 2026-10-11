#pragma once

#include "CoreMinimal.h"
#include "PropertyTypes.generated.h"

UENUM(BlueprintType)
enum class EPropertyOwnershipMode : uint8
{
	InheritRoom UMETA(DisplayName="Room owner"),
	Unowned UMETA(DisplayName="Unowned"),
	ExplicitOwner UMETA(DisplayName="Specific owner")
};

USTRUCT(BlueprintType)
struct NPCPROPERTY_API FPropertyIdentity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FName PersonId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FName FactionId;
};

USTRUCT(BlueprintType)
struct NPCPROPERTY_API FPropertyPermission
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Permission")
	bool bPublic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Permission")
	TArray<FName> InvitedPeople;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Permission")
	TArray<FName> AllowedFactions;

	bool Allows(const FPropertyIdentity& Visitor, const FPropertyIdentity& Owner) const;
};

USTRUCT(BlueprintType)
struct NPCPROPERTY_API FOwnedItemRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FGuid ItemId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FPropertyIdentity LegalOwner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	bool bStolen = false;
};

UENUM(BlueprintType)
enum class EPropertyEvidenceKind : uint8 { WitnessedTheft, RecoveredGoods };

USTRUCT(BlueprintType)
struct NPCPROPERTY_API FTheftIncident
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crime") EPropertyEvidenceKind EvidenceKind = EPropertyEvidenceKind::WitnessedTheft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crime")
	FGuid IncidentId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crime")
	FOwnedItemRecord Item;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crime")
	FPropertyIdentity Offender;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crime")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crime")
	float WorldTime = 0.f;
};
