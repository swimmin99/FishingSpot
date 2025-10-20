


#pragma once

#include "CoreMinimal.h"
#include "../InventoryTypes.h"
#include "InventoryStorage.generated.h"

class UInventoryGridManager;
class UItemBase;


UCLASS()
class FISHING_API UInventoryStorage : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UInventoryGridManager* InGridManager, UObject* InOuter);
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
    
	
	TArray<FItemSyncData> GenerateSyncData() const;
	void ApplySyncData(const TArray<FItemSyncData>& SyncData);
    
	FString DumpStorageContents() const;
	void PlaceItemInGrid(UItemBase* Item, int32 TopLeftIndex);

private:
	
	UPROPERTY()
	TArray<UItemBase*> Items;
    
	TMap<UItemBase*, FIntPoint> CachedUniqueItems;
    
	UPROPERTY()
	UInventoryGridManager* GridManager;
    
	UPROPERTY()
	UObject* Outer;
    
	
	

	void CreateItemFromSyncData(const FItemSyncData& SyncData);
};
