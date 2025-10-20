#pragma once

#include "CoreMinimal.h"
#include "ShopTransactionData.generated.h"

class UItemBase;
class UInventoryComponent;
class AFishingCharacter;

USTRUCT(BlueprintType)
struct FPendingTransactionItem
{
	GENERATED_BODY()

	UPROPERTY()
	UItemBase* Item = nullptr;

	UPROPERTY()
	UInventoryComponent* SourceInventory = nullptr;

	UPROPERTY()
	UInventoryComponent* TargetInventory = nullptr;

	UPROPERTY()
	FIntPoint OriginalTile = FIntPoint::ZeroValue;

	UPROPERTY()
	FIntPoint CurrentTile = FIntPoint::ZeroValue;

	UPROPERTY()
	bool bIsSelling = false;

	UPROPERTY()
	int32 Price = 0;

	FPendingTransactionItem()
	{
	}

	FPendingTransactionItem(UItemBase* InItem, UInventoryComponent* InSource, UInventoryComponent* InTarget,
	                        FIntPoint InOriginalTile, FIntPoint InCurrentTile, bool bInIsSelling, int32 InPrice)
		: Item(InItem)
		  , SourceInventory(InSource)
		  , TargetInventory(InTarget)
		  , OriginalTile(InOriginalTile)
		  , CurrentTile(InCurrentTile)
		  , bIsSelling(bInIsSelling)
		  , Price(InPrice)
	{
	}

	bool IsValid() const
	{
		return Item != nullptr && SourceInventory != nullptr && TargetInventory != nullptr;
	}
};

UCLASS()
class FISHING_API UShopTransactionManager : public UObject
{
	GENERATED_BODY()

public:
	void AddPendingItem(const FPendingTransactionItem& PendingItem);

	bool RemovePendingItem(UItemBase* Item);

	bool IsPending(UItemBase* Item) const;

	int32 GetTotalSellPrice() const;

	int32 GetTotalBuyPrice() const;

	void ConfirmSell(AFishingCharacter* Player);

	void ConfirmBuy(AFishingCharacter* Player);

	void CancelAll();

	const TArray<FPendingTransactionItem>& GetPendingItems() const { return PendingItems; }

	void Initialize(UInventoryComponent* InPlayerInventory, UInventoryComponent* InShopInventory);

private:
	UPROPERTY()
	TArray<FPendingTransactionItem> PendingItems;

	UPROPERTY()
	UInventoryComponent* PlayerInventory = nullptr;

	UPROPERTY()
	UInventoryComponent* ShopInventory = nullptr;

	int32 FindPendingItemIndex(UItemBase* Item) const;
};