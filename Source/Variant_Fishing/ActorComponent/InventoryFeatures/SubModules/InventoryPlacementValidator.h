


#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryPlacementValidator.generated.h"

class UInventoryGridManager;
class UInventoryStorage;
class UItemBase;


UCLASS()
class FISHING_API UInventoryPlacementValidator : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UInventoryGridManager* InGridManager, UInventoryStorage* InStorage);
    
	bool CanPlaceItemAt(UItemBase* Item, FIntPoint TopLeftTile, UItemBase* IgnoreItem = nullptr) const;
	bool CanPlaceItemAt(UItemBase* Item, int32 TopLeftIndex, UItemBase* IgnoreItem = nullptr) const;
	bool FindFirstAvailableSlot(UItemBase* Item, int32& OutTopLeftIndex) const;
    
private:
	bool CheckTileOccupancy(FIntPoint Tile, UItemBase* IgnoreItem) const;
	bool CheckBoundsForItem(UItemBase* Item, FIntPoint TopLeftTile) const;
    
	UPROPERTY()
	UInventoryGridManager* GridManager;
    
	UPROPERTY()
	UInventoryStorage* Storage;
};