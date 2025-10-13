// ============================================
// InventoryStorage.h
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryStorage.generated.h"

class UInventoryGridManager;
class UItemBase;

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryStorage, Log, All);

/**
 * Manages physical storage of items in the grid array
 */
UCLASS()
class FISHING_API UInventoryStorage : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UInventoryGridManager* InGridManager);
	void ResizeStorage(bool bZeroInit);
    
	UItemBase* GetItemAtIndex(int32 Index) const;
	void SetItemAtIndex(int32 Index, UItemBase* Item);
	void ClearItemAtIndex(int32 Index);
	void ClearAllOccurrences(UItemBase* Item);
	void ClearAll();
    
	TMap<UItemBase*, FIntPoint> GetAllUniqueItems();
	void RefreshUniqueItemsCache();
	const TMap<UItemBase*, FIntPoint>& GetCachedUniqueItems() const { return CachedUniqueItems; }
    
	const TArray<UItemBase*>& GetItemsArray() const { return Items; }
	int32 GetItemCount() const { return Items.Num(); }
    
	FString DumpStorageContents() const;

private:
	UPROPERTY()
	TArray<UItemBase*> Items;
    
	TMap<UItemBase*, FIntPoint> CachedUniqueItems;
    
	UPROPERTY()
	UInventoryGridManager* GridManager;
};