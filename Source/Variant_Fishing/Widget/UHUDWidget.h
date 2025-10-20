#pragma once

#include "Blueprint/UserWidget.h"
#include "UHUDWidget.generated.h"

class UBaseButtonWidget;
class UCanvasPanel;
class UTextBlock;
class UButton;
class UBorder;
class UWidget;
class UVerticalBox;
class UItemAcquiredBannerWidget;

UCLASS()
class FISHING_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="HUD")
	void AttachWidgetToRight(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void AttachWidgetToLeft(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void ClearWidgetInLeft();

	UFUNCTION(BlueprintCallable, Category="HUD")
	void ClearWidgetInRight();

	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowCloseSection();

	UFUNCTION(BlueprintCallable, Category="HUD")
	void HideCloseSection();

	UFUNCTION(BlueprintCallable, Category="HUD")
	void RefreshMoney();

	UFUNCTION(BlueprintCallable, Category="HUD")
	void RefreshTime();

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetPlayerName(FString PlayerName);

private:
	UFUNCTION()
	void OnClickSettings();

	UFUNCTION()
	void OnClickClose();

	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

public:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* MoneyText = nullptr;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* TimeText = nullptr;
	UPROPERTY(meta=(BindWidget))
	UTextBlock* PlayerNameLabel = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBaseButtonWidget* SettingsButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBorder* RightSection = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBorder* LeftSection = nullptr;
	UPROPERTY(meta=(BindWidget))
	UCanvasPanel* WindowSection = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBaseButtonWidget* CloseButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UVerticalBox* NoticeVerticalBox = nullptr;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Banner")
	TSubclassOf<UItemAcquiredBannerWidget> BannerWidgetClass;

	UPROPERTY()
	UItemAcquiredBannerWidget* ItemBanner;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowItemAcquiredBanner(UItemBase* Item);
};
