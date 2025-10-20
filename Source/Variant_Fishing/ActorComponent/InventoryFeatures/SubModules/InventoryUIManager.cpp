


#include "InventoryUIManager.h"

#include "Fishing.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "Variant_Fishing/Data/ItemBase.h"


void UInventoryUIManager::RegisterWidget(UInventoryWidget* Widget)
{
    if (!Widget)
    {
        UE_LOG(LogInventoryUI, Warning, TEXT("RegisterWidget: Widget is null"));
        return;
    }
    
    InventoryWidget = Widget;
    
    UE_LOG(LogInventoryUI, Log, TEXT("RegisterWidget: Widget registered"));
    
    
    RefreshGrid();
}

void UInventoryUIManager::UnregisterWidget()
{
    if (InventoryWidget)
    {
        UE_LOG(LogInventoryUI, Log, TEXT("UnregisterWidget: Widget unregistered"));
    }
    
    InventoryWidget = nullptr;
}

void UInventoryUIManager::RefreshGrid()
{
    if (!InventoryWidget)
    {
        UE_LOG(LogInventoryUI, Verbose, TEXT("RefreshGrid: No widget registered"));
        return;
    }
    
    InventoryWidget->RefreshGrid();
    
    UE_LOG(LogInventoryUI, Verbose, TEXT("RefreshGrid: Grid refreshed"));
}


void UInventoryUIManager::SetFocusGridWidget()
{
    if (!InventoryWidget)
    {
        return;
    }
    
    InventoryWidget->SetFocusGridWidget();
    
    UE_LOG(LogInventoryUI, Verbose, TEXT("SetFocusGridWidget: Focus set"));
}
