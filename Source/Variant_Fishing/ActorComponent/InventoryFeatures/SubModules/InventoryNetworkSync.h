// ============================================
// InventoryNetworkSync.h
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryNetworkSync.generated.h"

class UInventoryComponent;
class UInventoryStorage;
class UInventoryGridManager;
class UItemBase;

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryNetwork, Log, All);

/**
 * Handles network synchronization data preparation
 */
UCLASS()
class FISHING_API UInventoryNetworkSync : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UInventoryComponent* InOwnerComponent,
				   UInventoryStorage* InStorage,
				   UInventoryGridManager* InGridManager);
    
	// Find item's current top-left index in storage
	bool FindItemTopLeftIndex(UItemBase* Item, int32& OutIndex) const;
    
	// Helper to find any index where item exists
	bool FindItemIndex(UItemBase* Item, int32& OutIndex) const;

private:
	UPROPERTY()
	UInventoryComponent* OwnerComponent;
    
	UPROPERTY()
	UInventoryStorage* Storage;
    
	UPROPERTY()
	UInventoryGridManager* GridManager;
};
