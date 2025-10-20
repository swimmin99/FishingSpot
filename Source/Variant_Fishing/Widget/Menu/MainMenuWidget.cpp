
#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "SaveSlotItemWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Fishing.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Variant_Fishing/Widget/BaseButtonWidget.h"
#include "Variant_Fishing/Database/DatabaseManager.h"
#include "Variant_Fishing/GameInstance/FishingGameInstance.h"

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    
    if (NewGameButton)
    {
        NewGameButton->SetText(FText::FromString(TEXT("New Game")));
        NewGameButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnNewGameClicked);
    }

    if (LoadGameButton)
    {
        LoadGameButton->SetText(FText::FromString(TEXT("Load Game")));
        LoadGameButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnLoadGameClicked);
    }

    if (JoinGameButton)
    {
        JoinGameButton->SetText(FText::FromString(TEXT("Join Game")));
        JoinGameButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinGameClicked);
    }

    if (OptionsButton)
    {
        OptionsButton->SetText(FText::FromString(TEXT("Options")));
        OptionsButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnOptionsClicked);
    }

    if (ExitButton)
    {
        ExitButton->SetText(FText::FromString(TEXT("Exit")));
        ExitButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnExitClicked);
    }

    if (CreateGameButton)
    {
        CreateGameButton->GetBaseButton()->SetIsEnabled(true);
        CreateGameButton->SetText(FText::FromString(TEXT("Create")));
        CreateGameButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCreateGameClicked);
    }

    if (JoinServerButton)
    {
        JoinServerButton->GetBaseButton()->SetIsEnabled(true);
        JoinServerButton->SetText(FText::FromString(TEXT("Join")));
        JoinServerButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinServerClicked);
    }

    
    if (BackFromLoadButton)
    {
        BackFromLoadButton->SetText(FText::FromString(TEXT("Back")));
        BackFromLoadButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackClicked);
    }

    if (BackFromNewGameButton)
    {
        BackFromNewGameButton->SetText(FText::FromString(TEXT("Back")));
        BackFromNewGameButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackClicked);
    }

    if (BackFromJoinButton)
    {
        BackFromJoinButton->SetText(FText::FromString(TEXT("Back")));
        BackFromJoinButton->GetBaseButton()->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackClicked);
    }

    
    SwitchToPanel(0);
}





void UMainMenuWidget::OnNewGameClicked()
{
    
    SwitchToPanel(2);

    
    if (PlayerNameInput)
    {
        PlayerNameInput->SetText(FText::FromString(TEXT("Player")));
    }

    if (VillageNameInput)
    {
        VillageNameInput->SetText(FText::FromString(TEXT("MyVillage")));
    }
}

void UMainMenuWidget::OnLoadGameClicked()
{
    
    RefreshSaveSlots();
    SwitchToPanel(1);
}

void UMainMenuWidget::OnJoinGameClicked()
{
    
    SwitchToPanel(3);

    
    if (ServerIPInput)
    {
        ServerIPInput->SetText(FText::FromString(TEXT("127.0.0.1")));
    }
    
    if (JoinPlayerNameInput)
    {
        JoinPlayerNameInput->SetText(FText::FromString(TEXT("Guest")));
    }
}

void UMainMenuWidget::OnOptionsClicked()
{
    
    UE_LOG(MenuWidget, Log, TEXT("Options clicked - Not implemented yet"));
}

void UMainMenuWidget::OnExitClicked()
{
    
    if (APlayerController* PC = GetOwningPlayer())
    {
        UKismetSystemLibrary::QuitGame(
            this,
            PC,
            EQuitPreference::Quit,
            false
        );
    }
}

void UMainMenuWidget::OnBackClicked()
{
    
    SwitchToPanel(0);
}





void UMainMenuWidget::RefreshSaveSlots()
{
    if (!SaveSlotList)
    {
        UE_LOG(MenuWidget, Warning, TEXT("SaveSlotList is null!"));
        return;
    }

    
    SaveSlotList->ClearChildren();

    
    UDatabaseManager* DB = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DB)
    {
        UE_LOG(MenuWidget, Error, TEXT("DatabaseManager not found!"));
        return;
    }

    
    CachedSaveSlots = DB->GetAllSaveSlots();

    if (CachedSaveSlots.Num() == 0)
    {
        UE_LOG(MenuWidget, Log, TEXT("No save slots found"));
        
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 
                3.0f, 
                FColor::Yellow, 
                TEXT("No save files found. Create a new game!")
            );
        }
        return;
    }

    
    for (const FSaveSlotInfo& SlotInfo : CachedSaveSlots)
    {
        if (!SaveSlotItemClass)
        {
            UE_LOG(MenuWidget, Error, TEXT("SaveSlotItemClass not set!"));
            continue;
        }

        USaveSlotItemWidget* SlotWidget = CreateWidget<USaveSlotItemWidget>(
            this,
            SaveSlotItemClass
        );

        if (SlotWidget)
        {
            SlotWidget->SetSaveSlotInfo(SlotInfo);
            SlotWidget->OnSlotSelected.AddDynamic(this, &UMainMenuWidget::SelectSaveSlot);
            SlotWidget->OnSlotDeleted.AddDynamic(this, &UMainMenuWidget::DeleteSaveSlot);
            
            SaveSlotList->AddChild(SlotWidget);
            
            UE_LOG(MenuWidget, Verbose, TEXT("Added save slot: %s (%s)"), 
                *SlotInfo.PlayerName, *SlotInfo.VillageName);
        }
    }

    UE_LOG(MenuWidget, Log, TEXT("✅ Refreshed %d save slots"), CachedSaveSlots.Num());
}

void UMainMenuWidget::DeleteSaveSlot(const FString& VillageName)
{
    
    UDatabaseManager* DB = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DB)
    {
        UE_LOG(MenuWidget, Error, TEXT("DatabaseManager not found!"));
        return;
    }

    
    

    
    if (DB->DeleteSaveSlot(VillageName))
    {
        UE_LOG(MenuWidget, Log, TEXT("✅ Deleted save slot: %s"), *VillageName);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 
                3.0f, 
                FColor::Green, 
                FString::Printf(TEXT("Deleted: %s"), *VillageName)
            );
        }
        
        
        RefreshSaveSlots();
    }
    else
    {
        UE_LOG(MenuWidget, Error, TEXT("❌ Failed to delete save slot: %s"), *VillageName);
    }
}

void UMainMenuWidget::SwitchToPanel(int32 Index)
{
    if (MenuSwitcher)
    {
        MenuSwitcher->SetActiveWidgetIndex(Index);
        
        UE_LOG(MenuWidget, Verbose, TEXT("Switched to panel: %d"), Index);
    }
}



void UMainMenuWidget::OnCreateGameClicked()
{
    if (!PlayerNameInput || !VillageNameInput)
    {
        return;
    }

    FString PlayerName = PlayerNameInput->GetText().ToString();
    FString VillageName = VillageNameInput->GetText().ToString();

    
    if (PlayerName.IsEmpty() || VillageName.IsEmpty())
    {
        UE_LOG(MenuWidget, Warning, TEXT("Player name and village name required!"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 
                3.0f, 
                FColor::Red, 
                TEXT("Please enter both Player Name and Village Name!")
            );
        }
        return;
    }

    
    UDatabaseManager* DB = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DB)
    {
        UE_LOG(MenuWidget, Error, TEXT("DatabaseManager not found!"));
        return;
    }

    
    int32 DatabasePlayerID = DB->FindOrCreateSaveSlot(PlayerName, VillageName);
    if (DatabasePlayerID == -1)
    {
        UE_LOG(MenuWidget, Error, TEXT("Failed to create save slot!"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 
                5.0f, 
                FColor::Red, 
                TEXT("Failed to create save slot! Village name might already exist.")
            );
        }
        return;
    }

    
    DB->SetActivePlayer(DatabasePlayerID);

    
    if (UFishingGameInstance* FishingGI = Cast<UFishingGameInstance>(GetGameInstance()))
    {
        FishingGI->SetPendingPlayerName(PlayerName);
    }
    
    
    FString Options = FString::Printf(TEXT("?Name=%s"), *PlayerName);
    
    UE_LOG(MenuWidget, Log, TEXT("✅ Starting new game: %s (%s) - DatabasePlayerID=%d"), 
        *PlayerName, *VillageName, DatabasePlayerID);
    
    
    UGameplayStatics::OpenLevel(
        this, 
        MainGameLevelName,
        true,
        TEXT("listen") + Options
    );
}

void UMainMenuWidget::SelectSaveSlot(int32 DatabasePlayerID)
{
    
    UDatabaseManager* DB = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DB)
    {
        UE_LOG(MenuWidget, Error, TEXT("DatabaseManager not found!"));
        return;
    }

    
    DB->SetActivePlayer(DatabasePlayerID);

    
    FString PlayerName, VillageName;
    int32 Money;
    if (!DB->LoadPlayerData(DatabasePlayerID, PlayerName, VillageName, Money))
    {
        UE_LOG(MenuWidget, Error, TEXT("Failed to load player data for DatabasePlayerID=%d"), DatabasePlayerID);
        return;
    }

    
    if (UFishingGameInstance* FishingGI = Cast<UFishingGameInstance>(GetGameInstance()))
    {
        UE_LOG(MenuWidget, Log, TEXT("Saved PlayerName data in GameInstance for DatabasePlayerID=%d, %s"), DatabasePlayerID, *PlayerName);
        FishingGI->SetPendingPlayerName(PlayerName);
    }

    
    FString Options = FString::Printf(TEXT("?Name=%s"), *PlayerName);

    
    UE_LOG(MenuWidget, Log, TEXT("✅ Loading game: DatabasePlayerID=%d, Name=%s"), 
        DatabasePlayerID, *PlayerName);
    
    UGameplayStatics::OpenLevel(
        this, 
        MainGameLevelName,
        true,
        TEXT("listen") + Options
    );
}

void UMainMenuWidget::OnJoinServerClicked()
{
    if (!ServerIPInput || !JoinPlayerNameInput)
    {
        return;
    }

    const FString ServerIP = ServerIPInput->GetText().ToString();
    const FString PlayerName = JoinPlayerNameInput->GetText().ToString();

    if (ServerIP.IsEmpty() || PlayerName.IsEmpty())
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 
                3.0f, 
                FColor::Red, 
                TEXT("Please enter both Server IP and Player Name!")
            );
        }
        return;
    }

    
    if (UFishingGameInstance* FishingGI = Cast<UFishingGameInstance>(GetGameInstance()))
    {
        FishingGI->SetPendingPlayerName(PlayerName);
    }

    
    const FString TravelURL = FString::Printf(TEXT("%s?Name=%s"), *ServerIP, *PlayerName);

    UE_LOG(MenuWidget, Log, TEXT("✅ Joining server: %s as %s"), *ServerIP, *PlayerName);

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->ClientTravel(TravelURL, TRAVEL_Absolute);
    }
}