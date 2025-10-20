#pragma once

#include "CoreMinimal.h"
#include "Variant_Fishing/Widget/Inventory/InventoryGridWidget.h"
#include "Fishing.h"
#include "ShopInventoryGridWidget.generated.h"

class UShopTransactionManager;

UCLASS()
class FISHING_API UShopInventoryGridWidget : public UInventoryGridWidget
{
	GENERATED_BODY()

public:
	void SetTransactionManager(UShopTransactionManager* InManager);

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
							  UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	UShopTransactionManager* TransactionManager = nullptr;
	

	bool IsPlayerInventory() const;
	bool IsShopInventory() const;
};