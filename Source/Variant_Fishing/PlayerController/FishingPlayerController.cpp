
#include "FishingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Fishing.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InteractableCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Variant_Fishing/Widget/InteractionWidget.h"
#include "Variant_Fishing/Widget/UHUDWidget.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "Variant_Fishing/Widget/Shop/ShopInventoryWidget.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/Data/ShopTransactionData.h"
#include "Variant_Fishing/NPC/ShopCharacter.h"
#include "Variant_Fishing/Interface/Interactable.h"
#include "Variant_Fishing/Widget/LeaderboardWidget.h"
#include "Variant_Fishing/Widget/PauseMenuWidget.h"

void AFishingPlayerController::BeginPlay()
{
    Super::BeginPlay();
    InitializeRenderTargetPool();
    
}


UItemBase* FindItemByGuid(UInventoryComponent* InventoryComp, const FGuid& ItemGuid)
{
    if (!InventoryComp)
    {
        return nullptr;
    }

    TMap<UItemBase*, FIntPoint> AllItems = InventoryComp->GetAllItems();
    for (auto& Pair : AllItems)
    {
        if (Pair.Key && Pair.Key->ItemGuid == ItemGuid)
        {
            return Pair.Key;
        }
    }

    return nullptr;
}


UInventoryComponent* GetInventoryFromActor(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    
    if (AFishingCharacter* FishChar = Cast<AFishingCharacter>(Actor))
    {
        return GetInventoryFromActor(FishChar);
    }

    
    if (AShopCharacter* ShopChar = Cast<AShopCharacter>(Actor))
    {
        return ShopChar->ShopInventoryComponent;
    }

    
    return Actor->FindComponentByClass<UInventoryComponent>();
}


void AFishingPlayerController::InitializeRenderTargetPool()
{
    if (!IsLocalController())
    {
        return;
    }

    const int32 PoolSize = 4;
    RenderTargetPool.Reserve(PoolSize);

    for (int32 i = 0; i < PoolSize; ++i)
    {
        UTextureRenderTarget2D* RT = UKismetRenderingLibrary::CreateRenderTarget2D(
            this,
            1024,
            2028,
            ETextureRenderTargetFormat::RTF_RGBA8,
            FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
            false
        );

        if (RT)
        {
            RT->bAutoGenerateMips = false;
            RT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
        }
        
        FRenderTargetPoolResource PoolResource;
        PoolResource.RenderTarget = RT;
        PoolResource.AlphaMaterial = UMaterialInstanceDynamic::Create(AlphaInvertMaterial, this);
        RenderTargetPool.Add(PoolResource);
        
        UE_LOG(LogFishingPC, Log, TEXT("✅ RenderTarget Pool[%d] created"), i);
    }
}


FRenderTargetPoolResource AFishingPlayerController::GetRenderTargetFromPool(int32 Index)
{
    if (RenderTargetPool.IsValidIndex(Index))
    {
        return RenderTargetPool[Index];
    }
    
    UE_LOG(LogFishingPC, Error, TEXT("Invalid RenderTarget pool index: %d"), Index);
    return FRenderTargetPoolResource();
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

    
    if (PauseMenuWidgetClass && !CachedPauseWidget)
    {
        CachedPauseWidget = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
        if (CachedPauseWidget)
        {
            CachedPauseWidget->AddToViewport(TopOrder);
            CachedPauseWidget->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogFishingPC, Log, TEXT("✅ Pause Menu Panel created"));
        }
        else
        {
            UE_LOG(LogFishingPC, Error, TEXT("❌ Pause Menu Panel failed to create"));
        }
    }

    if (LeaderBoardWidgetClass && !CachedLeaderBoardWidget)
    {
        CachedLeaderBoardWidget = CreateWidget<ULeaderboardWidget>(this, LeaderBoardWidgetClass);
        if (CachedLeaderBoardWidget)
        {
            CachedLeaderBoardWidget->AddToViewport(HighOrder);
            CachedLeaderBoardWidget->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogFishingPC, Log, TEXT("✅ LeaderBoard Panel created"));
        }
        else
        {
            UE_LOG(LogFishingPC, Error, TEXT("❌ LeaderBoard Panel failed to create"));
        }
    }

    
    if (HUDWidgetClass && !HUDWidget)
    {
        HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->SetPlayerName(PlayerNameForCharacterInit);
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





void AFishingPlayerController::TogglePauseMenu()
{
    if (!CachedPauseWidget)
    {
        UE_LOG(LogFishingPC, Warning, TEXT("ToggleSettings: SettingsPanel is null"));
        return;
    }

    
    if (IsInventoryOpen())
    {
        CloseInventory();
    }

    if (IsShopOpen())
    {
        CloseShopUI();
    }

    CachedPauseWidget->TogglePanel();

    UE_LOG(LogFishingPC, Log, TEXT("⚙️ Settings toggled: %s"), 
        CachedPauseWidget->IsPanelOpen() ? TEXT("OPEN") : TEXT("CLOSED"));
}

void AFishingPlayerController::ShowItemAcquiredBanner(UItemBase* Item)
{
    if(HUDWidget)
        HUDWidget->ShowItemAcquiredBanner(Item);
}





void AFishingPlayerController::ToggleInventory(UInventoryComponent* PlayerInventory)
{
    
    if (IsShopOpen())
    {
        CloseShopUI();
        return;
    }

    if (!PlayerInventory)
    {
        UE_LOG(LogFishingPC, Warning, TEXT("ToggleInventory: PlayerInventory is null"));
        return;
    }

    
    if (IsInventoryOpen())
    {
        CloseInventory();
    }
    else
    {
        OpenInventory(PlayerInventory);
    }
}

void AFishingPlayerController::ToggleLeaderBoard()
{
    if (!CachedLeaderBoardWidget)
    {
        UE_LOG(LogFishingPC, Warning, TEXT("ToggleSettings: SettingsPanel is null"));
        return;
    }
    
    CachedLeaderBoardWidget->ToggleMenu();

    UE_LOG(LogFishingPC, Log, TEXT("⚙️ Settings toggled: %s"), 
        CachedLeaderBoardWidget->IsOpen() ? TEXT("OPEN") : TEXT("CLOSED"));
}


void AFishingPlayerController::OpenInventory(UInventoryComponent* PlayerInventory)
{
    if (!PlayerInventory)
    {
        UE_LOG(LogFishingPC, Warning, TEXT("OpenInventory: PlayerInventory is null"));
        return;
    }

    if (IsInventoryOpen())
    {
        UE_LOG(LogFishingPC, Warning, TEXT("OpenInventory: Already open"));
        return;
    }

    
    if (IsShopOpen())
    {
        CloseShopUI();
    }

    
    ShowInventoryWidget(PlayerInventory);

    
    SetUIMode(true);
    HideInteractionPrompt();

    UE_LOG(LogFishingPC, Log, TEXT("📦 Inventory opened"));
}

void AFishingPlayerController::CloseInventory()
{
    if (!IsInventoryOpen())
    {
        return;
    }

    
    HideInventoryWidget();

    
    SetGameOnlyMode();

    UE_LOG(LogFishingPC, Log, TEXT("📦 Inventory closed"));
}

bool AFishingPlayerController::IsInventoryOpen() const
{
    return InventoryWidget && InventoryWidget->IsVisible();
}





void AFishingPlayerController::OpenShopUI(AShopCharacter* Shop, UInventoryComponent* PlayerInventory)
{
    if (!Shop || !Shop->GetShopInventory() || !PlayerInventory)
    {
        UE_LOG(LogFishingPC, Error, TEXT("OpenShopUI: Invalid parameters"));
        return;
    }

    if (IsShopOpen())
    {
        CloseShopUI();
    }

    if (IsInventoryOpen())
    {
        CloseInventory();
    }

    UE_LOG(LogFishingPC, Log, TEXT("🛒 Opening shop: '%s'"), *IInteractable::Execute_GetInteractableName(Shop));

    CurrentShop = Shop;

    
    ShopTransactionManager = NewObject<UShopTransactionManager>(this);
    ShopTransactionManager->Initialize(PlayerInventory, Shop->GetShopInventory());

    
    ShowShopWidgets(PlayerInventory, Shop->GetShopInventory());

    SetUIMode(true);
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

    if (IsShopOpen())
    {
        CloseShopUI();
    }

    if (IsInventoryOpen())
    {
        CloseInventory();
    }

    UE_LOG(LogFishingPC, Log, TEXT("🔄 Opening exchange UI: '%s'"), *IInteractable::Execute_GetInteractableName(Shop));

    CurrentShop = Shop;

    
    ShowExchangeWidgets(PlayerInventory, Shop->GetShopInventory());

    SetUIMode(true);
    HideInteractionPrompt();

    UE_LOG(LogFishingPC, Log, TEXT("✅ Exchange UI opened"));
}

void AFishingPlayerController::CloseShopUI()
{
    if (!IsShopOpen())
    {
        return;
    }

    UE_LOG(LogFishingPC, Log, TEXT("🛒 Closing shop: '%s'"), 
        CurrentShop ? *IInteractable::Execute_GetInteractableName(CurrentShop): TEXT("NULL"));

    
    if (ShopTransactionManager)
    {
        ShopTransactionManager->CancelAll();
        ShopTransactionManager = nullptr;
        UE_LOG(LogFishingPC, Log, TEXT("✅ All transactions cancelled"));
    }

    
    HideShopWidgets();

    CurrentShop = nullptr;

    SetGameOnlyMode();
    
    UE_LOG(LogFishingPC, Log, TEXT("✅ Shop UI closed"));
}





void AFishingPlayerController::ShowInteractionPrompt(const TScriptInterface<IInteractable>& Target)
{
    if (!InteractionWidget || !Target.GetObject())
    {
        return;
    }

    if (IsUIOpen())
    {
        return;
    }
    
    FString DisplayName = *IInteractable::Execute_GetInteractableName(Target.GetObject());
    
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





void AFishingPlayerController::SetUIMode(bool bEnableUI)
{
    if (bEnableUI)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
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





bool AFishingPlayerController::IsUIOpen() const
{
    if (IsInventoryOpen())
    {
        return true;
    }

    if (IsShopOpen())
    {
        return true;
    }

    if (CachedPauseWidget && CachedPauseWidget->IsPanelOpen())
    {
        return true;
    }

    return false;
}





int32 AFishingPlayerController::GetGoldSafe() const
{
    if (const AInteractableCharacter* MyCharacter = Cast<AInteractableCharacter>(GetPawn()))
    {
        return MyCharacter->GetGold();
    }
    return 0;
}

FString AFishingPlayerController::GetLocalTimeString() const
{
    const FDateTime Now = FDateTime::Now();
    return Now.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
}





UInventoryWidget* AFishingPlayerController::GetOrCreatePlayerInventoryWidget(UInventoryComponent* PlayerInventory)
{
    if (!InventoryWidget && InventoryWidgetClass)
    {
        FRenderTargetPoolResource PoolResource = GetRenderTargetFromPool(0);
        if (AInteractableCharacter* MyCharacter = Cast<AInteractableCharacter>(GetPawn()))
        {
            UTextureRenderTarget2D* RT = PoolResource.RenderTarget;
            UMaterialInstanceDynamic* AlphaMat = PoolResource.AlphaMaterial;
            if (!RT || !AlphaMat)
            {
                return nullptr;
            }
            
            MyCharacter->SetPortraitRenderTarget(RT);
            MyCharacter->SetPortraitCaptureEnabled(true);
            AlphaMat->SetTextureParameterValue(FName("PortraitTexture"), RT);
            UE_LOG(LogFishingPC, Log, TEXT("✅ Player RenderTarget[0] assigned and Material Set"));

            InventoryWidget = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
            
            if (InventoryWidget)
            {
                InventoryWidget->InitializeWidget(PlayerInventory, MyCharacter, AlphaMat);
                PlayerInventory->RegisterInventoryWidget(InventoryWidget);
            
                InventoryWidget->SetOwningPlayer(this);
                InventoryWidget->SetLabel(FText::FromString("Inventory"));
            
                UE_LOG(LogFishingPC, Log, TEXT("✅ Player Inventory widget created (init once)"));
            }
        }
    }
    
    return InventoryWidget;
}

UShopInventoryWidget* AFishingPlayerController::GetOrCreateLeftShopWidget()
{
    if (!ShopInventoryWidget && ShopInventoryWidgetClass)
    {
        ShopInventoryWidget = CreateWidget<UShopInventoryWidget>(GetWorld(), ShopInventoryWidgetClass);
        if (ShopInventoryWidget)
        {
            ShopInventoryWidget->bIsPlayerInventory = false;
            ShopInventoryWidget->SetDroppable(false);
            ShopInventoryWidget->SetLabel(FText::FromString("Shop"));
            
            UE_LOG(LogFishingPC, Log, TEXT("✅ Left Shop widget created (will re-init each time)"));
        }
    }
    
    return ShopInventoryWidget;
}

UShopInventoryWidget* AFishingPlayerController::GetOrCreateRightShopWidget(UInventoryComponent* PlayerInventory)
{
    if (!PlayerShopInventoryWidget && ShopInventoryWidgetClass)
    {
        FRenderTargetPoolResource PoolResource = GetRenderTargetFromPool(2);
        if (AInteractableCharacter* MyCharacter = Cast<AInteractableCharacter>(GetPawn()))
        {
            UTextureRenderTarget2D* RT = PoolResource.RenderTarget;
            UMaterialInstanceDynamic* AlphaMat = PoolResource.AlphaMaterial;
            if (!RT || !AlphaMat)
            {
                return nullptr;
            }
            
            MyCharacter->SetPortraitRenderTarget(RT);
            MyCharacter->SetPortraitCaptureEnabled(true);
            AlphaMat->SetTextureParameterValue(FName("PortraitTexture"), RT);
            UE_LOG(LogFishingPC, Log, TEXT("✅ Player Shop RenderTarget[2] assigned and Material Set"));

            PlayerShopInventoryWidget = CreateWidget<UShopInventoryWidget>(GetWorld(), ShopInventoryWidgetClass);
            
            if (PlayerShopInventoryWidget && PlayerInventory)
            {
                PlayerShopInventoryWidget->InitializeWidget(PlayerInventory, MyCharacter, AlphaMat);
                PlayerInventory->RegisterInventoryWidget(PlayerShopInventoryWidget);
                
                PlayerShopInventoryWidget->bIsPlayerInventory = true;
                PlayerShopInventoryWidget->SetDroppable(false);
                PlayerShopInventoryWidget->SetLabel(FText::FromString("Player"));
                
                UE_LOG(LogFishingPC, Log, TEXT("✅ Right Shop widget created (init once)"));
            }
        }
    }
    
    return PlayerShopInventoryWidget;
}





void AFishingPlayerController::ShowInventoryWidget(UInventoryComponent* PlayerInventory)
{
    if (!HUDWidget)
    {
        return;
    }

    UInventoryWidget* Widget = GetOrCreatePlayerInventoryWidget(PlayerInventory);
    if (!Widget)
    {
        return;
    }

    
    Widget->RefreshGrid();
    PlayerInventory->RefreshAllItems();
    
    HUDWidget->AttachWidgetToRight(Widget);
    Widget->SetVisibility(ESlateVisibility::Visible);
}

void AFishingPlayerController::HideInventoryWidget()
{
    if (!HUDWidget || !InventoryWidget)
    {
        return;
    }

    if (AInteractableCharacter* MyCharacter = Cast<AInteractableCharacter>(GetPawn()))
    {
        MyCharacter->SetPortraitCaptureEnabled(false);
        MyCharacter->SetPortraitRenderTarget(nullptr);
        
        UE_LOG(LogFishingPC, Log, TEXT("✅ Player RenderTarget released"));
    }

    HUDWidget->ClearWidgetInRight();
    InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AFishingPlayerController::ShowShopWidgets(UInventoryComponent* PlayerInventory, UInventoryComponent* ShopInventory)
{
    if (!HUDWidget || !CurrentShop)
    {
        return;
    }

    
    UShopInventoryWidget* ShopWidget = GetOrCreateLeftShopWidget();
    if (ShopWidget)
    {
        
        FRenderTargetPoolResource ShopPoolResource = GetRenderTargetFromPool(1);
        UTextureRenderTarget2D* ShopRT = ShopPoolResource.RenderTarget;
        UMaterialInstanceDynamic* ShopAlphaMat = ShopPoolResource.AlphaMaterial;
        
        if (ShopRT && ShopAlphaMat)
        {
            CurrentShop->SetPortraitRenderTarget(ShopRT);
            CurrentShop->SetPortraitCaptureEnabled(true);
            ShopAlphaMat->SetTextureParameterValue(FName("PortraitTexture"), ShopRT);
        }
        
        UInventoryComponent* PreviousInventory = ShopWidget->GetConnectedInventory();
        if (PreviousInventory)
        {
            PreviousInventory->ClearInventoryWidget();
        }
        
        ShopWidget->InitializeWidget(ShopInventory, CurrentShop, ShopAlphaMat);
        ShopInventory->RegisterInventoryWidget(ShopWidget);
        
        ShopWidget->SetTransactionManager(ShopTransactionManager);
        ShopWidget->RefreshGrid();
        ShopInventory->RefreshAllItems();
        
        HUDWidget->AttachWidgetToLeft(ShopWidget);
        ShopWidget->SetVisibility(ESlateVisibility::Visible);
    }

    
    UShopInventoryWidget* PlayerWidget = GetOrCreateRightShopWidget(PlayerInventory);
    if (PlayerWidget)
    {
        PlayerWidget->SetTransactionManager(ShopTransactionManager);
        PlayerWidget->RefreshGrid();
        PlayerInventory->RefreshAllItems();
        
        HUDWidget->AttachWidgetToRight(PlayerWidget);
        PlayerWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void AFishingPlayerController::ShowExchangeWidgets(UInventoryComponent* PlayerInventory, UInventoryComponent* OpponentPlayerInventory)
{
    if (!HUDWidget || !CurrentShop)
    {
        return;
    }

    
}

void AFishingPlayerController::CloseExchangeWidget()
{
}

void AFishingPlayerController::HideShopWidgets()
{
    if (!HUDWidget)
    {
        return;
    }

    
    if (CurrentShop)
    {
        CurrentShop->SetPortraitCaptureEnabled(false);
        CurrentShop->SetPortraitRenderTarget(nullptr);
        
        UE_LOG(LogFishingPC, Log, TEXT("✅ Shop RenderTarget released"));
    }

    
    if (ShopInventoryWidget)
    {
        UInventoryComponent* ConnectedInventory = ShopInventoryWidget->GetConnectedInventory();
        if (ConnectedInventory)
        {
            ConnectedInventory->ClearInventoryWidget();
        }
        
        HUDWidget->ClearWidgetInLeft();
        ShopInventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
    }

    
    if (PlayerShopInventoryWidget)
    {
        if (AInteractableCharacter* MyCharacter = Cast<AInteractableCharacter>(GetPawn()))
        {
            MyCharacter->SetPortraitCaptureEnabled(false);
            MyCharacter->SetPortraitRenderTarget(nullptr);
            
            UE_LOG(LogFishingPC, Log, TEXT("✅ Player Shop RenderTarget released"));
        }
        
        HUDWidget->ClearWidgetInRight();
        PlayerShopInventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
    }

    
    HideInventoryWidget();
}