// ============================================
// InventoryStorage.cpp
// ============================================
#include "InventoryStorage.h"
#include "InventoryGridManager.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY(LogInventoryStorage);

void UInventoryStorage::Initialize(UInventoryGridManager* InGridManager)
{
    if (!InGridManager)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("Initialize: GridManager is null!"));
        return;
    }
    
    GridManager = InGridManager;
    ResizeStorage(true);
    
    UE_LOG(LogInventoryStorage, Log, TEXT("Initialize: Storage ready with %d slots"), 
           Items.Num());
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
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ResizeStorage: %d -> %d slots (ZeroInit=%s)"),
           OldSize, Expected, bZeroInit ? TEXT("Yes") : TEXT("No"));
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
    if (!Item)
    {
        return;
    }
    
    int32 ClearedCount = 0;
    
    for (int32 i = 0; i < Items.Num(); i++)
    {
        if (Items[i] == Item)
        {
            Items[i] = nullptr;
            ClearedCount++;
        }
    }
    
    UE_LOG(LogInventoryStorage, Verbose, TEXT("ClearAllOccurrences: Cleared %d slots for %s"),
           ClearedCount, *Item->GetName());
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
        UE_LOG(LogInventoryStorage, Error, TEXT("GetAllUniqueItems: GridManager is null!"));
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
    
    UE_LOG(LogInventoryStorage, Verbose, TEXT("GetAllUniqueItems: Found %d unique items"), 
           UniqueItems.Num());
    
    return UniqueItems;
}

void UInventoryStorage::RefreshUniqueItemsCache()
{
    CachedUniqueItems = GetAllUniqueItems();
    
    UE_LOG(LogInventoryStorage, Log, TEXT("RefreshUniqueItemsCache: Cached %d items"), 
           CachedUniqueItems.Num());
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
