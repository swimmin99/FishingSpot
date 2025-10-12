#pragma once
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

class UTextBlock;

UCLASS()
class FISHING_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetLabel(const FText& InText);

protected:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* Text;
};
