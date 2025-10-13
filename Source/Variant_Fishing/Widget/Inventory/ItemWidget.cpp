#include "ItemWidget.h"

#include "InventoryGridWidget.h"
#include "Variant_Fishing/Data/ItemDragOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "././../Fishing.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"


void UItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogInventory, Verbose, TEXT("NativeConstruct: UItemWidget constructed"));
}

void UItemWidget::SetItemWidget(UItemBase* ItemToAdd, UInventoryComponent* InOwnerInventoryComponent)
{
	if (!ItemToAdd || !InOwnerInventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("SetItemWidget: nullptr param (ItemToAdd=%p, OwnerInv=%p)"),
		       ItemToAdd, InOwnerInventoryComponent);
		return;
	}
	if (!ItemImage)
	{
		UE_LOG(LogInventory, Warning, TEXT("SetItemWidget: ItemImage is nullptr"));
		return;
	}

	OwnerInventoryComponent = InOwnerInventoryComponent;
	Item = ItemToAdd;

	if (Item->GetIcon())
	{
		UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(Item->GetIcon(), this);
		if (DynMaterial)
		{
			ItemImage->SetBrushFromMaterial(DynMaterial);
			UE_LOG(LogInventory, Log, TEXT("SetItemWidget: Dynamic material created for %s"), *Item->GetName());
		}
		else
		{
			ItemImage->SetBrushFromMaterial(Item->GetIcon());
			UE_LOG(LogInventory, Warning, TEXT("SetItemWidget: Failed to create DynMaterial, fallback to original"));
		}
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("SetItemWidget: Item has no icon material (%s)"), *Item->GetName());
	}

	UpdateVisual();
}

void UItemWidget::UpdateVisual()
{
	if (!Item)
	{
		UE_LOG(LogInventory, Warning, TEXT("UpdateVisual: Item is nullptr"));
		return;
	}
	if (!OwnerInventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("UpdateVisual: OwnerInventoryComponent is nullptr (Item=%s)"),
		       *Item->GetName());
		return;
	}
	if (!ItemImage)
	{
		UE_LOG(LogInventory, Warning, TEXT("UpdateVisual: ItemImage is nullptr (Item=%s)"),
		       *Item->GetName());
		return;
	}

	const float TileSize = OwnerInventoryComponent->TileSize;
	const FIntPoint Dims = Item->GetCurrentDimensions();
	Size = FVector2D(Dims.X * TileSize, Dims.Y * TileSize);

	if (BackgroundSizeBox)
	{
		BackgroundSizeBox->SetWidthOverride(Size.X);
		BackgroundSizeBox->SetHeightOverride(Size.Y);
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("UpdateVisual: BackgroundSizeBox is nullptr (non-fatal)"));
	}

	if (UCanvasPanelSlot* MySlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemImage))
	{
		MySlot->SetSize(Size);
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("UpdateVisual: ItemImage is not in a CanvasPanelSlot (non-fatal)"));
	}

	if (UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(ItemImage->GetBrush().GetResourceObject()))
	{
		const float RotParam = Item->GetIsRotated() ? 0.25f : 0.0f;
		DynMat->SetScalarParameterValue(FName("IconRotation"), RotParam);
		UE_LOG(LogInventory, Log, TEXT("UpdateVisual: IconRotation=%.2f (Rotated=%s, Size=%.0fx%.0f) for %s"),
		       RotParam, Item->GetIsRotated() ? TEXT("Yes") : TEXT("No"), Size.X, Size.Y, *Item->GetName());
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("UpdateVisual: No DynMaterial bound to ItemImage (Item=%s)"),
		       *Item->GetName());
	}
}

FReply UItemWidget::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& InMouseEvent)
{
	if (!OwnerInventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnMouseButtonDown: OwnerInventoryComponent is nullptr"));
		return FReply::Handled();
	}
	if (!Item)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnMouseButtonDown: Item is nullptr"));
		return FReply::Handled();
	}

	if (OwnerInventoryComponent)
	{
		OwnerInventoryComponent->SetFocusGridWidget();
		UE_LOG(LogInventory, Verbose, TEXT("OnMouseButtonDown: Focus set to InventoryGridWidget"));
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("OnMouseButtonDown: InventoryGridWidget is nullptr (non-fatal)"));
	}

	OwnerInventoryComponent->UpdateDescription(Item);
	UE_LOG(LogInventory, Log, TEXT("OnMouseButtonDown: UpdateDescription called for %s"), *Item->GetName());

	return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
}

void UItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (BackgroundBorder)
	{
		BackgroundBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.2f));
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("OnMouseEnter: BackgroundBorder is nullptr (non-fatal)"));
	}
}

void UItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (BackgroundBorder)
	{
		BackgroundBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("OnMouseLeave: BackgroundBorder is nullptr (non-fatal)"));
	}
}

void UItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                       UDragDropOperation*& OutOp)
{
	if (!Item)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDragDetected: Item is nullptr"));
		return;
	}
	if (!OwnerInventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDragDetected: OwnerInventoryComponent is nullptr (Item=%s)"),
		       *Item->GetName());
		return;
	}

	OwnerInventoryComponent->ClearDescription();
	UE_LOG(LogInventory, Verbose, TEXT("OnDragDetected: Cleared description"));

	UItemDragOperation* Op = NewObject<UItemDragOperation>();
	if (!Op)
	{
		UE_LOG(LogInventory, Error, TEXT("OnDragDetected: Failed to create UItemDragOperation"));
		return;
	}

	Op->Item = Item;
	Op->SourceInventoryComponent = OwnerInventoryComponent;

	TMap<UItemBase*, FIntPoint> AllItems = OwnerInventoryComponent->GetAllItems();
	if (AllItems.Contains(Item))
	{
		Op->OriginalTopLeftTile = AllItems[Item];
		UE_LOG(LogInventory, Log, TEXT("OnDragDetected: Stored origin (%d,%d) for %s"),
		       Op->OriginalTopLeftTile.X, Op->OriginalTopLeftTile.Y, *Item->GetName());
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("OnDragDetected: No origin found in grid (non-fatal)"));
	}

	UItemWidget* Ghost = CreateWidget<UItemWidget>(GetWorld(), GetClass());
	if (!Ghost)
	{
		UE_LOG(LogInventory, Error, TEXT("OnDragDetected: Failed to create Ghost ItemWidget"));
		return;
	}

	Ghost->SetOwningPlayer(GetOwningPlayer());
	Ghost->SetItemWidget(Item, OwnerInventoryComponent);
	Ghost->SetVisibility(ESlateVisibility::HitTestInvisible);

	Op->GhostItemWidget = Ghost;
	Op->DefaultDragVisual = Ghost;
	Op->Pivot = EDragPivot::CenterCenter;
	Op->Payload = Item;

	OutOp = Op;

	UE_LOG(LogInventory, Log, TEXT("OnDragDetected: DragOp created (Owner=%s, Item=%s, Rotated=%s)"),
	       OwnerInventoryComponent && OwnerInventoryComponent->GetOwner()
	       ? *OwnerInventoryComponent->GetOwner()->GetName()
	       : TEXT("None"),
	       *Item->GetName(),
	       Item->GetIsRotated() ? TEXT("Yes") : TEXT("No"));
}

void UItemWidget::NativeOnDragCancelled(const FDragDropEvent& DragEvent, UDragDropOperation* InOp)
{
	UItemDragOperation* Op = Cast<UItemDragOperation>(InOp);
	if (!Op)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDragCancelled: DragOperation cast failed (nullptr)"));
		return;
	}
	if (!Op->SourceInventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDragCancelled: SourceInventoryComponent is nullptr"));
		return;
	}

	UE_LOG(LogInventory, Warning, TEXT("OnDragCancelled: Restoring layout for source inventory"));
	Op->SourceInventoryComponent->RefreshGridLayout();
}
