#pragma once

#include "CoreMinimal.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "ShopInventoryWidget.generated.h"

class UTextBlock;
class UShopTransactionManager;
class UBaseButtonWidget;

UCLASS()
class FISHING_API UShopInventoryWidget : public UInventoryWidget
{
	GENERATED_BODY()

public:
	void SetTransactionManager(UShopTransactionManager* InManager);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void UpdatePriceDisplay();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void UpdateConfirmButtonState();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	bool bIsPlayerInventory = false;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	UInventoryComponent* GetConnectedInventory();

	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UBaseButtonWidget* ConfirmButton;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category="UI")
	UTextBlock* PriceText;

	
private:
	UPROPERTY()
	UShopTransactionManager* TransactionManager = nullptr;

	UFUNCTION()
	void OnConfirmButtonClicked();
};