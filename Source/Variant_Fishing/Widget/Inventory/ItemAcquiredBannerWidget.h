
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemAcquiredBannerWidget.generated.h"

class UItemBase;
class UTextBlock;
class UImage;

UCLASS()
class FISHING_API UItemAcquiredBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "Banner")
	void ShowBanner(UItemBase* Item);

	
	UFUNCTION(BlueprintCallable, Category = "Banner")
	void HideBanner();

	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* ItemNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* ItemDescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	UImage* ItemIcon;

	

	
	UPROPERTY(EditDefaultsOnly, Category = "Banner")
	float DisplayDuration = 3.0f;

private:
	FTimerHandle HideTimerHandle;
    
	void OnFadeOutComplete();
};