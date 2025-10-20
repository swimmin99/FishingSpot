
#pragma once

#include "CoreMinimal.h"
#include "MultiplayerPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "FishingPlayerController.generated.h"

class ULeaderboardWidget;
class UShopInventoryWidget;
class UInputMappingContext;
class AFishingGameState;
class UInventoryWidget;
class UInventoryComponent;
class UInteractionWidget;
class UHUDWidget;
class AShopCharacter;
class UShopTransactionManager;
class IInteractable;
class UPauseMenuWidget;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;


USTRUCT()
struct FRenderTargetPoolResource
{
    GENERATED_BODY()
    
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;
    
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> AlphaMaterial = nullptr;
};

UCLASS(abstract)
class AFishingPlayerController : public AMultiplayerPlayerController
{
    GENERATED_BODY()
    
protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Input", meta = (AllowPrivateAccess = "true"))
    TArray<UInputMappingContext*> DefaultMappingContexts;

    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;

    const int32 TopOrder = 4;
    const int32 HighOrder = 3;
    const int32 MiddleOrder = 2;
    const int32 BottomOrder = 1;
    const int32 LowestOrder = 0;

public:
  
    
    
    UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<UHUDWidget> HUDWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<UShopInventoryWidget> ShopInventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="UI|Interaction")
    TSubclassOf<UInteractionWidget> InteractionWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="UI|PauseMenu")
    TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="UI|PauseMenu")
    TSubclassOf<ULeaderboardWidget> LeaderBoardWidgetClass;

    
    UPROPERTY()
    UPauseMenuWidget* CachedPauseWidget = nullptr;
    
    ULeaderboardWidget* CachedLeaderBoardWidget = nullptr;

    UPROPERTY()
    UHUDWidget* HUDWidget = nullptr;

    UPROPERTY()
    UInventoryWidget* InventoryWidget = nullptr;

    UPROPERTY()
    UInteractionWidget* InteractionWidget = nullptr;

    UPROPERTY()
    UShopInventoryWidget* ShopInventoryWidget = nullptr;

    UPROPERTY()
    UShopInventoryWidget* PlayerShopInventoryWidget = nullptr;

    UPROPERTY()
    UShopTransactionManager* ShopTransactionManager = nullptr;

    UPROPERTY()
    AShopCharacter* CurrentShop = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    UMaterialInterface* PortraitPostProcessMaterial;
    
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
    UMaterialInterface* AlphaInvertMaterial; 

    UPROPERTY()
    TArray<FRenderTargetPoolResource> RenderTargetPool;

    
    void InitializeWidgets(UInventoryComponent* PlayerInventory);

    
    UFUNCTION(BlueprintCallable, Category="UI")
    void TogglePauseMenu();

    UFUNCTION(BlueprintCallable, Category="UI")
    void ShowItemAcquiredBanner(UItemBase* Item);

    
    void ToggleLeaderBoard();

    
    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleInventory(UInventoryComponent* PlayerInventory);


    void OpenInventory(UInventoryComponent* PlayerInventory);
    void CloseInventory();
    bool IsInventoryOpen() const ;

    
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OpenShopUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OpenExchangeUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void CloseShopUI();

    UFUNCTION(BlueprintCallable, Category = "Shop")
    bool IsShopOpen() const { return CurrentShop != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Shop")
    AShopCharacter* GetCurrentShop() const { return CurrentShop; }

    
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowInteractionPrompt(const TScriptInterface<IInteractable>& Target);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideInteractionPrompt();

    
    void SetUIMode(bool bEnableUI);
    void SetGameOnlyMode();

    
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool IsUIOpen() const;

    
    UFUNCTION(BlueprintCallable, Category="HUD")
    int32 GetGoldSafe() const;

    UFUNCTION(BlueprintCallable, Category="HUD")
    FString GetLocalTimeString() const;

private:
    UPROPERTY()
    UInventoryWidget* OpponentInventoryWidget;

    
    
    void InitializeRenderTargetPool();
    FRenderTargetPoolResource GetRenderTargetFromPool(int32 Index);

    
    UInventoryWidget* GetOrCreatePlayerInventoryWidget(UInventoryComponent* PlayerInventory);
    UShopInventoryWidget* GetOrCreateLeftShopWidget();
    UShopInventoryWidget* GetOrCreateRightShopWidget(UInventoryComponent* PlayerInventory);

    
    void ShowInventoryWidget(UInventoryComponent* PlayerInventory);
    void HideInventoryWidget();
    
    void ShowShopWidgets(UInventoryComponent* PlayerInventory, UInventoryComponent* ShopInventory);
    void HideShopWidgets();
    
    void ShowExchangeWidgets(UInventoryComponent* PlayerInventory, UInventoryComponent* ShopInventory);
    void CloseExchangeWidget();
    
    
};