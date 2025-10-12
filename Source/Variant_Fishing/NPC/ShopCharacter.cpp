#include "ShopCharacter.h"
#include "FishingCharacter.h"
#include "Variant_Fishing/ActorComponent/InventoryComponent.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogShop, Log, All);

AShopCharacter::AShopCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	ShopInventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("ShopInventoryComponent"));

	AutoPossessAI = EAutoPossessAI::Disabled;

	bReplicates = true;
}

void AShopCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitializeShop();
		UE_LOG(LogShop, Log, TEXT("ShopCharacter '%s' initialized on server"), *ShopName);
	}

	PlayIdleLoop();
}

FString AShopCharacter::GetName_Implementation() const
{
	return ShopName;
}

EInteractionType AShopCharacter::GetType_Implementation() const
{
	return EInteractionType::Shop;
}

void AShopCharacter::InitializeShop()
{
	if (!ShopInventoryComponent)
	{
		UE_LOG(LogShop, Error, TEXT("InitializeShop: ShopInventoryComponent is null!"));
		return;
	}

	ShopInventoryComponent->Columns = ShopColumns;
	ShopInventoryComponent->Rows = ShopRows;
	ShopInventoryComponent->TileSize = ShopTileSize;
	ShopInventoryComponent->Initalize(this);

	ShopInventoryComponent->ResizeItemsToGrid(true);

	UE_LOG(LogShop, Log, TEXT("InitializeShop: '%s' Grid=%dx%d"),
	       *ShopName, ShopColumns, ShopRows);

	SpawnInitialItems();
}

void AShopCharacter::SpawnInitialItems()
{
	if (!HasAuthority())
	{
		return;
	}
	if (!ShopInventoryComponent)
	{
		return;
	}

	if (InitialShopItems.Num() == 0)
	{
		UE_LOG(LogShop, Warning, TEXT("SpawnInitialItems: No initial items configured for '%s'"),
		       *ShopName);
		return;
	}

	UE_LOG(LogShop, Log, TEXT("SpawnInitialItems: Spawning %d items for '%s'"),
	       InitialShopItems.Num(), *ShopName);

	for (TSubclassOf<AItemActor> ItemClass : InitialShopItems)
	{
		if (!ItemClass)
		{
			UE_LOG(LogShop, Warning, TEXT("SpawnInitialItems: Null ItemClass in InitialShopItems"));
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AItemActor* TempItem = GetWorld()->SpawnActor<AItemActor>(
			ItemClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!TempItem)
		{
			UE_LOG(LogShop, Warning, TEXT("SpawnInitialItems: Failed to spawn ItemActor"));
			continue;
		}

		UItemBase* NewItem = TempItem->MakeItemInstance(ShopInventoryComponent, this);

		if (NewItem)
		{
			bool bAdded = ShopInventoryComponent->TryAddItem(NewItem);

			if (bAdded)
			{
				UE_LOG(LogShop, Log, TEXT("SpawnInitialItems: Added %s to shop"),
				       *NewItem->GetName());
			}
			else
			{
				UE_LOG(LogShop, Warning, TEXT("SpawnInitialItems: No room for %s"),
				       *NewItem->GetName());
			}
		}

		TempItem->Destroy();
	}

	ShopInventoryComponent->RefreshAllItems();

	UE_LOG(LogShop, Log, TEXT("SpawnInitialItems: Complete for '%s'"), *ShopName);
}

bool AShopCharacter::IsInInteractionRange(AActor* OtherActor) const
{
	if (!OtherActor)
	{
		return false;
	}

	const float Distance = FVector::Dist(GetActorLocation(), OtherActor->GetActorLocation());
	return Distance <= InteractionRange;
}

void AShopCharacter::PlayIdleLoop()
{
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		if (IdleLoopAnimation)
		{
			Skel->PlayAnimation(IdleLoopAnimation, true);
		}
	}
}
