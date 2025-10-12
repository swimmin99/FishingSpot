#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/ActorComponent/InventoryComponent.h"
#include "InventoryDescriptionWidget.generated.h"

class UItemBase;

class UCanvasPanel;
class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UScrollBox;

UCLASS()
class FISHING_API UInventoryDescriptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	constexpr static const TCHAR* MoneyChar = TEXT("$");

	TWeakObjectPtr<UInventoryComponent> LinkedInventoryComponent;

	UFUNCTION(BlueprintCallable, Category = "Description")
	void UpdateContent(UItemBase* Item);

	UFUNCTION(BlueprintCallable, Category = "Description")
	void ClearContent();
	void InitializeWidget(UInventoryComponent* InventoryComponent);

protected:
	virtual void NativeConstruct() override;

	void SetTitleText(const FString& InTitle);
	void ClearTitleText();

	void SetPriceText(const FString& InPrice);
	void ClearPriceText();

	void SetDescriptionText(const FString& InDesc);
	void ClearDescriptionText();

public:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UCanvasPanel* Canvas = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UBorder* GridBorder = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UVerticalBox* VBoxRoot = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UTextBlock* TitleText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UHorizontalBox* PriceRow = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UTextBlock* PriceLabelText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UTextBlock* PriceValueText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UScrollBox* DescriptionScroll = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UI")
	UTextBlock* DescriptionText = nullptr;
};
