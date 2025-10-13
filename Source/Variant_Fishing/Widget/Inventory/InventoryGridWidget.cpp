#include "Variant_Fishing/Widget/Inventory/InventoryGridWidget.h"

#include "Variant_Fishing/Data/ItemDragOperation.h"
#include "ItemWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Styling/SlateBrush.h"
#include "Fishing.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"


void UInventoryGridWidget::InitializeWidget(UInventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	if (!InventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("InventoryGridWidget: No InventoryComponent"));
		return;
	}

	ConstructBaseBackground();
}

void UInventoryGridWidget::ConstructBaseBackground()
{
	Colums = InventoryComponent->Columns;
	Rows = InventoryComponent->Rows;
	TileSize = InventoryComponent->TileSize;

	float NewWidth = Colums * TileSize;
	float NewHeight = Rows * TileSize;

	StartX.Empty();
	StartY.Empty();
	EndX.Empty();
	EndY.Empty();

	UCanvasPanelSlot* BorderAsCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridBorder);
	if (BorderAsCanvasSlot)
	{
		BorderAsCanvasSlot->SetSize(FVector2d(NewWidth, NewHeight));
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("InventoryGridWidget: GridBorder Slot not found"));
	}

	UE_LOG(LogInventory, Log,
	       TEXT("InventoryGridWidget::NativeConstruct Columns=%d Rows=%d TileSize=%.1f Size=(%.1f,%.1f)"),
	       Colums, Rows, TileSize, NewWidth, NewHeight);

	CreateLineSegments();
	SetIsFocusable(true);
}

bool UInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& E,
                                            UDragDropOperation* InOp)
{
	auto* Op = Cast<UItemDragOperation>(InOp);

	if (!Op)
	{
		return false;
	}
	if (!Op->Item)
	{
		return false;
	}
	if (!InventoryComponent)
	{
		return false;
	}
	if (!GridBorder)
	{
		return false;
	}

	ActiveOp = Op;
	DraggedItemRef = Op->Item;

	const FGeometry& G = GridBorder->GetCachedGeometry();
	const FVector2D LocalPos = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	const float TileSz = InventoryComponent->TileSize;

	const FIntPoint CurrentDims = Op->Item->GetCurrentDimensions();

	const FIntPoint MouseTile(
		FMath::FloorToInt(LocalPos.X / TileSz),
		FMath::FloorToInt(LocalPos.Y / TileSz)
	);

	const FIntPoint HalfDims(CurrentDims.X / 2, CurrentDims.Y / 2);
	FIntPoint TopLeft = MouseTile - HalfDims;

	TopLeft.X = FMath::Clamp(TopLeft.X, 0, InventoryComponent->Columns - CurrentDims.X);
	TopLeft.Y = FMath::Clamp(TopLeft.Y, 0, InventoryComponent->Rows - CurrentDims.Y);

	DraggedItemTopLeftTile = TopLeft;

	return true;
}

FMousePositionInTile UInventoryGridWidget::MousePositionInTileResult(FVector2D MousePosition)
{
	if (!InventoryComponent)
	{
		return MousePositionInTile;
	}

	MousePositionInTile.Right = fmod(MousePosition.X, InventoryComponent->TileSize) > (InventoryComponent->TileSize /
		2);
	MousePositionInTile.Down = fmod(MousePosition.Y, InventoryComponent->TileSize) > (InventoryComponent->TileSize / 2);

	return MousePositionInTile;
}

void UInventoryGridWidget::CreateLineSegments()
{
	LineStructData.XLines.Empty();
	LineStructData.YLines.Empty();

	for (int32 i = 0; i <= Colums; i++)
	{
		float X = i * TileSize;
		LineStructData.XLines.Add(FVector2D(X, X));
		LineStructData.YLines.Add(FVector2D(0.0f, Rows * TileSize));
	}

	for (int32 i = 0; i <= Rows; i++)
	{
		float Y = i * TileSize;
		LineStructData.YLines.Add(FVector2D(Y, Y));
		LineStructData.XLines.Add(FVector2D(0.0f, Colums * TileSize));
	}

	for (const FVector2D& Elements : LineStructData.XLines)
	{
		StartX.Add(Elements.X);
		EndX.Add(Elements.Y);
	}
	for (const FVector2D& Elements : LineStructData.YLines)
	{
		StartY.Add(Elements.X);
		EndY.Add(Elements.Y);
	}

	UE_LOG(LogInventory, Log, TEXT("CreateLineSegments: Colums=%d Rows=%d LinesX=%d LinesY=%d"),
	       Colums, Rows, LineStructData.XLines.Num(), LineStructData.YLines.Num());
}

int32 UInventoryGridWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!GridBorder)
	{
		return LayerId;
	}

	FPaintContext PaintContext(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                           bParentEnabled);
	FLinearColor CustomColor(0.5f, 0.5f, 0.5f, 0.5f);
	FVector2D TopLeftCorner = GridBorder->GetCachedGeometry().GetLocalPositionAtCoordinates(FVector2D(0.0f, 0.0f));

	for (int32 i = 0; i < LineStructData.XLines.Num() && i < StartX.Num(); i++)
	{
		int32 k = FMath::Min(i, StartY.Num() - 1);
		UWidgetBlueprintLibrary::DrawLine(PaintContext,
		                                  FVector2D(StartX[i], StartY[k]) + TopLeftCorner,
		                                  FVector2D(EndX[i], EndY[k]) + TopLeftCorner,
		                                  CustomColor);

#if !(UE_BUILD_SHIPPING)
		if (i < 5)
		{
			UE_LOG(LogInventory, VeryVerbose, TEXT("NativePaint: DrawLine #%d from (%.1f,%.1f) to (%.1f,%.1f)"),
			       i, StartX[i], StartY[k], EndX[i], EndY[k]);
		}
#endif
	}

	return LayerId + 1;
}

void UInventoryGridWidget::Clean()
{
	for (int i = 0; i < CreatedItemWidget.Num(); ++i)
	{
		if (CreatedItemWidget[i])
		{
			CreatedItemWidget[i]->RemoveFromParent();
		}
	}
	CreatedItemWidget.Empty();
}

void UInventoryGridWidget::Refresh()
{
	if (!InventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("Refresh: InventoryComponent null"));
		return;
	}

	Clean();

	TArray<UItemBase*> Keys;
	TMap<UItemBase*, FIntPoint> Map = InventoryComponent->GetAllItems();
	Map.GetKeys(Keys);

	UE_LOG(LogInventory, Log, TEXT("Refresh: %d items in inventory"), Keys.Num());

	if (!InventoryComponent->ItemWidgetClass)
	{
		UE_LOG(LogInventory, Warning, TEXT("Refresh: No ItemWidgetClass"));
		return;
	}

	for (UItemBase* AddedItem : Keys)
	{
		if (!AddedItem)
		{
			UE_LOG(LogInventory, Warning, TEXT("Refresh: Null item in inventory"));
			continue;
		}

		UUserWidget* ItemWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryComponent->ItemWidgetClass);
		if (!ItemWidget)
		{
			UE_LOG(LogInventory, Warning, TEXT("Refresh: Failed to create ItemWidget for %s"), *AddedItem->GetName());
			continue;
		}

		CreatedItemWidget.Add(ItemWidget);
		ItemWidget->SetOwningPlayer(GetOwningPlayer());

		UItemWidget* IW = Cast<UItemWidget>(ItemWidget);
		if (!IW)
		{
			UE_LOG(LogInventory, Warning, TEXT("Refresh: ItemWidget is not of type UItemWidget for %s"),
			       *AddedItem->GetName());
			continue;
		}

		IW->SetItemWidget(AddedItem, InventoryComponent);

		const FIntPoint Tile = Map[AddedItem];
		const float X = Tile.X * InventoryComponent->TileSize;
		const float Y = Tile.Y * InventoryComponent->TileSize;

		if (!GridCanvasPanel)
		{
			UE_LOG(LogInventory, Warning, TEXT("Refresh: GridCanvasPanel is null"));
			continue;
		}

		UPanelSlot* MySlot = GridCanvasPanel->AddChild(ItemWidget);
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MySlot);
		if (CanvasSlot)
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(X, Y));

			UE_LOG(LogInventory, Log, TEXT("Refresh: Placed %s at (%.1f, %.1f), Rotated=%s"),
			       *AddedItem->GetName(), X, Y,
			       AddedItem->GetIsRotated() ? TEXT("Yes") : TEXT("No"));
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("Refresh: CanvasSlot invalid for %s"), *AddedItem->GetName());
		}
	}

	this->InvalidateLayoutAndVolatility();
	this->ForceLayoutPrepass();
}

void UInventoryGridWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                             UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	auto* Op = Cast<UItemDragOperation>(InOperation);
	if (!Op || !Op->Item)
	{
		UE_LOG(LogInventory, Warning, TEXT("NativeOnDragEnter: Invalid operation"));
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("NativeOnDragEnter: Item=%s entered grid"), *Op->Item->GetName());

	if (!GridBorder || !InventoryComponent)
	{
		return;
	}

	const FGeometry& G = GridBorder->GetCachedGeometry();
	const FVector2D LocalPos = G.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	const float TileSz = InventoryComponent->TileSize;

	const FIntPoint CurrentDims = Op->Item->GetCurrentDimensions();
	const FIntPoint MouseTile(
		FMath::FloorToInt(LocalPos.X / TileSz),
		FMath::FloorToInt(LocalPos.Y / TileSz)
	);

	const FIntPoint HalfDims(CurrentDims.X / 2, CurrentDims.Y / 2);
	FIntPoint TopLeft = MouseTile - HalfDims;

	TopLeft.X = FMath::Clamp(TopLeft.X, 0, InventoryComponent->Columns - CurrentDims.X);
	TopLeft.Y = FMath::Clamp(TopLeft.Y, 0, InventoryComponent->Rows - CurrentDims.Y);

	const bool bCanPlace = InventoryComponent->IsRoomAvailableAt(Op->Item, TopLeft, Op->Item);

	UE_LOG(LogInventory, Log, TEXT("NativeOnDragEnter: Initial preview at (%d,%d), CanPlace=%s"),
	       TopLeft.X, TopLeft.Y, bCanPlace ? TEXT("Yes") : TEXT("No"));
}

void UInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	UE_LOG(LogInventory, Log, TEXT("NativeOnDragLeave: Removing preview"));
}

bool UInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                        UDragDropOperation* InOperation)
{
	UE_LOG(LogInventory, Warning, TEXT("==== [InventoryGridWidget::OnDrop] START ===="));

	auto* Op = Cast<UItemDragOperation>(InOperation);
	if (!Op || !Op->Item || !InventoryComponent)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDrop -> Invalid operation"));
		return false;
	}

	UItemBase* Item = Op->Item;
	UInventoryComponent* SourceInv = Op->SourceInventoryComponent;
	UInventoryComponent* TargetInv = this->InventoryComponent;

	if (!SourceInv)
	{
		UE_LOG(LogInventory, Error, TEXT("OnDrop -> SourceInventoryComponent is null!"));
		return false;
	}

	const bool bCanPlace = TargetInv->IsRoomAvailableAt(
		Item,
		DraggedItemTopLeftTile,
		(SourceInv == TargetInv) ? Item : nullptr
	);

	UE_LOG(LogInventory, Warning, TEXT("OnDrop -> SourceInv=%s, TargetInv=%s, CanPlace=%s, Tile=(%d,%d)"),
	       *SourceInv->GetOwner()->GetName(),
	       *TargetInv->GetOwner()->GetName(),
	       bCanPlace ? TEXT("TRUE") : TEXT("FALSE"),
	       DraggedItemTopLeftTile.X, DraggedItemTopLeftTile.Y);

	if (!bCanPlace)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDrop -> Cannot place item, cancelling drop"));

		ActiveOp = nullptr;
		DraggedItemRef = nullptr;
		return false;
	}

	if (SourceInv == TargetInv)
	{
		UE_LOG(LogInventory, Log, TEXT("OnDrop -> Same inventory move"));

		TargetInv->RemoveItem(Item);
		const int32 TargetIndex = TargetInv->TileToIndex(DraggedItemTopLeftTile);
		TargetInv->AddItemAt(Item, TargetIndex);
		TargetInv->RefreshAllItems();
		Refresh();

		TargetInv->Server_MoveItem(Item, DraggedItemTopLeftTile);

		UE_LOG(LogInventory, Warning, TEXT("OnDrop -> Moved within inventory at TL=(%d,%d) Index=%d"),
		       DraggedItemTopLeftTile.X, DraggedItemTopLeftTile.Y, TargetIndex);
	}

	else
	{
		UE_LOG(LogInventory, Log, TEXT("OnDrop -> Cross-inventory transfer"));

		const int32 TargetIndex = TargetInv->TileToIndex(DraggedItemTopLeftTile);

		SourceInv->RemoveItem(Item);
		SourceInv->RefreshAllItems();
		Refresh();

		TargetInv->AddItemAt(Item, TargetIndex);
		TargetInv->RefreshAllItems();
		Refresh();

		SourceInv->RemoveItemForServer(Item);
		TargetInv->AddItemAtForServer(Item, TargetIndex);

		UE_LOG(LogInventory, Warning, TEXT("OnDrop -> Transferred from %s to %s at TL=(%d,%d) Index=%d"),
		       *SourceInv->GetOwner()->GetName(),
		       *TargetInv->GetOwner()->GetName(),
		       DraggedItemTopLeftTile.X, DraggedItemTopLeftTile.Y, TargetIndex);
	}

	ActiveOp = nullptr;
	DraggedItemRef = nullptr;

	UE_LOG(LogInventory, Warning, TEXT("==== [InventoryGridWidget::OnDrop] END ===="));
	return true;
}

FReply UInventoryGridWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UE_LOG(LogInventory, Warning, TEXT("==== [Grid::NativeOnPreviewKeyDown] Key=%s ===="),
	       *InKeyEvent.GetKey().ToString());

	if (InKeyEvent.GetKey() == EKeys::R)
	{
		if (!ActiveOp)
		{
			return FReply::Unhandled();
		}
		if (!ActiveOp->Item)
		{
			return FReply::Unhandled();
		}
		if (!InventoryComponent)
		{
			return FReply::Unhandled();
		}

		UItemBase* Item = ActiveOp->Item;

		UE_LOG(LogInventory, Warning, TEXT("NativeOnPreviewKeyDown -> Rotating item, Was: %s"),
		       Item->GetIsRotated() ? TEXT("Rotated") : TEXT("Normal"));

		const FIntPoint OldDims = Item->GetCurrentDimensions();
		const FIntPoint OldCenter = DraggedItemTopLeftTile + FIntPoint(OldDims.X / 2, OldDims.Y / 2);

		Item->RotateItem();

		const FIntPoint NewDims = Item->GetCurrentDimensions();
		FIntPoint NewTopLeft = OldCenter - FIntPoint(NewDims.X / 2, NewDims.Y / 2);

		NewTopLeft.X = FMath::Clamp(NewTopLeft.X, 0, InventoryComponent->Columns - NewDims.X);
		NewTopLeft.Y = FMath::Clamp(NewTopLeft.Y, 0, InventoryComponent->Rows - NewDims.Y);

		DraggedItemTopLeftTile = NewTopLeft;

		UE_LOG(LogInventory, Warning,
		       TEXT("NativeOnPreviewKeyDown -> Rotated: OldCenter=(%d,%d), NewTopLeft=(%d,%d), NewDims=(%d,%d)"),
		       OldCenter.X, OldCenter.Y, NewTopLeft.X, NewTopLeft.Y, NewDims.X, NewDims.Y);

		if (ActiveOp->GhostItemWidget)
		{
			UE_LOG(LogInventory, Warning, TEXT("NativeOnPreviewKeyDown -> Updating Ghost Visual"));
			ActiveOp->GhostItemWidget->UpdateVisual();
		}
		InvalidateLayoutAndVolatility();
		ForceLayoutPrepass();

		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
