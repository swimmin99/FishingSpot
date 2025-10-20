#include "ShopInventoryGridWidget.h"
#include "Variant_Fishing/Data/ShopTransactionData.h"
#include "FishingCharacter.h"
#include "Variant_Fishing/Data/ItemDragOperation.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Data/ItemBase.h"

void UShopInventoryGridWidget::SetTransactionManager(UShopTransactionManager* InManager)
{
    TransactionManager = InManager;
    UE_LOG(LogShopInventory, Log, TEXT("SetTransactionManager: Manager=%s"),
           InManager ? TEXT("Valid") : TEXT("NULL"));
}

bool UShopInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                            UDragDropOperation* InOperation)
{
    auto* Op = Cast<UItemDragOperation>(InOperation);
    if (!Op || !Op->Item || !InventoryComponent)
    {
        UE_LOG(LogShopInventory, Warning, TEXT("ShopInventoryGridWidget : NativeOnDrop -> Invalid operation"));
        return false;
    }

    UItemBase* Item = Op->Item;
    UInventoryComponent* SourceInv = Op->SourceInventoryComponent;
    FIntPoint OriginalTile = Op->OriginalTopLeftTile;
    FIntPoint TargetTile = DraggedItemTopLeftTile;

    if (!SourceInv)
    {
        UE_LOG(LogShopInventory, Error, TEXT("ShopInventoryGridWidget : NativeOnDrop -> SourceInventoryComponent is null!"));
        return false;
    }

     UE_LOG(LogShopInventory, Warning, TEXT("==== [ShopInventoryGridWidget::ServerHandleDrop] START (SERVER ONLY) ===="));

    if (!Item || !SourceInv || !InventoryComponent)
    {
        UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop -> Invalid parameters"));
        return false;
    }

    if (!TransactionManager)
    {
        UE_LOG(LogShopInventory, Error, TEXT("ServerHandleDrop -> TransactionManager is null!"));
        return false;
    }

    UInventoryComponent* TargetInv = this->InventoryComponent;

    
    const bool bIsPending = TransactionManager->IsPending(Item);
    
    UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop : Item=%s, IsPending=%s, Source=%s, Target=%s"),
           *Item->DisplayName(),
           bIsPending ? TEXT("YES") : TEXT("NO"),
           *SourceInv->GetOwner()->GetName(),
           *TargetInv->GetOwner()->GetName());

    const bool bCanPlace = TargetInv->IsRoomAvailableAt(
        Item,
        TargetTile,
        (SourceInv == TargetInv) ? Item : nullptr
    );

    UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop -> CanPlace=%s, Tile=(%d,%d)"),
           bCanPlace ? TEXT("TRUE") : TEXT("FALSE"),
           TargetTile.X, TargetTile.Y);

    if (!bCanPlace)
    {
        UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop -> Cannot place item, cancelling drop"));
        return false;
    }

    
    
    
    if (bIsPending)
    {
        const TArray<FPendingTransactionItem>& PendingItems = TransactionManager->GetPendingItems();
        const FPendingTransactionItem* PendingInfo = nullptr;
        
        for (const FPendingTransactionItem& PItem : PendingItems)
        {
            if (PItem.Item == Item)
            {
                PendingInfo = &PItem;
                break;
            }
        }

        if (!PendingInfo)
        {
            UE_LOG(LogShopInventory, Error, TEXT("ServerHandleDrop : Pending item but no pending info found!"));
            return false;
        }

        UE_LOG(LogShopInventory, Log, TEXT("ServerHandleDrop : Pending Info - OriginalInventory=%s, TargetInventory=%s, CurrentInventory=%s"),
               *PendingInfo->SourceInventory->GetOwner()->GetName(),
               *PendingInfo->TargetInventory->GetOwner()->GetName(),
               *TargetInv->GetOwner()->GetName());

        
        if (TargetInv == PendingInfo->SourceInventory)
        {
            UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop : Returning to ORIGINAL inventory - CANCEL PENDING"));
            
            SourceInv->RemoveItem(Item);
            const int32 OriginalIndex = PendingInfo->SourceInventory->TileToIndex(PendingInfo->OriginalTile);
            TargetInv->AddItemAt(Item, OriginalIndex);
            TransactionManager->RemovePendingItem(Item);
            
            SourceInv->RefreshAllItems();
            TargetInv->RefreshAllItems();
            SourceInv->RefreshGridLayout();
            TargetInv->RefreshGridLayout();
            
            UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop : PENDING CANCELLED"));
        }
        
        else if (SourceInv == TargetInv && TargetInv == PendingInfo->TargetInventory)
        {
            UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop : Moving PENDING item within target inventory"));
            
            TargetInv->RemoveItem(Item);
            const int32 TargetIndex = TargetInv->TileToIndex(TargetTile);
            TargetInv->AddItemAt(Item, TargetIndex);
            TargetInv->RefreshAllItems();
            TargetInv->RefreshGridLayout();
            
            FPendingTransactionItem UpdatedPending = *PendingInfo;
            UpdatedPending.CurrentTile = TargetTile;
            
            TransactionManager->RemovePendingItem(Item);
            TransactionManager->AddPendingItem(UpdatedPending);
            
            UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop : PENDING position updated"));
        }
        
        else
        {
            UE_LOG(LogShopInventory, Error, TEXT("ServerHandleDrop : Cannot move PENDING item to different inventory!"));
            return false;
        }
    }
    
    
    
    else if (SourceInv == TargetInv)
    {
        UE_LOG(LogShopInventory, Log, TEXT("ServerHandleDrop : Same inventory move"));

        TargetInv->RemoveItem(Item);
        const int32 TargetIndex = TargetInv->TileToIndex(TargetTile);
        TargetInv->AddItemAt(Item, TargetIndex);
        TargetInv->RefreshAllItems();
        TargetInv->RefreshGridLayout();
    }
    else
    {
        UE_LOG(LogShopInventory, Log, TEXT("ServerHandleDrop : Cross-inventory transfer (PENDING)"));

        const int32 TargetIndex = TargetInv->TileToIndex(TargetTile);

        SourceInv->RemoveItem(Item);
        SourceInv->RefreshAllItems();
        SourceInv->RefreshGridLayout();

        TargetInv->AddItemAt(Item, TargetIndex);
        TargetInv->RefreshAllItems();
        TargetInv->RefreshGridLayout();

        bool bIsPlayerSource = (SourceInv->GetOwner()->IsA<AFishingCharacter>());
        bool bIsSelling = bIsPlayerSource;

        FPendingTransactionItem PendingItem(
            Item,
            SourceInv,
            TargetInv,
            OriginalTile,
            TargetTile,
            bIsSelling,
            Item->GetPrice()
        );

        TransactionManager->AddPendingItem(PendingItem);

        UE_LOG(LogShopInventory, Warning, TEXT("ServerHandleDrop : Added to PENDING: %s"), *Item->GetName());
    }

    UE_LOG(LogShopInventory, Warning, TEXT("==== [ShopInventoryGridWidget::ServerHandleDrop] END ===="));
    
    
    ActiveOp = nullptr;
    DraggedItemRef = nullptr;
    
    return true;
}

bool UShopInventoryGridWidget::IsPlayerInventory() const
{
    if (!InventoryComponent) return false;
    AActor* Owner = InventoryComponent->GetOwner();
    return Owner && Owner->IsA<AFishingCharacter>();
}

bool UShopInventoryGridWidget::IsShopInventory() const
{
    return !IsPlayerInventory();
}