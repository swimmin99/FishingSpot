// ============================================
// InventoryPlacementValidator.cpp
// ============================================
#include "InventoryPlacementValidator.h"

#include "Fishing.h"
#include "InventoryGridManager.h"
#include "InventoryStorage.h"
#include "Variant_Fishing/Data/ItemBase.h"

void UInventoryPlacementValidator::Initialize(UInventoryGridManager* InGridManager, 
                                              UInventoryStorage* InStorage)
{
    if (!InGridManager || !InStorage)
    {
        UE_LOG(LogInventoryValidator, Error, TEXT("Initialize: Null dependencies!"));
        return;
    }
    
    GridManager = InGridManager;
    Storage = InStorage;
    
    UE_LOG(LogInventoryValidator, Log, TEXT("Initialize: Validator ready"));
}

bool UInventoryPlacementValidator::CanPlaceItemAt(UItemBase* Item, FIntPoint TopLeftTile, 
                                                  UItemBase* IgnoreItem) const
{
    if (!Item)
    {
        UE_LOG(LogInventoryValidator, Warning, TEXT("CanPlaceItemAt: Item is null"));
        return false;
    }
    
    if (!GridManager || !Storage)
    {
        UE_LOG(LogInventoryValidator, Error, TEXT("CanPlaceItemAt: Dependencies not initialized!"));
        return false;
    }
    
    // Check bounds
    if (!CheckBoundsForItem(Item, TopLeftTile))
    {
        UE_LOG(LogInventoryValidator, Verbose, TEXT("CanPlaceItemAt: Out of bounds at %s"),
               *GridManager->TileToString(TopLeftTile));
        return false;
    }
    
    // Check each tile the item would occupy
    const FIntPoint CurrentDims = Item->GetCurrentDimensions();
    
    for (int32 y = 0; y < CurrentDims.Y; ++y)
    {
        for (int32 x = 0; x < CurrentDims.X; ++x)
        {
            const FIntPoint CheckTile(TopLeftTile.X + x, TopLeftTile.Y + y);
            
            if (!CheckTileOccupancy(CheckTile, IgnoreItem))
            {
                UE_LOG(LogInventoryValidator, Verbose, TEXT("CanPlaceItemAt: Tile occupied at %s"),
                       *GridManager->TileToString(CheckTile));
                return false;
            }
        }
    }
    
    return true;
}


bool UInventoryPlacementValidator::CanPlaceItemAt(UItemBase* Item, int32 TopLeftIndex, 
                                                  UItemBase* IgnoreItem) const
{
    if (!GridManager)
    {
        return false;
    }
    
    if (!GridManager->IsIndexValid(TopLeftIndex))
    {
        UE_LOG(LogInventoryValidator, Verbose, TEXT("CanPlaceItemAt: Invalid index %d"), TopLeftIndex);
        return false;
    }
    
    const FIntPoint TopLeftTile = GridManager->IndexToTile(TopLeftIndex);
    return CanPlaceItemAt(Item, TopLeftTile, IgnoreItem);
}

bool UInventoryPlacementValidator::FindFirstAvailableSlot(UItemBase* Item, int32& OutTopLeftIndex) const
{
    if (!Item || !GridManager)
    {
        return false;
    }
    
    const int32 TotalTiles = GridManager->GetTotalTiles();
    
    for (int32 i = 0; i < TotalTiles; ++i)
    {
        if (CanPlaceItemAt(Item, i, nullptr))
        {
            OutTopLeftIndex = i;
            UE_LOG(LogInventoryValidator, Log, TEXT("FindFirstAvailableSlot: Found slot at index %d (%s)"),
                   i, *GridManager->TileToString(GridManager->IndexToTile(i)));
            return true;
        }
    }
    
    UE_LOG(LogInventoryValidator, Warning, TEXT("FindFirstAvailableSlot: No available slot for item %s"),
           *Item->GetName());
    return false;
}

