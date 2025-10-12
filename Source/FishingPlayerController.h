

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FishingPlayerController.generated.h"

class UShopInventoryWidget;
class UInputMappingContext;
class AFishingPlayerState;
class UInventoryWidget;
class UInventoryComponent;
class UInteractionWidget;
class UHUDWidget;
class AShopCharacter;
class UShopTransactionManager;
class IInteractable;
class USettingsPanelWidget; 

UCLASS(abstract)
class AFishingPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Input", meta = (AllowPrivateAccess = "true"))
	TArray<UInputMappingContext*> DefaultMappingContexts;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;

	
	const int32 TopOrder = 3;
	const int32 MiddleOrder = 2;
	const int32 BottomOrder = 1;
	const int32 LowestOrder = 0;

public:
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UShopInventoryWidget> ShopInventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI|Interaction")
	TSubclassOf<UInteractionWidget> InteractionWidgetClass;

	
	UPROPERTY(EditDefaultsOnly, Category="UI|Settings")
	TSubclassOf<USettingsPanelWidget> SettingsPanelClass;

	
	UPROPERTY()
	USettingsPanelWidget* SettingsPanel = nullptr;
	
	UPROPERTY()
	UHUDWidget* HUDWidget = nullptr;

	UPROPERTY()
	UUserWidget* InventoryWidget = nullptr;

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

	
	
	void InitializeWidgets(UInventoryComponent* PlayerInventory);

	
	
	UFUNCTION(BlueprintCallable, Category="UI")
	void ToggleSettings();

	
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInventory(UInventoryComponent* PlayerInventory);

	
	void OpenInventory(UInventoryComponent* PlayerInventory);

	
	void CloseInventory();

	
	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShopUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory);

	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenExchangeUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory);
	void SetInventory(UInventoryWidget* InInventoryWidget, UInventoryComponent* TargetInventoryComponent);

	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void CloseShopUI();

	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool IsShopOpen() const { return CurrentShop != nullptr; }

	/**
	 * Get currently open shop
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop")
	AShopCharacter* GetCurrentShop() const { return CurrentShop; }

	// ===== Interaction Prompt =====
	/**
	 * Show interaction prompt at bottom of screen
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowInteractionPrompt(const TScriptInterface<IInteractable>& Target);

	/**
	 * Hide interaction prompt
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideInteractionPrompt();

	// ===== Input Mode Management =====
	/**
	 * Switch to UI mode (show cursor, GameAndUI input)
	 */
	void SetUIMode(bool bEnableUI);

	/**
	 * Switch to Game-only mode (hide cursor, GameOnly input)
	 */
	void SetGameOnlyMode();

	// ===== UI State Check =====
	/**
	 * Check if any UI is currently open (Inventory, Shop, or Settings)
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool IsUIOpen() const;

	// ===== HUD Bridge Functions =====
	UFUNCTION(BlueprintCallable, Category="HUD")
	int32 GetGoldSafe() const;

	UFUNCTION(BlueprintCallable, Category="HUD")
	FString GetLocalTimeString() const;

private:
	// ===== Helper Functions =====
	/**
	 * Create and attach normal inventory widget to HUD Right
	 */
	void CreateNormalInventoryWidget(UInventoryComponent* PlayerInventory);
	void UpdateInventoryLayout(UInventoryWidget* Widget, UInventoryComponent* Inventory);

	/**
	 * Create Shop UI widgets (Left: Shop, Right: Player Shop version)
	 */
	void CreateShopWidgets(UInventoryComponent* PlayerInventory, UInventoryComponent* ShopInventory);

	/**
	 * Cleanup Shop UI widgets
	 */
	void CleanupShopWidgets();
};