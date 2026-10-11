#include "Property/OwnedPickup.h"
#include "Property/OwnershipComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AOwnedPickup::AOwnedPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Ownership = CreateDefaultSubobject<UOwnershipComponent>(TEXT("Ownership"));
	DisplayName = FText::FromString(TEXT("Owned item"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetWorldScale3D(FVector(.25f));
}

void AOwnedPickup::BeginPlay()
{
	Super::BeginPlay();
	if (!ItemId.IsValid()) ItemId = FGuid::NewGuid();
}

void AOwnedPickup::PostActorCreated()
{
	Super::PostActorCreated();
	if (!ItemId.IsValid()) ItemId = FGuid::NewGuid();
}

#if WITH_EDITOR
void AOwnedPickup::PostEditImport()
{
	Super::PostEditImport();
	ItemId = FGuid::NewGuid();
}

void AOwnedPickup::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	if (DuplicateMode != EDuplicateMode::PIE) ItemId = FGuid::NewGuid();
}
#endif

bool AOwnedPickup::Consume(const FPropertyIdentity& Taker, FOwnedItemRecord& OutItem)
{
	if (!bAvailable || !HasAuthority()) return false;
	bAvailable = false;
	if (!ItemId.IsValid()) ItemId = FGuid::NewGuid();
	OutItem.ItemId = ItemId;
	OutItem.DisplayName = DisplayName;
	OutItem.LegalOwner = Ownership->GetLegalOwner();
	OutItem.bStolen = !Ownership->CanTake(Taker);
	return true;
}
