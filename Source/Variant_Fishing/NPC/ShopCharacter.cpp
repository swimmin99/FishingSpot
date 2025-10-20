#include "ShopCharacter.h"
#include "FishingCharacter.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogShop, Log, All);

AShopCharacter::AShopCharacter()
{
	bReplicates = true;

	
	PrimaryActorTick.bCanEverTick = false;

	ShopInventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("ShopInventoryComponent"));
	
	AutoPossessAI = EAutoPossessAI::Disabled;

}

void AShopCharacter::BeginPlay()
{
	Super::BeginPlay();
	ShopInventoryComponent->TurnReplicationOff();

	InitializeShop();
	UE_LOG(LogShop, Log, TEXT("ShopCharacter '%s' initialized on server"), *ShopName);
	bReplicates = false;

	PlayIdleLoop();
}

FString AShopCharacter::GetInteractableName_Implementation() const
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
	ShopInventoryComponent->Initalize(this);

	ShopInventoryComponent->ResizeItemsToGrid(true);

	UE_LOG(LogShop, Log, TEXT("InitializeShop: '%s' Grid=%dx%d"),
	       *ShopName, ShopColumns, ShopRows);

	SpawnInitialItems();
}

void AShopCharacter::SpawnInitialItems()
{
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

	for (TObjectPtr<UPrimaryDataAsset> ItemObject : InitialShopItems)
	{
		if (!ItemObject)
		{
			UE_LOG(LogShop, Warning, TEXT("SpawnInitialItems: Null ItemObject in InitialShopItems"));
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		if (ItemObject->GetClass()->ImplementsInterface(UItemDataProvider::StaticClass()))
		{
			UItemBase * Item = IItemDataProvider::Execute_CreateBaseItem(ItemObject, this, ShopName);
			if (Item)
			{
				bool bAdded = ShopInventoryComponent->TryAddItem(Item);

				if (bAdded)
				{
					UE_LOG(LogShop, Log, TEXT("SpawnInitialItems: Added %s to shop"),
						   *Item->GetName());
				}
				else
				{
					UE_LOG(LogShop, Warning, TEXT("SpawnInitialItems: No room for %s"),
						   *Item->GetName());
				}
			}
		}
		else
		{
			UE_LOG(LogShop, Warning, TEXT("Item %s does not implement IItemDataProvider"), *ItemObject->GetName());
		}
		
		
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
