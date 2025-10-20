
#include "InventoryStorage.h"
#include "Fishing.h"
#include "InventoryGridManager.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Engine/AssetManager.h"

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




TArray<FItemSyncData> UInventoryStorage::GenerateSyncData() const
{
    TArray<FItemSyncData> SyncData;
    if (!GridManager) { return SyncData; }

    
    for (const auto& Pair : CachedUniqueItems) 
        {
        UItemBase* Item = Pair.Key;
        const FIntPoint TopLeftTile = Pair.Value;
        const int32 TopLeftIndex = GridManager->TileToIndex(TopLeftTile);
        if (!Item || !GridManager->IsIndexValid(TopLeftIndex)) { continue; }

        
        FItemSyncData Data(Item, TopLeftIndex);
        if (Data.IsValid())
        {
            SyncData.Add(Data);
        }
        else
        {
            UE_LOG(LogInventoryStorage, Warning, TEXT("GenerateSyncData: Invalid sync for %s"), *Item->DisplayName());
        }
        }

    UE_LOG(LogInventoryStorage, Log, TEXT("GenerateSyncData: %d entries"), SyncData.Num());
    return SyncData;
}



void UInventoryStorage::ApplySyncData(const TArray<FItemSyncData>& SyncData)
{
    if (!GridManager || !Outer)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("ApplySyncData: Dependencies not initialized!"));
        return;
    }
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ApplySyncData: Applying %d sync entries"), 
           SyncData.Num());
    
    
    ClearAll();
    
    
    for (const FItemSyncData& Data : SyncData)
    {
        if (!Data.IsValid())
        {
            UE_LOG(LogInventoryStorage, Warning, TEXT("ApplySyncData: Invalid sync data"));
            continue;
        }
        
        
        CreateItemFromSyncData(Data);
    }
    
    
    RefreshUniqueItemsCache();
    
    UE_LOG(LogInventoryStorage, Log, TEXT("ApplySyncData: Complete. %d unique items placed"),
           CachedUniqueItems.Num());
}




void UInventoryStorage::CreateItemFromSyncData(const FItemSyncData& SyncData)
{
    if (!Outer)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("CreateItemFromSyncData: Outer is null!"));
        return;
    }

    
    UObject* LoadedAsset = SyncData.DataAssetPath.TryLoad();
    
    if (!LoadedAsset)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("CreateItemFromSyncData: Failed to load asset %s"),
               *SyncData.DataAssetPath.ToString());
        return;
    }

    
    if (!LoadedAsset->GetClass()->ImplementsInterface(UItemDataProvider::StaticClass()))
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("CreateItemFromSyncData: Asset %s does not implement IItemDataProvider"),
               *SyncData.DataAssetPath.ToString());
        return;
    }

    
    UItemBase* NewItem = NewObject<UItemBase>(Outer, UItemBase::StaticClass());
    if (!NewItem)
    {
        UE_LOG(LogInventoryStorage, Error, TEXT("CreateItemFromSyncData: Failed to create ItemBase"));
        return;
    }
    
    
    NewItem->ItemGuid = SyncData.ItemGuid;
    NewItem->ItemDataProvider = LoadedAsset;
    NewItem->bIsRotated = SyncData.bIsRotated;
    NewItem->SpecificData = SyncData.SpecificData;
    
    
    PlaceItemInGrid(NewItem, SyncData.TopLeftIndex);
    
    UE_LOG(LogInventoryStorage, Log, TEXT("✅ CreateItemFromSyncData: Created and placed %s at index %d"),
           *NewItem->DisplayName(), SyncData.TopLeftIndex);
}

void UInventoryStorage::PlaceItemInGrid(UItemBase* Item, int32 TopLeftIndex)
{
    if (!Item || !GridManager)
    {
        return;
    }
    
    const FIntPoint CurrentDims = Item->GetCurrentDimensions();
    const FIntPoint TopLeft = GridManager->IndexToTile(TopLeftIndex);
    
    
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