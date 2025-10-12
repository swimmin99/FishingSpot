#pragma once

#include "Blueprint/UserWidget.h"
#include "UHUDWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class UButton;
class UBorder;
class UWidget;

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
	UButton* SettingsButton = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBorder* RightSection = nullptr;
	UPROPERTY(meta=(BindWidget))
	UBorder* LeftSection = nullptr;
	UPROPERTY(meta=(BindWidget))
	UCanvasPanel* WindowSection = nullptr;
	UPROPERTY(meta=(BindWidget))
	UButton* CloseButton = nullptr;
};
