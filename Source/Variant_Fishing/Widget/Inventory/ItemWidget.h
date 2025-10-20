#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemWidget.generated.h"

class UCanvasPanel;
class USizeBox;
class UBorder;
class UImage;
class AFishingCharacter;
class UItemBase;
class UInventoryComponent;

UCLASS()

class FISHING_API UItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetFilteredOutState(bool bFiltered);

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanInteract() const { return !bIsFilteredOut; }
	
	UFUNCTION(BlueprintCallable, Category = "Item Widget")
	void UpdateVisual(float InTileSize);

	void SetItemWidget(UItemBase* ItemToAdd, UInventoryComponent* InOwnerInventoryComponent, float InTileSize);

	float TileSize = 48.0f;
private:
	UPROPERTY()
	bool bIsFilteredOut = false;
	

	UPROPERTY()
	UInventoryComponent* OwnerInventoryComponent = nullptr;

protected:
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UCanvasPanel* Canvas;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UBorder* BackgroundBorder;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	USizeBox* BackgroundSizeBox;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UImage* ItemImage;

	UPROPERTY()
	UItemBase* Item = nullptr;

	FVector2D Size;

	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	                                  UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
