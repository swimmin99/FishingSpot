// ============================================
// InventoryStorage.h (수정)
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "../InventoryTypes.h"
#include "InventoryStorage.generated.h"

class UInventoryGridManager;
class UItemBase;

/**
 * 인벤토리 아이템 물리적 저장소
 */
UCLASS()
class FISHING_API UInventoryStorage : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UInventoryGridManager* InGridManager, UObject* InOuter);
	void ResizeStorage(bool bZeroInit);
    
	// 아이템 접근
	UItemBase* GetItemAtIndex(int32 Index) const;
	void SetItemAtIndex(int32 Index, UItemBase* Item);
	void ClearItemAtIndex(int32 Index);
	void ClearAllOccurrences(UItemBase* Item);
	void ClearAll();
    
	// 유니크 아이템 관리
	TMap<UItemBase*, FIntPoint> GetAllUniqueItems();
	void RefreshUniqueItemsCache();
	const TMap<UItemBase*, FIntPoint>& GetCachedUniqueItems() const { return CachedUniqueItems; }
    
	const TArray<UItemBase*>& GetItemsArray() const { return Items; }
	int32 GetItemCount() const { return Items.Num(); }
    
	// ★★★ 레플리케이션 지원 함수 ★★★
	TArray<FItemSyncData> GenerateSyncData() const;
	void ApplySyncData(const TArray<FItemSyncData>& SyncData);
    
	FString DumpStorageContents() const;

private:
	// 로컬 아이템 배열 (레플리케이트되지 않음)
	UPROPERTY()
	TArray<UItemBase*> Items;
    
	TMap<UItemBase*, FIntPoint> CachedUniqueItems;
    
	UPROPERTY()
	UInventoryGridManager* GridManager;
    
	UPROPERTY()
	UObject* Outer;
    
	// 헬퍼 함수
	UItemBase* CreateItemFromSyncData(const FItemSyncData& SyncData);
	void PlaceItemInGrid(UItemBase* Item, int32 TopLeftIndex);
};
