#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HostingMenuWidget.generated.h"

class UButton;
class UTextBlock;
class USettingsPanelWidget;

UCLASS()
class FISHING_API UHostingMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetParentPanel(USettingsPanelWidget* Panel) { ParentPanel = Panel; }

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

private:
	UPROPERTY()
	USettingsPanelWidget* ParentPanel;

	bool bIsHosting = false;

	UFUNCTION()
	void OnHostButtonClicked();

	UFUNCTION()
	void OnBackClicked();

	void StartHosting();
	void StopHosting();

	void UpdateUI();
};
