#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BasicMenuWidget.generated.h"

class UButton;
class USettingsPanelWidget;

UCLASS()
class FISHING_API UBasicMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetParentPanel(USettingsPanelWidget* Panel) { ParentPanel = Panel; }

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* ResumeButton;

	UPROPERTY(meta = (BindWidget))
	UButton* MultiplayerButton;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

private:
	UPROPERTY()
	USettingsPanelWidget* ParentPanel;

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnMultiplayerClicked();

	UFUNCTION()
	void OnQuitClicked();
};
