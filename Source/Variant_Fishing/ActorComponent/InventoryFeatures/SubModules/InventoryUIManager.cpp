// ============================================
// InventoryUIManager.cpp
// ============================================
#include "InventoryUIManager.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY(LogInventoryUI);

void UInventoryUIManager::RegisterWidget(UInventoryWidget* Widget)
{
    if (!Widget)
    {
        UE_LOG(LogInventoryUI, Warning, TEXT("RegisterWidget: Widget is null"));
        return;
    }
    
    InventoryWidget = Widget;
    
    UE_LOG(LogInventoryUI, Log, TEXT("RegisterWidget: Widget registered"));
    
    // Refresh immediately
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

void UInventoryUIManager::UpdateDescription(UItemBase* Item)
{
    if (!InventoryWidget)
    {
        UE_LOG(LogInventoryUI, Verbose, TEXT("UpdateDescription: No widget registered"));
        return;
    }
    
    if (!Item)
    {
        UE_LOG(LogInventoryUI, Warning, TEXT("UpdateDescription: Item is null"));
        return;
    }
    
    InventoryWidget->UpdateDescription(Item);
    
    UE_LOG(LogInventoryUI, Verbose, TEXT("UpdateDescription: Updated for %s"), *Item->GetName());
}

void UInventoryUIManager::ClearDescription()
{
    if (!InventoryWidget)
    {
        return;
    }
    
    InventoryWidget->ClearDescription();
    
    UE_LOG(LogInventoryUI, Verbose, TEXT("ClearDescription: Cleared"));
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