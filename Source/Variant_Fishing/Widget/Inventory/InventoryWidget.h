#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UVerticalBox;
class UCanvasPanel;
class UBorder;
class UBackgroundBlur;
class UItemBase;
class AFishingCharacter;
class UInventoryComponent;
class UInventoryGridWidget;
class UInventoryDescriptionWidget;
class UTextBlock;

UCLASS()
class FISHING_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FText& InText);
	void SetDroppable(bool value) { bDroppable = value; }
	void SetFocusGridWidget();
	UPROPERTY()
	bool bDroppable = true;

	UFUNCTION()
	void RefreshGrid();
	void UpdateDescription(UItemBase* TargetItem);
	void ClearDescription();

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UVerticalBox* MainVerticalBox;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UInventoryGridWidget* InventoryGridWidget;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void InitializeWidget(UInventoryComponent* InInventoryComponent);

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UInventoryComponent* ConnectedInventoryComponent = nullptr;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UInventoryDescriptionWidget* ItemDescriptionWidget;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UTextBlock* TextTitle;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                          UDragDropOperation* InOperation) override;
};
