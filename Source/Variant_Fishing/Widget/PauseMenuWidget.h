

#pragma once

#include "CoreMinimal.h"
#include "FishingPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UBaseButtonWidget;
class AMultiplayerPlayerController;


UCLASS()
class FISHING_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(meta = (BindWidget))
	UBaseButtonWidget* ResumeButton;

	UPROPERTY(meta = (BindWidget))
	UBaseButtonWidget* SaveButton;

	UPROPERTY(meta = (BindWidget))
	UBaseButtonWidget* MainMenuButton;

	
	UFUNCTION(BlueprintPure, Category = "PauseMenu")
	bool IsPanelOpen() const;

	
	UFUNCTION(BlueprintCallable, Category = "PauseMenu")
	void TogglePanel();

protected:
	
	UPROPERTY(Transient)
	AMultiplayerPlayerController* CachedMultiplayerController = nullptr;

	
	virtual void NativeConstruct() override;

	
	UFUNCTION()
	void OnClickedResume();
	
	UFUNCTION()
	void OnClickedMainMenu();

	UFUNCTION()
	void OnClickedSave();

	
	UFUNCTION(BlueprintCallable, Category = "PauseMenu")
	void OnOpen();

	UFUNCTION(BlueprintCallable, Category = "PauseMenu")
	void OnClosed();
};
