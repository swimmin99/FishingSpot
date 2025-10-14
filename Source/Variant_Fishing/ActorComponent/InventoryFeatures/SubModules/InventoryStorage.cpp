// ============================================
// InventoryStorage.cpp (수정)
// ============================================
#include "InventoryStorage.h"

#include "Fishing.h"
#include "InventoryGridManager.h"
#include "Variant_Fishing/Data/ItemBase.h"

void UInventoryStorage::Initialize(UInventoryGridManager* InGridManager, UObject* InOuter)
{
    if (!InGridManager || !InOuter)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("Initialize: Null parameters!"));
        return;
    }
    
    GridManager = InGridManager;
    Outer = InOuter;
    ResizeStorage(true);
    
    UE_LOG(LogInventoryStorage, Log, TEXT("Initialize: Storage ready with %d slots"), Items.Num());
}

void UInventoryStorage::ResizeStorage(bool bZeroInit)
{
    if (!GridManager)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("ResizeStorage: GridManager is null!"));
        return;
    }
    
    const int32 Expected = GridManager->GetTotalTiles();
    if (Expected <= 0)
    {
        UE_LOG(LogInventoryStorage, Warning, TEXT("ResizeStorage: Invalid tile count %d"), Expected);
        return;
    }
    
    const int32 OldSize = Items.Num();
    
    if (bZeroInit)
    {
        Items.SetNumZeroed(Expected);
    }
    else
    {
        Items.SetNum(Expected);
    }
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ResizeStorage: %d -> %d slots"), OldSize, Expected);
}

UItemBase* UInventoryStorage::GetItemAtIndex(int32 Index) const
{
    return Items.IsValidIndex(Index) ? Items[Index] : nullptr;
}

void UInventoryStorage::SetItemAtIndex(int32 Index, UItemBase* Item)
{
    if (!Items.IsValidIndex(Index))
    {
        UE_LOG(LogInventoryStorage, Warning, TEXT("SetItemAtIndex: Invalid index %d"), Index);
        return;
    }
    
    Items[Index] = Item;
}

void UInventoryStorage::ClearItemAtIndex(int32 Index)
{
    SetItemAtIndex(Index, nullptr);
}

void UInventoryStorage::ClearAllOccurrences(UItemBase* Item)
{
    if (!Item) return;
    
    int32 ClearedCount = 0;
    for (int32 i = 0; i < Items.Num(); i++)
    {
        if (Items[i] == Item)
        {
            Items[i] = nullptr;
            ClearedCount++;
        }
    }
    
    UE_LOG(LogInventoryStorage, Verbose, TEXT("ClearAllOccurrences: Cleared %d slots"), ClearedCount);
}

void UInventoryStorage::ClearAll()
{
    for (int32 i = 0; i < Items.Num(); i++)
    {
        Items[i] = nullptr;
    }
    CachedUniqueItems.Empty();
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ClearAll: All storage cleared"));
}

TMap<UItemBase*, FIntPoint> UInventoryStorage::GetAllUniqueItems()
{
    if (!GridManager)
    {
        return TMap<UItemBase*, FIntPoint>();
    }
    
    TMap<UItemBase*, FIntPoint> UniqueItems;
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (UItemBase* Item = Items[i])
        {
            if (!UniqueItems.Contains(Item))
            {
                UniqueItems.Add(Item, GridManager->IndexToTile(i));
            }
        }
    }
    
    return UniqueItems;
}

void UInventoryStorage::RefreshUniqueItemsCache()
{
    CachedUniqueItems = GetAllUniqueItems();
    UE_LOG(LogInventoryStorage, Log, TEXT("RefreshUniqueItemsCache: Cached %d items"), 
           CachedUniqueItems.Num());
}

// ★★★ 서버에서 호출: 동기화 데이터 생성 ★★★
TArray<FItemSyncData> UInventoryStorage::GenerateSyncData() const
{
    TArray<FItemSyncData> SyncData;
    TSet<FGuid> ProcessedGuids;
    
    if (!GridManager)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("GenerateSyncData: GridManager is null!"));
        return SyncData;
    }
    
    // 유니크 아이템만 전송 (같은 아이템이 여러 슬롯 차지하므로)
    for (int32 i = 0; i < Items.Num(); i++)
    {
        UItemBase* Item = Items[i];
        
        if (!Item || !Item->ItemDef)
            continue;
        
        // 이미 처리한 아이템이면 스킵
        if (ProcessedGuids.Contains(Item->ItemGuid))
            continue;
        
        // 이 아이템의 TopLeft 위치 찾기 (첫 등장 위치)
        int32 TopLeftIndex = i;
        
        FItemSyncData Data(
            Item->ItemDef,
            Item->ItemGuid,
            Item->bIsRotated,
            TopLeftIndex
        );
        
        SyncData.Add(Data);
        ProcessedGuids.Add(Item->ItemGuid);
        
        UE_LOG(LogInventoryStorage, Verbose, TEXT("GenerateSyncData: Added %s at index %d"),
               *Item->GetName(), TopLeftIndex);
    }
    
    UE_LOG(LogInventoryStorage, Log, TEXT("GenerateSyncData: Generated %d sync entries"), 
           SyncData.Num());
    
    return SyncData;
}

// ★★★ 클라이언트에서 호출: 동기화 데이터 적용 ★★★
void UInventoryStorage::ApplySyncData(const TArray<FItemSyncData>& SyncData)
{
    if (!GridManager || !Outer)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("ApplySyncData: Dependencies not initialized!"));
        return;
    }
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ApplySyncData: Applying %d sync entries"), 
           SyncData.Num());
    
    // 기존 아이템 전부 클리어
    ClearAll();
    
    // 동기화 데이터로부터 아이템 재생성 및 배치
    for (const FItemSyncData& Data : SyncData)
    {
        if (!Data.IsValid())
        {
            UE_LOG(LogInventoryStorage, Warning, TEXT("ApplySyncData: Invalid sync data"));
            continue;
        }
        
        // UItemBase 생성
        UItemBase* NewItem = CreateItemFromSyncData(Data);
        if (!NewItem)
        {
            UE_LOG(LogInventoryStorage, Error, TEXT("ApplySyncData: Failed to create item"));
            continue;
        }
        
        // 그리드에 배치
        PlaceItemInGrid(NewItem, Data.TopLeftIndex);
        
        UE_LOG(LogInventoryStorage, Verbose, TEXT("ApplySyncData: Placed %s at index %d"),
               *NewItem->GetName(), Data.TopLeftIndex);
    }
    
    // 캐시 갱신
    RefreshUniqueItemsCache();
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ApplySyncData: Complete. %d unique items placed"),
           CachedUniqueItems.Num());
}

UItemBase* UInventoryStorage::CreateItemFromSyncData(const FItemSyncData& SyncData)
{
    if (!Outer || !SyncData.ItemDef)
    {
        return nullptr;
    }
    
    UItemBase* NewItem = NewObject<UItemBase>(Outer, UItemBase::StaticClass());
    if (!NewItem)
    {
        return nullptr;
    }
    
    NewItem->Initialize(SyncData.ItemDef);
    NewItem->bIsRotated = SyncData.bIsRotated;
    NewItem->ItemGuid = SyncData.ItemGuid;
    
    return NewItem;
}

void UInventoryStorage::PlaceItemInGrid(UItemBase* Item, int32 TopLeftIndex)
{
    if (!Item || !GridManager)
    {
        return;
    }
    
    const FIntPoint CurrentDims = Item->GetCurrentDimensions();
    const FIntPoint TopLeft = GridManager->IndexToTile(TopLeftIndex);
    
    // 아이템이 차지하는 모든 타일에 배치
    for (int32 y = 0; y < CurrentDims.Y; ++y)
    {
        for (int32 x = 0; x < CurrentDims.X; ++x)
        {
            const FIntPoint Tile(TopLeft.X + x, TopLeft.Y + y);
            const int32 Index = GridManager->TileToIndex(Tile);
            
            if (GridManager->IsIndexValid(Index))
            {
                Items[Index] = Item;
            }
        }
    }
}

FString UInventoryStorage::DumpStorageContents() const
{
    if (!GridManager)
    {
        return TEXT("Storage: GridManager is null");
    }
    
    FString Result;
    Result += FString::Printf(TEXT("Storage Contents (%d/%d slots occupied):\n"),
                             CachedUniqueItems.Num(), Items.Num());
    
    const int32 Cols = GridManager->GetColumns();
    const int32 Rows = GridManager->GetRows();
    
    for (int32 y = 0; y < Rows; ++y)
    {
        for (int32 x = 0; x < Cols; ++x)
        {
            const int32 Index = GridManager->TileToIndex(FIntPoint(x, y));
            const UItemBase* Item = GetItemAtIndex(Index);
            Result += Item ? TEXT("[X]") : TEXT("[.]");
        }
        Result += TEXT("\n");
    }
    
    return Result;
}