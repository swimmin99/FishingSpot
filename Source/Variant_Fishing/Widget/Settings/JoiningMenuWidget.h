#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JoiningMenuWidget.generated.h"
class UButton;
class UEditableTextBox;
class UTextBlock;
class USettingsPanelWidget;

UCLASS()
class FISHING_API UJoiningMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetParentPanel(USettingsPanelWidget* Panel) { ParentPanel = Panel; }

protected:
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* IPAddressInput;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

private:
	UPROPERTY()
	USettingsPanelWidget* ParentPanel;

	UFUNCTION()
	void OnJoinButtonClicked();

	UFUNCTION()
	void OnBackClicked();

	void AttemptJoin(const FString& IPAddress);

	void ShowStatus(const FString& Message, const FLinearColor& Color = FLinearColor::White);
	void ClearStatus();
};
