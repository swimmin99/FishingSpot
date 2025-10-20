

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "BaseButtonWidget.generated.h"

class UImage;


UCLASS()
class FISHING_API UBaseButtonWidget : public UUserWidget
{
	GENERATED_BODY()
public :
	UFUNCTION(BlueprintCallable, Category= "Button")
	void SetSelected(bool bInSelected);
	void SetText(FText InText)
	{
		if (BaseText)
		{
			BaseText->SetText(InText);
		}
	}
	
	UButton* GetBaseButton() { return BaseButton;}
	
	UPROPERTY()
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UButton* BaseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* BaseText;
		
	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor SelectedTextColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor UnselectedTextColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor SelectedBGColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor UnselectedBGColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	
	
};
