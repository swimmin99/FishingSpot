#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "InventoryGridWidget.generated.h"

class UItemDragOperation;
class UCanvasPanel;
class UBorder;
class UInventoryComponent;
class UItemBase;

USTRUCT(BlueprintType) 
struct FLines
{
	GENERATED_BODY()

public:
	FLines(){};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Lines")
	TArray<FVector2D> XLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Lines")
	TArray<FVector2D> YLines;
};

USTRUCT(BlueprintType)
struct FMousePositionInTile
{
	GENERATED_BODY()

public:
	FMousePositionInTile() : Right(false), Down(false)	{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouse Position")
	bool Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouse Position")
	bool Down;
};


UCLASS()
class FISHING_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Clean();
	void Refresh();
	void InitializeWidget(UInventoryComponent* InInventoryComponent);
	void SetCategoryFilter(EItemCategory NewFilter);
	bool IsPlayerInventory();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	float TileSize = 48.0f;
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Filter")
	EItemCategory CurrentFilter = EItemCategory::All;

	
	
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