#include "ShopInventoryGridWidget.h"
#include "Variant_Fishing/Data/ShopTransactionData.h"
#include "FishingCharacter.h"
#include "Variant_Fishing/Data/ItemDragOperation.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogShopInventory, Log, All);

void UShopInventoryGridWidget::SetTransactionManager(UShopTransactionManager* InManager)
{
	TransactionManager = InManager;
	UE_LOG(LogShopInventory, Log, TEXT("SetTransactionManager: Manager=%s"),
	       InManager ? TEXT("Valid") : TEXT("NULL"));
}

bool UShopInventoryGridWidget::IsPlayerInventory() const
{
	if (!InventoryComponent)
	{
		return false;
	}
	AActor* Owner = InventoryComponent->GetOwner();
	return Owner && Owner->IsA<AFishingCharacter>();
}

bool UShopInventoryGridWidget::IsShopInventory() const
{
	return !IsPlayerInventory();
}

bool UShopInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                            UDragDropOperation* InOperation)
{
	UE_LOG(LogShopInventory, Warning, TEXT("==== [ShopInventoryGridWidget::OnDrop] START ===="));

	auto* Op = Cast<UItemDragOperation>(InOperation);
	if (!Op || !Op->Item || !InventoryComponent)
	{
		UE_LOG(LogShopInventory, Warning, TEXT("OnDrop -> Invalid operation"));
		return false;
	}

	if (!TransactionManager)
	{
		UE_LOG(LogShopInventory, Error, TEXT("OnDrop -> TransactionManager is null!"));
		return false;
	}

	UItemBase* Item = Op->Item;
	UInventoryComponent* SourceInv = Op->SourceInventoryComponent;
	UInventoryComponent* TargetInv = this->InventoryComponent;

	if (!SourceInv)
	{
		UE_LOG(LogShopInventory, Error, TEXT("OnDrop -> SourceInventoryComponent is null!"));
		return false;
	}

	const bool bCanPlace = TargetInv->IsRoomAvailableAt(
		Item,
		DraggedItemTopLeftTile,
		(SourceInv == TargetInv) ? Item : nullptr
	);

	UE_LOG(LogShopInventory, Warning, TEXT("OnDrop -> SourceInv=%s, TargetInv=%s, CanPlace=%s, Tile=(%d,%d)"),
	       *SourceInv->GetOwner()->GetName(),
	       *TargetInv->GetOwner()->GetName(),
	       bCanPlace ? TEXT("TRUE") : TEXT("FALSE"),
	       DraggedItemTopLeftTile.X, DraggedItemTopLeftTile.Y);

	if (!bCanPlace)
	{
		UE_LOG(LogShopInventory, Warning, TEXT("OnDrop -> Cannot place item, cancelling drop"));
		ActiveOp = nullptr;
		DraggedItemRef = nullptr;
		return false;
	}

	if (SourceInv == TargetInv)
	{
		UE_LOG(LogShopInventory, Log, TEXT("OnDrop -> Same inventory move (no transaction)"));

		TargetInv->RemoveItem(Item);
		const int32 TargetIndex = TargetInv->TileToIndex(DraggedItemTopLeftTile);
		TargetInv->AddItemAt(Item, TargetIndex);
		TargetInv->RefreshAllItems();
		TargetInv->RefreshGridLayout();

		if (TransactionManager->IsPending(Item))
		{
			TransactionManager->RemovePendingItem(Item);
			UE_LOG(LogShopInventory, Log, TEXT("OnDrop -> Removed from pending (returned to source)"));
		}
	}

	else
	{
		UE_LOG(LogShopInventory, Log, TEXT("OnDrop -> Cross-inventory transfer (PENDING)"));

		const int32 TargetIndex = TargetInv->TileToIndex(DraggedItemTopLeftTile);

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
			Op->OriginalTopLeftTile,
			DraggedItemTopLeftTile,
			bIsSelling,
			Item->GetPrice()
		);

		TransactionManager->AddPendingItem(PendingItem);

		UE_LOG(LogShopInventory, Warning, TEXT("OnDrop -> Added to PENDING: %s, Type=%s, Price=%d"),
		       *Item->GetName(),
		       bIsSelling ? TEXT("SELL") : TEXT("BUY"),
		       Item->GetPrice());
	}

	ActiveOp = nullptr;
	DraggedItemRef = nullptr;

	UE_LOG(LogShopInventory, Warning, TEXT("==== [ShopInventoryGridWidget::OnDrop] END ===="));
	return true;
}
