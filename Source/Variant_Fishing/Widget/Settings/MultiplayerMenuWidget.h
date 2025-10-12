#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerMenuWidget.generated.h"

class UButton;
class USettingsPanelWidget;

UCLASS()
class FISHING_API UMultiplayerMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetParentPanel(USettingsPanelWidget* Panel) { ParentPanel = Panel; }

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

private:
	UPROPERTY()
	USettingsPanelWidget* ParentPanel;

	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnJoinClicked();

	UFUNCTION()
	void OnBackClicked();

	void UpdateHostButtonVisibility();
};
