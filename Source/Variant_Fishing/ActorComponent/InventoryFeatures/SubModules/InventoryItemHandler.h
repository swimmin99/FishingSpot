// ============================================
// InventoryItemHandler.h
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryItemHandler.generated.h"

class UInventoryGridManager;
class UInventoryStorage;
class UInventoryPlacementValidator;
class UItemBase;

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryHandler, Log, All);

/**
 * Handles item operations: add, remove, move, place
 */
UCLASS()
class FISHING_API UInventoryItemHandler : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UInventoryGridManager* InGridManager, 
                   UInventoryStorage* InStorage, 
                   UInventoryPlacementValidator* InValidator);
    
    bool TryAddItem(UItemBase* Item, int32& OutTopLeftIndex);
    bool AddItemAt(UItemBase* Item, int32 TopLeftIndex);
    bool RemoveItem(UItemBase* Item);
    bool MoveItem(UItemBase* Item, FIntPoint NewTopLeftTile);
    
    void PlaceItemInGrid(UItemBase* Item, int32 TopLeftIndex);
    void ClearItemFromGrid(UItemBase* Item);

private:
    UPROPERTY()
    UInventoryGridManager* GridManager;
    
    UPROPERTY()
    UInventoryStorage* Storage;
    
    UPROPERTY()
    UInventoryPlacementValidator* Validator;
};
