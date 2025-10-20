
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/Database/DatabaseManager.h"
#include "MainMenuWidget.generated.h"

class UBaseButtonWidget;
class UWidgetSwitcher;
class UScrollBox;
class UEditableTextBox;
class USaveSlotItemWidget;


UCLASS()
class FISHING_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    
    
    
    
    
    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* NewGameButton;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* LoadGameButton;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* JoinGameButton;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* OptionsButton;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* ExitButton;

    
    UPROPERTY(meta = (BindWidget))
    UWidgetSwitcher* MenuSwitcher;

    
    
    
    
    UPROPERTY(meta = (BindWidget))
    UScrollBox* SaveSlotList;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* BackFromLoadButton;

    
    
    
    
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PlayerNameInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* VillageNameInput;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* CreateGameButton;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* BackFromNewGameButton;

    
    
    
    
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* ServerIPInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* JoinPlayerNameInput;

    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* JoinServerButton;
    
    UPROPERTY(meta = (BindWidget))
    UBaseButtonWidget* BackFromJoinButton;

    
    
    
    
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USaveSlotItemWidget> SaveSlotItemClass;

    
    
    
    
    
    UFUNCTION()
    void OnNewGameClicked();

    
    UFUNCTION()
    void OnLoadGameClicked();

    
    UFUNCTION()
    void OnJoinGameClicked();

    
    UFUNCTION()
    void OnOptionsClicked();

    
    UFUNCTION()
    void OnExitClicked();

    
    UFUNCTION()
    void OnCreateGameClicked();

    
    UFUNCTION()
    void OnJoinServerClicked();

    
    UFUNCTION()
    void OnBackClicked();

    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void RefreshSaveSlots();

    
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void SelectSaveSlot(int32 DatabasePlayerID);

    
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void DeleteSaveSlot(const FString& VillageName);

	UPROPERTY()
	FName MainGameLevelName = TEXT("BasicMap");
	
    
    void SwitchToPanel(int32 Index);

private:
    
    TArray<FSaveSlotInfo> CachedSaveSlots;
};