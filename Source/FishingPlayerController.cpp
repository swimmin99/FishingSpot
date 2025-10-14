

#include "FishingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Fishing.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InteractableCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/Widget/InteractionWidget.h"
#include "Variant_Fishing/Widget/UHUDWidget.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "Variant_Fishing/Widget/Shop/ShopInventoryWidget.h"
#include "Variant_Fishing/Widget/Settings/SettingsPanelWidget.h" 
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Data/ShopTransactionData.h"
#include "Variant_Fishing/NPC/ShopCharacter.h"
#include "Variant_Fishing/Interface/Interactable.h"






void AFishingPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	
}

void AFishingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}

void AFishingPlayerController::InitializeWidgets(UInventoryComponent* PlayerInventory)
{
	if (!IsLocalController() || !PlayerInventory)
	{
		UE_LOG(LogFishingPC, Warning, TEXT("InitializeWidgets: Not local controller or no inventory"));
		return;
	}

	UE_LOG(LogFishingPC, Log, TEXT("Initializing UI widgets for controller"));

	
	if (SettingsPanelClass && !SettingsPanel)
	{
		SettingsPanel = CreateWidget<USettingsPanelWidget>(this, SettingsPanelClass);
		if (SettingsPanel)
		{
			SettingsPanel->AddToViewport(TopOrder);
			SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogFishingPC, Log, TEXT("✅ Settings Panel created"));
		}
		else
		{
			UE_LOG(LogFishingPC, Error, TEXT("❌ Settings Panel failed to create"));
		}
	}

	
	if (HUDWidgetClass && !HUDWidget)
	{
		HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(MiddleOrder);
			HUDWidget->SetOwningPlayer(this);
			HUDWidget->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogFishingPC, Log, TEXT("✅ HUD widget created"));
		}
		else 
		{
			UE_LOG(LogFishingPC, Error, TEXT("❌ HUD widget failed to create"));
			return;
		}
	}

	
	if (InteractionWidgetClass && !InteractionWidget)
	{
		InteractionWidget = CreateWidget<UInteractionWidget>(this, InteractionWidgetClass);
		if (InteractionWidget)
		{
			InteractionWidget->AddToViewport(BottomOrder);
			InteractionWidget->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogFishingPC, Log, TEXT("✅ Interaction widget created"));
		}
	}

	UE_LOG(LogFishingPC, Log, TEXT("✅ Widget initialization complete"));
}





void AFishingPlayerController::ToggleSettings()
{
	if (!SettingsPanel)
	{
		UE_LOG(LogFishingPC, Warning, TEXT("ToggleSettings: SettingsPanel is null"));
		return;
	}

	
	if (InventoryWidget)
	{
		CloseInventory();
	}

	if (IsShopOpen())
	{
		CloseShopUI();
	}

	
	SettingsPanel->TogglePanel();

	UE_LOG(LogFishingPC, Log, TEXT("⚙️ Settings toggled: %s"), 
		SettingsPanel->IsPanelOpen() ? TEXT("OPEN") : TEXT("CLOSED"));
}





void AFishingPlayerController::ToggleInventory(UInventoryComponent* PlayerInventory)
{
	
	if (ShopInventoryWidget)
	{
		CloseShopUI();
		return;
	}

	if (!PlayerInventory)
	{
		UE_LOG(LogFishingPC, Warning, TEXT("ToggleInventory: PlayerInventory is null"));
		return;
	}

	// Toggle inventory
	if (InventoryWidget)
	{
		CloseInventory();
	}
	else
	{
		OpenInventory(PlayerInventory);
	}
}

void AFishingPlayerController::OpenInventory(UInventoryComponent* PlayerInventory)
{
	if (!PlayerInventory)
	{
		UE_LOG(LogFishingPC, Warning, TEXT("OpenInventory: PlayerInventory is null"));
		return;
	}

	// Already open
	if (InventoryWidget)
	{
		UE_LOG(LogFishingPC, Warning, TEXT("OpenInventory: Already open"));
		return;
	}

	// Close shop if open
	if (IsShopOpen())
	{
		CloseShopUI();
	}

	// Create inventory widget
	CreateNormalInventoryWidget(PlayerInventory);

	// Enable UI mode
	SetUIMode(true);

	// Hide interaction prompt
	HideInteractionPrompt();

	UE_LOG(LogFishingPC, Log, TEXT("📦 Inventory opened"));
}

void AFishingPlayerController::CloseInventory()
{
	if (!InventoryWidget)
	{
		return;
	}

	// Remove widget
	if (HUDWidget)
	{
		HUDWidget->ClearWidgetInRight();
	}
	
	InventoryWidget->RemoveFromParent();
	InventoryWidget = nullptr;

	// Restore game mode
	SetGameOnlyMode();

	UE_LOG(LogFishingPC, Log, TEXT("📦 Inventory closed"));
}

// ========================================
// Shop UI Management
// ========================================

void AFishingPlayerController::OpenShopUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory)
{
	if (!Shop || !Shop->GetShopInventory() || !PlayerInventory)
	{
		UE_LOG(LogFishingPC, Error, TEXT("OpenShopUI: Invalid parameters"));
		return;
	}

	// Close if already open
	if (IsShopOpen())
	{
		CloseShopUI();
	}

	// Close normal inventory if open
	if (InventoryWidget)
	{
		CloseInventory();
	}

	UE_LOG(LogFishingPC, Log, TEXT("🛒 Opening shop: '%s'"), *Shop->GetName_Implementation());

	CurrentShop = Shop;

	// Create transaction manager
	ShopTransactionManager = NewObject<UShopTransactionManager>(this);
	ShopTransactionManager->Initialize(PlayerInventory, Shop->GetShopInventory());

	// Create shop widgets
	CreateShopWidgets(PlayerInventory, Shop->GetShopInventory());

	// Enable UI mode
	SetUIMode(true);

	// Hide interaction prompt
	HideInteractionPrompt();

	UE_LOG(LogFishingPC, Log, TEXT("✅ Shop UI opened"));
}

void AFishingPlayerController::OpenExchangeUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory)
{
	if (!Shop || !Shop->GetShopInventory() || !PlayerInventory)
	{
		UE_LOG(LogFishingPC, Error, TEXT("OpenExchangeUI: Invalid parameters"));
		return;
	}

	// Close if already open
	if (IsShopOpen())
	{
		CloseShopUI();
	}

	// Close normal inventory if open
	if (InventoryWidget)
	{
		CloseInventory();
	}

	UE_LOG(LogFishingPC, Log, TEXT("🔄 Opening exchange UI: '%s'"), *Shop->GetName_Implementation());

	CurrentShop = Shop;

	// Create exchange widgets (no transaction system)
	if (InventoryWidgetClass && ShopInventoryWidgetClass)
	{
		// Left: Shop Inventory
		ShopInventoryWidget = CreateWidget<UShopInventoryWidget>(GetWorld(), ShopInventoryWidgetClass);
		if (!ShopInventoryWidget)
		{
			return;
		}
		SetInventory(ShopInventoryWidget, Shop->GetShopInventory());
		UpdateInventoryLayout(ShopInventoryWidget, Shop->GetShopInventory());

		ShopInventoryWidget->SetDroppable(false);

		if (ShopInventoryWidget->InventoryGridWidget)
		{
			Shop->GetShopInventory()->RefreshAllItems();
			ShopInventoryWidget->RefreshGrid();
		}

		HUDWidget->AttachWidgetToLeft(ShopInventoryWidget);

		// Right: Player Inventory (normal version)
		CreateNormalInventoryWidget(PlayerInventory);
	}

	// Enable UI mode
	SetUIMode(true);

	// Hide interaction prompt
	HideInteractionPrompt();

	UE_LOG(LogFishingPC, Log, TEXT("✅ Exchange UI opened"));
}

void AFishingPlayerController::SetInventory(UInventoryWidget* InInventoryWidget, UInventoryComponent* TargetInventoryComponent)
{
	InInventoryWidget->InitializeWidget(TargetInventoryComponent);
	TargetInventoryComponent->RegisterInventoryWidget(InInventoryWidget);
}

void AFishingPlayerController::CloseShopUI()
{
	if (!IsShopOpen())
	{
		return;
	}

	UE_LOG(LogFishingPC, Log, TEXT("🛒 Closing shop: '%s'"), 
		CurrentShop ? *CurrentShop->GetName_Implementation() : TEXT("NULL"));

	// Cancel all transactions
	if (ShopTransactionManager)
	{
		ShopTransactionManager->CancelAll();
		ShopTransactionManager = nullptr;
		UE_LOG(LogFishingPC, Log, TEXT("✅ All transactions cancelled"));
	}

	// Cleanup widgets
	CleanupShopWidgets();

	CurrentShop = nullptr;

	// Restore game mode
	SetGameOnlyMode();
	
	UE_LOG(LogFishingPC, Log, TEXT("✅ Shop UI closed"));
}

// ========================================
// Interaction Prompt
// ========================================

void AFishingPlayerController::ShowInteractionPrompt(const TScriptInterface<IInteractable>& Target)
{
	if (!InteractionWidget || !Target.GetObject())
	{
		return;
	}

	// Don't show if any UI is open
	if (IsUIOpen())
	{
		return;
	}

	IInteractable* InteractablePtr = Cast<IInteractable>(Target.GetObject());
	if (!InteractablePtr)
	{
		return;
	}

	FString DisplayName = InteractablePtr->GetName_Implementation();
	
	InteractionWidget->SetLabel(FText::FromString(DisplayName));
	InteractionWidget->SetVisibility(ESlateVisibility::Visible);

	UE_LOG(LogFishingPC, Log, TEXT("💬 Interaction prompt: '%s'"), *DisplayName);
}

void AFishingPlayerController::HideInteractionPrompt()
{
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ========================================
// Input Mode Management
// ========================================

void AFishingPlayerController::SetUIMode(bool bEnableUI)
{
	if (bEnableUI)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetWidgetToFocus(nullptr);
		SetInputMode(InputMode);

		SetShowMouseCursor(true);
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;

		UE_LOG(LogFishingPC, Verbose, TEXT("🖱️ UI Mode enabled"));
	}
	else
	{
		SetGameOnlyMode();
	}
}

void AFishingPlayerController::SetGameOnlyMode()
{
	SetShowMouseCursor(false);
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetInputMode(FInputModeGameOnly());

	UE_LOG(LogFishingPC, Verbose, TEXT("🎮 Game-only Mode enabled"));
}

// ========================================
// HUD Bridge Functions
// ========================================

int32 AFishingPlayerController::GetGoldSafe() const
{
	if (const AInteractableCharacter* PS = Cast<AInteractableCharacter>(GetPawn()))
	{
		return PS->GetGold();
	}
	return 0;
}

FString AFishingPlayerController::GetLocalTimeString() const
{
	const FDateTime Now = FDateTime::Now();
	return Now.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
}

bool AFishingPlayerController::IsUIOpen() const
{
	// Check if inventory is open
	if (InventoryWidget)
	{
		return true;
	}

	// Check if shop is open
	if (IsShopOpen())
	{
		return true;
	}

	// Check if settings panel is open
	if (SettingsPanel && SettingsPanel->IsPanelOpen())
	{
		return true;
	}

	return false;
}

// ========================================
// Private Helper Functions
// ========================================

void AFishingPlayerController::CreateNormalInventoryWidget(UInventoryComponent* PlayerInventory)
{
	if (!HUDWidget || !InventoryWidgetClass || !PlayerInventory)
	{
		UE_LOG(LogFishingPC, Error, TEXT("CreateNormalInventoryWidget: Missing requirements"));
		return;
	}

	if (InventoryWidget)
	{
		UE_LOG(LogFishingPC, Warning, TEXT("CreateNormalInventoryWidget: Already exists"));
		return;
	}

	// Create normal inventory widget
	InventoryWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);
	if (UInventoryWidget* InvWidget = Cast<UInventoryWidget>(InventoryWidget))
	{
		SetInventory(InvWidget, PlayerInventory);
		UpdateInventoryLayout(InvWidget, PlayerInventory);

		InvWidget->SetOwningPlayer(this);
		InvWidget->SetVisibility(ESlateVisibility::Visible);
		InvWidget->SetLabel(FText::FromString("Inventory"));

		// Attach to HUD Right
		HUDWidget->AttachWidgetToRight(InventoryWidget);

		UE_LOG(LogFishingPC, Log, TEXT("✅ Normal Inventory widget created"));
	}
	else
	{
		UE_LOG(LogFishingPC, Error, TEXT("❌ Failed to create normal Inventory widget"));
		InventoryWidget = nullptr;
	}
}

void AFishingPlayerController::UpdateInventoryLayout(UInventoryWidget* Widget, UInventoryComponent* Inventory)
{
	Widget->RefreshGrid();
	if (Widget->InventoryGridWidget)
	{
		Inventory->RefreshAllItems();
	}

}


void AFishingPlayerController::CreateShopWidgets(UInventoryComponent* PlayerInventory, UInventoryComponent* ShopInventory)
{
	if (!HUDWidget || !ShopInventoryWidgetClass || !PlayerInventory || !ShopInventory)
	{
		return;
	}

	// Left: Shop Inventory Widget
	ShopInventoryWidget = CreateWidget<UShopInventoryWidget>(GetWorld(), ShopInventoryWidgetClass);
	if (ShopInventoryWidget)
	{
		SetInventory(ShopInventoryWidget, ShopInventory);  // ✅ ShopInventory 연결
		UpdateInventoryLayout(ShopInventoryWidget, ShopInventory);
		
		ShopInventoryWidget->bIsPlayerInventory = false;
		ShopInventoryWidget->SetTransactionManager(ShopTransactionManager);
		ShopInventoryWidget->SetDroppable(false);
		ShopInventoryWidget->SetLabel(FText::FromString("Shop"));
		

		HUDWidget->AttachWidgetToLeft(ShopInventoryWidget);
		UE_LOG(LogFishingPC, Log, TEXT("✅ Shop Inventory widget created"));
	}
	// Right: Player Shop Inventory Widget
	PlayerShopInventoryWidget = CreateWidget<UShopInventoryWidget>(GetWorld(), ShopInventoryWidgetClass);
	if (PlayerShopInventoryWidget)
	{
		SetInventory(PlayerShopInventoryWidget, PlayerInventory);  // ✅ PlayerShopInventoryWidget에 PlayerInventory 연결
		UpdateInventoryLayout(PlayerShopInventoryWidget, PlayerInventory);

		PlayerShopInventoryWidget->bIsPlayerInventory = true;
		PlayerShopInventoryWidget->SetTransactionManager(ShopTransactionManager);
		PlayerShopInventoryWidget->SetDroppable(false);
		PlayerShopInventoryWidget->SetLabel(FText::FromString("Player"));
		

		HUDWidget->AttachWidgetToRight(PlayerShopInventoryWidget);
		UE_LOG(LogFishingPC, Log, TEXT("✅ Player Shop Inventory widget created"));
	}
	
}
void AFishingPlayerController::CleanupShopWidgets()
{
	if (!HUDWidget)
	{
		return;
	}

	// Remove Shop Inventory (Left)
	if (ShopInventoryWidget)
	{
		if (CurrentShop && CurrentShop->GetShopInventory())
		{
			CurrentShop->GetShopInventory()->ClearInventoryWidget();
		}

		HUDWidget->ClearWidgetInLeft();
		ShopInventoryWidget->RemoveFromParent();
		ShopInventoryWidget = nullptr;
		UE_LOG(LogFishingPC, Log, TEXT("✅ Shop Inventory removed"));
	}

	// Remove Player Shop Inventory (Right)
	if (PlayerShopInventoryWidget)
	{
		HUDWidget->ClearWidgetInRight();
		PlayerShopInventoryWidget->RemoveFromParent();
		PlayerShopInventoryWidget = nullptr;
		UE_LOG(LogFishingPC, Log, TEXT("✅ Player Shop Inventory removed"));
	}

	// Remove Normal Inventory (Right)
	if (InventoryWidget)
	{
		HUDWidget->ClearWidgetInRight();
		InventoryWidget->RemoveFromParent();
		InventoryWidget = nullptr;
		UE_LOG(LogFishingPC, Log, TEXT("✅ Normal Inventory removed"));
	}
}