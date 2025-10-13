
// ============================================
// InventoryUIManager.h
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryUIManager.generated.h"

class UInventoryWidget;
class UItemBase;

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryUI, Log, All);

/**
 * Manages UI widgets and visual updates
 */
UCLASS()
class FISHING_API UInventoryUIManager : public UObject
{
	GENERATED_BODY()

public:
	void RegisterWidget(UInventoryWidget* Widget);
	void UnregisterWidget();
    
	void RefreshGrid();
	void UpdateDescription(UItemBase* Item);
	void ClearDescription();
	void SetFocusGridWidget();
    
	bool HasWidget() const { return InventoryWidget != nullptr; }
	UInventoryWidget* GetWidget() const { return InventoryWidget; }

private:
	UPROPERTY()
	UInventoryWidget* InventoryWidget;
};