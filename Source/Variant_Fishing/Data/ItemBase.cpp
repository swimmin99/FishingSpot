#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"

AItemActor* UItemBase::SpawnItemActor(UWorld* World, const FTransform& Transform,
                                      const TSubclassOf<AItemActor> ActorClass, AActor* Owner)
{
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemActor* Spawned = World->SpawnActor<AItemActor>(ActorClass, Transform, Params);
	if (!Spawned)
	{
		return nullptr;
	}

	Spawned->InitFromItem(this);
	return Spawned;
}

void UItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UItemBase, ItemDef);
	DOREPLIFETIME(UItemBase, bIsRotated);
	DOREPLIFETIME(UItemBase, ItemGuid);
}

bool UItemBase::Initialize(UItemDataAsset* InData)
{
	ItemDef = InData;

	if (!ItemGuid.IsValid())
	{
		ItemGuid = FGuid::NewGuid();
	}

	return ItemDef != nullptr;
}
