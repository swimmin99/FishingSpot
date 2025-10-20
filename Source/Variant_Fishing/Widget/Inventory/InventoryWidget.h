#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "InventoryWidget.generated.h"

class UItemToolTipWidget;
class UVerticalBox;
class UCanvasPanel;
class UBorder;
class UBackgroundBlur;
class UItemBase;
class AFishingCharacter;
class UInventoryComponent;
class UInventoryGridWidget;
class UInventoryPortrait;
class UTextBlock;
class AInteractableCharacter;
class UCategoryFilterButton;
class UHorizontalBox;

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
	
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UVerticalBox* MainVerticalBox;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UInventoryGridWidget* InventoryGridWidget;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void InitializeWidget(UInventoryComponent* InInventoryComponent, AInteractableCharacter* InInteractableCharacter, UMaterialInstanceDynamic* PortraitMaterial);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Category")
	void CreateCategoryFilterButtons();

	UFUNCTION()
	void OnCategoryFilterSelected(FString ButtonID);
	FText GetCategoryDisplayName(EItemCategory Category) const;


	UPROPERTY(VisibleAnywhere, meta = (BindWidgetOptional), Category="UI")
	UHorizontalBox* CategoryFilterContainer;
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UInventoryComponent* ConnectedInventoryComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	AInteractableCharacter* ConnectedCharacter = nullptr;
		

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UInventoryPortrait* InventoryPortrait;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UTextBlock* TextTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Category")
	TSubclassOf<UCategoryFilterButton> CategoryFilterButtonClass;


	
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                          UDragDropOperation* InOperation) override;


private:
	UPROPERTY()
	TArray<UCategoryFilterButton*> CategoryFilterButtons;

	UPROPERTY()
	EItemCategory CurrentSelectedCategory = EItemCategory::All;
};