#include "Variant_Fishing/Data/ShopTransactionData.h"

#include "FishingCharacter.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogShopTransaction, Log, All);

void UShopTransactionManager::Initialize(UInventoryComponent* InPlayerInventory, UInventoryComponent* InShopInventory)
{
	PlayerInventory = InPlayerInventory;
	ShopInventory = InShopInventory;
	PendingItems.Empty();

	UE_LOG(LogShopTransaction, Log, TEXT("Initialize: Player=%s, Shop=%s"),
	       PlayerInventory ? *PlayerInventory->GetOwner()->GetName() : TEXT("NULL"),
	       ShopInventory ? *ShopInventory->GetOwner()->GetName() : TEXT("NULL"));
}

void UShopTransactionManager::AddPendingItem(const FPendingTransactionItem& PendingItem)
{
	if (!PendingItem.IsValid())
	{
		UE_LOG(LogShopTransaction, Warning, TEXT("AddPendingItem: Invalid item"));
		return;
	}

	RemovePendingItem(PendingItem.Item);

	PendingItems.Add(PendingItem);

	UE_LOG(LogShopTransaction, Log, TEXT("AddPendingItem: %s, Type=%s, Price=%d"),
	       *PendingItem.Item->GetName(),
	       PendingItem.bIsSelling ? TEXT("Sell") : TEXT("Buy"),
	       PendingItem.Price);
}

bool UShopTransactionManager::RemovePendingItem(UItemBase* Item)
{
	int32 Index = FindPendingItemIndex(Item);
	if (Index != INDEX_NONE)
	{
		PendingItems.RemoveAt(Index);
		UE_LOG(LogShopTransaction, Log, TEXT("RemovePendingItem: %s removed"), *Item->GetName());
		return true;
	}
	return false;
}

bool UShopTransactionManager::IsPending(UItemBase* Item) const
{
	return FindPendingItemIndex(Item) != INDEX_NONE;
}

int32 UShopTransactionManager::FindPendingItemIndex(UItemBase* Item) const
{
	for (int32 i = 0; i < PendingItems.Num(); ++i)
	{
		if (PendingItems[i].Item == Item)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UShopTransactionManager::GetTotalSellPrice() const
{
	int32 Total = 0;
	for (const FPendingTransactionItem& Item : PendingItems)
	{
		if (Item.bIsSelling)
		{
			Total += Item.Price;
		}
	}
	return Total;
}

int32 UShopTransactionManager::GetTotalBuyPrice() const
{
	int32 Total = 0;
	for (const FPendingTransactionItem& Item : PendingItems)
	{
		if (!Item.bIsSelling)
		{
			Total += Item.Price;
		}
	}
	return Total;
}

void UShopTransactionManager::ConfirmSell(AFishingCharacter* Player)
{
	if (!Player || !PlayerInventory || !ShopInventory)
	{
		UE_LOG(LogShopTransaction, Error, TEXT("ConfirmSell: Invalid state"));
		return;
	}

	UE_LOG(LogShopTransaction, Log, TEXT("ConfirmSell: Starting sell confirmation"));

	int32 TotalEarned = 0;
	TArray<UItemBase*> ItemsToRemove;

	for (const FPendingTransactionItem& PendingItem : PendingItems)
	{
		if (PendingItem.bIsSelling)
		{
			UE_LOG(LogShopTransaction, Log, TEXT("ConfirmSell: Selling %s for %d"),
			       *PendingItem.Item->GetName(), PendingItem.Price);

			TotalEarned += PendingItem.Price;
			ItemsToRemove.Add(PendingItem.Item);
		}
	}

	for (UItemBase* Item : ItemsToRemove)
	{
		PlayerInventory->RemoveItem(Item);
		RemovePendingItem(Item);
	}

	Player->AddGold(TotalEarned);

	UE_LOG(LogShopTransaction, Log, TEXT("ConfirmSell: Sold %d items for %d gold"),
	       ItemsToRemove.Num(), TotalEarned);

	PlayerInventory->RefreshGridLayout();
	ShopInventory->RefreshGridLayout();
}

void UShopTransactionManager::ConfirmBuy(AFishingCharacter* Player)
{
	if (!Player || !PlayerInventory || !ShopInventory)
	{
		UE_LOG(LogShopTransaction, Error, TEXT("ConfirmBuy: Invalid state"));
		return;
	}

	int32 TotalCost = GetTotalBuyPrice();

	if (Player->GetGold() < TotalCost)
	{
		UE_LOG(LogShopTransaction, Warning, TEXT("ConfirmBuy: Not enough gold. Need %d, Have %d"),
		       TotalCost, Player->GetGold());

		return;
	}

	UE_LOG(LogShopTransaction, Log, TEXT("ConfirmBuy: Starting buy confirmation"));

	TArray<UItemBase*> ItemsToRemove;

	for (const FPendingTransactionItem& PendingItem : PendingItems)
	{
		if (!PendingItem.bIsSelling)
		{
			UE_LOG(LogShopTransaction, Log, TEXT("ConfirmBuy: Buying %s for %d"),
			       *PendingItem.Item->GetName(), PendingItem.Price);

			ItemsToRemove.Add(PendingItem.Item);
		}
	}

	for (UItemBase* Item : ItemsToRemove)
	{
		ShopInventory->RemoveItem(Item);
		RemovePendingItem(Item);
	}

	Player->AddGold(-TotalCost);

	UE_LOG(LogShopTransaction, Log, TEXT("ConfirmBuy: Bought %d items for %d gold"),
	       ItemsToRemove.Num(), TotalCost);

	PlayerInventory->RefreshGridLayout();
	ShopInventory->RefreshGridLayout();
}

void UShopTransactionManager::CancelAll()
{
	UE_LOG(LogShopTransaction, Log, TEXT("CancelAll: Cancelling %d pending transactions"), PendingItems.Num());

	for (const FPendingTransactionItem& PendingItem : PendingItems)
	{
		if (!PendingItem.IsValid())
		{
			continue;
		}

		PendingItem.TargetInventory->RemoveItem(PendingItem.Item);

		int32 OriginalIndex = PendingItem.SourceInventory->TileToIndex(PendingItem.OriginalTile);
		PendingItem.SourceInventory->AddItemAt(PendingItem.Item,  OriginalIndex);
		

		UE_LOG(LogShopTransaction, Log, TEXT("CancelAll: Restored %s to original position"),
		       *PendingItem.Item->GetName());
	}

	PendingItems.Empty();

	if (PlayerInventory)
	{
		PlayerInventory->RefreshGridLayout();
	}
	if (ShopInventory)
	{
		ShopInventory->RefreshGridLayout();
	}
}