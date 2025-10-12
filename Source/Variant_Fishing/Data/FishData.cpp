#include "Variant_Fishing/Data/FishData.h"

#include "ItemBase.h"

UItemBase* UFishData::CreateItemFromDataAsset(UObject* Outer)
{
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFishData::CreateItemFromDataAsset - ItemData is null"));
		return nullptr;
	}

	UItemBase* NewItem = NewObject<UItemBase>(Outer);
	if (NewItem && NewItem->Initialize(ItemData))
	{
		return NewItem;
	}

	UE_LOG(LogTemp, Warning, TEXT("Failed to initialize UItemBase from ItemData"));
	return nullptr;
}
