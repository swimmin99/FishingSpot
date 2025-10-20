#include "ItemBase.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Fishing.h"

AItemActor* UItemBase::SpawnItemActor(UWorld* World, const FTransform& Transform,
									  const TSubclassOf<AItemActor> ActorClass, AActor* Owner)
{
	if (!World || !ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ItemBase::SpawnItemActor - Invalid World or ActorClass"));
		return nullptr;
	}

	if (!ItemDataProvider)
	{
		UE_LOG(LogTemp, Error, TEXT("ItemBase::SpawnItemActor - No ItemDataProvider set"));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemActor* SpawnedActor = World->SpawnActor<AItemActor>(ActorClass, Transform, SpawnParams);

	if (SpawnedActor)
	{
		
		SpawnedActor->InitializeFromItem(this);

		
		if (IsFish())
		{
			UE_LOG(LogTemp, Log, TEXT("ItemBase::SpawnItemActor - Spawned Fish:\n%s"), 
				   *GetDebugString());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("ItemBase::SpawnItemActor - Spawned: %s"), 
				   *DisplayName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ItemBase::SpawnItemActor - Failed to spawn actor"));
	}

	return SpawnedActor;
}

void UItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemBase, ItemGuid);
	DOREPLIFETIME(UItemBase, ItemDataProvider);
	DOREPLIFETIME(UItemBase, bIsRotated);
	DOREPLIFETIME(UItemBase, SpecificData);
}