
// ============================================
// InventoryNetworkSync.cpp
// ============================================
#include "InventoryNetworkSync.h"
#include "../InventoryComponent.h"
#include "InventoryStorage.h"
#include "InventoryGridManager.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY(LogInventoryNetwork);

void UInventoryNetworkSync::Initialize(UInventoryComponent* InOwnerComponent,
									  UInventoryStorage* InStorage,
									  UInventoryGridManager* InGridManager)
{
	if (!InOwnerComponent || !InStorage || !InGridManager)
	{
		UE_LOG(LogInventoryNetwork, Error, TEXT("Initialize: Null dependencies!"));
		return;
	}
    
	OwnerComponent = InOwnerComponent;
	Storage = InStorage;
	GridManager = InGridManager;
    
	UE_LOG(LogInventoryNetwork, Log, TEXT("Initialize: NetworkSync ready"));
}

bool UInventoryNetworkSync::FindItemTopLeftIndex(UItemBase* Item, int32& OutIndex) const
{
	if (!Item || !Storage || !GridManager)
	{
		return false;
	}
    
	// Get all unique items with their top-left positions
	const TMap<UItemBase*, FIntPoint>& UniqueItems = Storage->GetCachedUniqueItems();
    
	if (const FIntPoint* TopLeftTile = UniqueItems.Find(Item))
	{
		OutIndex = GridManager->TileToIndex(*TopLeftTile);
		return true;
	}
    
	return false;
}

bool UInventoryNetworkSync::FindItemIndex(UItemBase* Item, int32& OutIndex) const
{
	if (!Item || !Storage || !GridManager)
	{
		return false;
	}
    
	const TArray<UItemBase*>& Items = Storage->GetItemsArray();
	const int32 TotalTiles = GridManager->GetTotalTiles();
    
	for (int32 i = 0; i < TotalTiles; i++)
	{
		if (Items.IsValidIndex(i) && Items[i] == Item)
		{
			OutIndex = i;
			return true;
		}
	}
    
	return false;
}