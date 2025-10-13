#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Fishing/Variant_Fishing/Data/InventoryDataStruct.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "InventoryGridWidget.generated.h"

class UItemDragOperation;
class UCanvasPanel;
class UBorder;
class UInventoryComponent;
class UItemBase;

UCLASS()
class FISHING_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Clean();
	void Refresh();
	void InitializeWidget(UInventoryComponent* InInventoryComponent);

protected:
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UCanvasPanel* Canvas;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UBorder* GridBorder;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UCanvasPanel* GridCanvasPanel;

	UPROPERTY()
	UInventoryComponent* InventoryComponent = nullptr;

	UPROPERTY()
	TArray<UUserWidget*> CreatedItemWidget;

	UPROPERTY()
	UItemDragOperation* ActiveOp = nullptr;

	UPROPERTY()
	UItemBase* DraggedItemRef = nullptr;

	FIntPoint DraggedItemTopLeftTile;

	int32 Colums;
	int32 Rows;
	float TileSize;

	FLines LineStructData;

	TArray<float> StartX;
	TArray<float> StartY;
	TArray<float> EndX;
	TArray<float> EndY;

	FMousePositionInTile MousePositionInTile;

	void ConstructBaseBackground();
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                          UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                              UDragDropOperation* InOperation) override;

	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                               UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	                          const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void CreateLineSegments();
	FMousePositionInTile MousePositionInTileResult(FVector2D MousePosition);
};
