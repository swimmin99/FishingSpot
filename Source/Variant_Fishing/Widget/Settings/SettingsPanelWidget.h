#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsPanelWidget.generated.h"

class UBasicMenuWidget;
class UMultiplayerMenuWidget;
class UHostingMenuWidget;
class UJoiningMenuWidget;

UCLASS()
class FISHING_API USettingsPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void TogglePanel();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ShowPanel();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void HidePanel();

	void ShowBasicMenu();
	void ShowMultiplayerMenu();
	void ShowHostingMenu();
	void ShowJoinMenu();

	void GoBack();

	bool IsPanelOpen() const { return bIsPanelOpen; }

protected:
	UPROPERTY(meta = (BindWidget))
	UBasicMenuWidget* BasicMenuWidget;

	UPROPERTY(meta = (BindWidget))
	UMultiplayerMenuWidget* MultiplayerMenuWidget;

	UPROPERTY(meta = (BindWidget))
	UHostingMenuWidget* HostingMenuWidget;

	UPROPERTY(meta = (BindWidget))
	UJoiningMenuWidget* JoinMenuWidget;

private:
	UPROPERTY()
	UUserWidget* CurrentMenu = nullptr;

	UPROPERTY()
	TArray<UUserWidget*> MenuStack;

	bool bIsPanelOpen = false;

	void ShowMenu(UUserWidget* Menu);

	void HideAllMenus();

	void SetupUIMode();

	void RestoreGameMode();
};
