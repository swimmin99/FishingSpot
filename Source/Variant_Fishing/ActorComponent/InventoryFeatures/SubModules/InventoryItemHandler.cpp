#include "InventoryItemHandler.h"



// ============================================
// InventoryItemHandler.cpp
// ============================================
#include "InventoryItemHandler.h"
#include "InventoryGridManager.h"
#include "InventoryStorage.h"
#include "InventoryPlacementValidator.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY(LogInventoryHandler);

void UInventoryItemHandler::Initialize(UInventoryGridManager* InGridManager,
                                       UInventoryStorage* InStorage,
                                       UInventoryPlacementValidator* InValidator)
{
    if (!InGridManager || !InStorage || !InValidator)
    {
        UE_LOG(LogInventoryHandler, Error, TEXT("Initialize: Null dependencies!"));
        return;
    }
    
    GridManager = InGridManager;
    Storage = InStorage;
    Validator = InValidator;
    
    UE_LOG(LogInventoryHandler, Log, TEXT("Initialize: ItemHandler ready"));
}

bool UInventoryItemHandler::TryAddItem(UItemBase* Item, int32& OutTopLeftIndex)
{
    if (!Item)
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("TryAddItem: Item is null"));
        return false;
    }
    
    if (!GridManager || !Storage || !Validator)
    {
        UE_LOG(LogInventoryHandler, Error, TEXT("TryAddItem: Dependencies not initialized!"));
        return false;
    }
    
    const FIntPoint CurrentDims = Item->GetCurrentDimensions();
    UE_LOG(LogInventoryHandler, Log, TEXT("TryAddItem: Item=%s, Dims=%s, IsRotated=%s"),
           *Item->GetName(),
           *GridManager->TileToString(CurrentDims),
           Item->GetIsRotated() ? TEXT("Yes") : TEXT("No"));
    
    // Find first available slot
    if (!Validator->FindFirstAvailableSlot(Item, OutTopLeftIndex))
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("TryAddItem: No room for Item=%s"), *Item->GetName());
        return false;
    }
    
    // Place the item
    PlaceItemInGrid(Item, OutTopLeftIndex);
    
    UE_LOG(LogInventoryHandler, Log, TEXT("TryAddItem: Successfully added at index %d (%s)"),
           OutTopLeftIndex, *GridManager->TileToString(GridManager->IndexToTile(OutTopLeftIndex)));
    
    return true;
}

bool UInventoryItemHandler::AddItemAt(UItemBase* Item, int32 TopLeftIndex)
{
    if (!Item)
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("AddItemAt: Item is null"));
        return false;
    }
    
    if (!GridManager || !Storage || !Validator)
    {
        UE_LOG(LogInventoryHandler, Error, TEXT("AddItemAt: Dependencies not initialized!"));
        return false;
    }
    
    if (!GridManager->IsIndexValid(TopLeftIndex))
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("AddItemAt: Invalid TopLeftIndex=%d"), TopLeftIndex);
        return false;
    }
    
    const FIntPoint TopLeftTile = GridManager->IndexToTile(TopLeftIndex);
    
    // Validate placement
    if (!Validator->CanPlaceItemAt(Item, TopLeftTile, nullptr))
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("AddItemAt: Cannot place at index %d"), TopLeftIndex);
        return false;
    }
    
    // Place the item
    PlaceItemInGrid(Item, TopLeftIndex);
    
    UE_LOG(LogInventoryHandler, Log, TEXT("AddItemAt: Successfully added at index %d"), TopLeftIndex);
    
    return true;
}

bool UInventoryItemHandler::RemoveItem(UItemBase* Item)
{
    if (!Item)
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("RemoveItem: Item is null"));
        return false;
    }
    
    if (!Storage)
    {
        UE_LOG(LogInventoryHandler, Error, TEXT("RemoveItem: Storage not initialized!"));
        return false;
    }
    
    // Check if item exists
    bool bFound = false;
    const TArray<UItemBase*>& Items = Storage->GetItemsArray();
    
    for (int32 i = 0; i < Items.Num(); i++)
    {
        if (Items[i] == Item)
        {
            bFound = true;
            break;
        }
    }
    
    if (!bFound)
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("RemoveItem: Item %s not found"), *Item->GetName());
        return false;
    }
    
    // Clear the item
    ClearItemFromGrid(Item);
    
    UE_LOG(LogInventoryHandler, Log, TEXT("RemoveItem: Removed %s"), *Item->GetName());
    
    return true;
}

bool UInventoryItemHandler::MoveItem(UItemBase* Item, FIntPoint NewTopLeftTile)
{
    if (!Item || !GridManager || !Storage)
    {
        UE_LOG(LogInventoryHandler, Warning, TEXT("MoveItem: Invalid parameters or dependencies"));
        return false;
    }
    
    UE_LOG(LogInventoryHandler, Log, TEXT("MoveItem: Moving %s to %s"),
           *Item->GetName(), *GridManager->TileToString(NewTopLeftTile));
    
    // Clear from old position
    ClearItemFromGrid(Item);
    
    // Place at new position
    const int32 NewIndex = GridManager->TileToIndex(NewTopLeftTile);
    PlaceItemInGrid(Item, NewIndex);
    
    UE_LOG(LogInventoryHandler, Log, TEXT("MoveItem: Successfully moved to index %d"), NewIndex);
    
    return true;
}

void UInventoryItemHandler::PlaceItemInGrid(UItemBase* Item, int32 TopLeftIndex)
{
    if (!Item || !GridManager || !Storage)
    {
        UE_LOG(LogInventoryHandler, Error, TEXT("PlaceItemInGrid: Invalid parameters or dependencies"));
        return;
    }
    
    const FIntPoint CurrentDims = Item->GetCurrentDimensions();
    const FIntPoint TopLeft = GridManager->IndexToTile(TopLeftIndex);
    
    UE_LOG(LogInventoryHandler, Verbose, TEXT("PlaceItemInGrid: Item=%s at %s, Dims=%s"),
           *Item->GetName(),
           *GridManager->TileToString(TopLeft),
           *GridManager->TileToString(CurrentDims));
    
    // Fill all tiles occupied by this item
    for (int32 y = 0; y < CurrentDims.Y; ++y)
    {
        for (int32 x = 0; x < CurrentDims.X; ++x)
        {
            const FIntPoint Tile(TopLeft.X + x, TopLeft.Y + y);
            const int32 Index = GridManager->TileToIndex(Tile);
            
            if (GridManager->IsIndexValid(Index))
            {
                Storage->SetItemAtIndex(Index, Item);
            }
            else
            {
                UE_LOG(LogInventoryHandler, Warning, TEXT("PlaceItemInGrid: Skip invalid index %d"), Index);
            }
        }
    }
}

void UInventoryItemHandler::ClearItemFromGrid(UItemBase* Item)
{
    if (!Item || !Storage)
    {
        return;
    }
    
    Storage->ClearAllOccurrences(Item);
}