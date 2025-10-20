
#include "FishingGameMode.h"
#include "Variant_Fishing/Database/DatabaseManager.h"
#include "FishingCharacter.h"
#include "FishingGameState.h"
#include "FishingPlayerState.h"
#include "Fishing.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/GameInstance/FishingGameInstance.h"

AFishingGameMode::AFishingGameMode()
{
    PrimaryActorTick.bCanEverTick = false;
    GameStateClass = AFishingGameState::StaticClass();
    
    
    PlayerStateClass = AFishingPlayerState::StaticClass();
}

void AFishingGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    if (!HasAuthority())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("FishingGameMode: Not authority, skipping save system"));
        return;
    }

    DatabaseManager = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    UE_LOG(FishingGameMode, Log, TEXT("=== FishingGameMode: BeginPlay ==="));

    
    InitializeSession();

    
    if (bAutoLoadOnStart)
    {
        FTimerHandle LoadDelayTimer;
        GetWorldTimerManager().SetTimer(LoadDelayTimer, this, &AFishingGameMode::LoadGame, 1.0f, false);
    }

    
    StartAutoSaveTimer();
    
}

void AFishingGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(FishingGameMode, Log, TEXT("=== FishingGameMode: EndPlay (Reason=%d) ==="), 
        static_cast<int32>(EndPlayReason));

    StopAutoSaveTimer();
    
    Super::EndPlay(EndPlayReason);
}

void AFishingGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer)
    {
        return;
    }

    if (!HasAuthority())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("❌ PostLogin: Not authority!"));
        return;
    }

    if (!NewPlayer->IsLocalPlayerController())
    {
        UE_LOG(FishingGameMode, Log, TEXT("PostLogin: Not Locally Controlled!"));

        return;
    }
    
    
    UDatabaseManager* DB = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DB)
    {
        UE_LOG(FishingGameMode, Error, TEXT("❌ PostLogin: DatabaseManager not found!"));
        return;
    }

    AFishingPlayerState* PS = NewPlayer->GetPlayerState<AFishingPlayerState>();
    if (!PS)
    {
        UE_LOG(FishingGameMode, Error, TEXT("❌ PostLogin: PlayerState is not AFishingPlayerState!"));
        return;
    }

    UE_LOG(FishingGameMode, Warning, TEXT("PostLogin: %s, has passed the test!"), *PS->GetPlayerName());
    

    UE_LOG(FishingGameMode, Warning, TEXT("PostLogin: %s, is varified as a HOST!"), *PS->GetPlayerName());

        
        
        
        
        int32 DBPlayerID = DB->GetActivePlayerID();
        
        if (DBPlayerID == -1)
        {
            UE_LOG(FishingGameMode, Error, TEXT("❌ Host DatabasePlayerID not set!"));
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
                    TEXT("❌ Database Error: No active player!"));
            }
            
            FTimerHandle ReturnTimer;
            GetWorldTimerManager().SetTimer(ReturnTimer, [this, NewPlayer]()
            {
                NewPlayer->ClientReturnToMainMenuWithTextReason(
                    FText::FromString(TEXT("Database initialization failed"))
                );
            }, 2.0f, false);
            
            return;
        }

        
        FString PlayerName;
        if (UFishingGameInstance* FishingGI = Cast<UFishingGameInstance>(GetGameInstance()))
        {
            PlayerName = FishingGI->GetAndClearPendingPlayerName();
        }

        if (PlayerName.IsEmpty())
        {
            FString DBPlayerName, VillageName;
            int32 Money;
            if (DB->LoadPlayerData(DBPlayerID, DBPlayerName, VillageName, Money))
            {
                PlayerName = DBPlayerName;
            }
        }

        if (PlayerName.IsEmpty())
        {
            PlayerName = TEXT("Host");
        }

        
        NewPlayer->PlayerState->SetPlayerName(PlayerName);
        PS->InitializeAsHost(DBPlayerID);
        
        
        CachedHostDatabasePlayerID = DBPlayerID;
        HostDatabasePlayerID = DBPlayerID;
        
        
        PlayerDatabaseIDMap.Add(NewPlayer, DBPlayerID);

        UE_LOG(FishingGameMode, Log, TEXT("✅ Host logged in: DatabasePlayerID=%d, Name=%s"),
            DBPlayerID, *PlayerName);
            
        
        if (AFishingGameState* FishingGS = GetFishingGameState())
        {
            FishingGS->SetHostPlayerID(DBPlayerID);
            FishingGS->UpdateSessionPlayerCount(1);
        }
}

void AFishingGameMode::ServerReceivePlayerName(APlayerController* PC, const FString& PlayerName)
{
    if (!PC || !HasAuthority())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("ServerReceivePlayerName: Invalid call"));
        return;
    }

    if (PlayerName.IsEmpty())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("ServerReceivePlayerName: Empty PlayerName"));
        return;
    }

    UE_LOG(FishingGameMode, Log, TEXT("✅ ServerReceivePlayerName: Received '%s' from client"), *PlayerName);

    
    UDatabaseManager* DB = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DB)
    {
        UE_LOG(FishingGameMode, Error, TEXT("ServerReceivePlayerName: DatabaseManager not found"));
        return;
    }

    
    AFishingPlayerState* PS = PC->GetPlayerState<AFishingPlayerState>();
    if (!PS)
    {
        UE_LOG(FishingGameMode, Error, TEXT("ServerReceivePlayerName: PlayerState not found"));
        return;
    }

    
    PC->PlayerState->SetPlayerName(PlayerName);

    
    int32 GuestDBPlayerID = DB->FindSessionPlayer(CachedHostDatabasePlayerID, PlayerName);

    if (GuestDBPlayerID == -1)
    {
        
        GuestDBPlayerID = DB->CreateSessionPlayer(CachedHostDatabasePlayerID, PlayerName);
        
        if (GuestDBPlayerID == -1)
        {
            UE_LOG(FishingGameMode, Error, TEXT("❌ Failed to create session player: %s"), *PlayerName);
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
                    TEXT("❌ Failed to create player data!"));
            }
            
            return;
        }

        UE_LOG(FishingGameMode, Log, TEXT("✅ New guest created: %s (DatabasePlayerID=%d)"), 
            *PlayerName, GuestDBPlayerID);
    }
    else
    {
        UE_LOG(FishingGameMode, Log, TEXT("✅ Returning guest: %s (DatabasePlayerID=%d)"), 
            *PlayerName, GuestDBPlayerID);
    }

    
    PS->InitializeAsGuest(GuestDBPlayerID, CachedHostDatabasePlayerID);
    
    
    PlayerDatabaseIDMap.Add(PC, GuestDBPlayerID);
    
    
    if (AFishingGameState* FishingGS = GetFishingGameState())
    {
        int32 CurrentCount = GetWorld()->GetNumPlayerControllers();
        FishingGS->UpdateSessionPlayerCount(CurrentCount);
    }
    
    
    if (bAutoLoadOnJoin)
    {
        FTimerHandle LoadTimer;
        GetWorldTimerManager().SetTimer(LoadTimer, [this, PC, GuestDBPlayerID]()
        {
            LoadPlayerDataDelayed(PC, GuestDBPlayerID);
        }, 2.0f, false);
    }

    
    
    UE_LOG(FishingGameMode, Log, TEXT("✅ Guest setup complete: %s"), *PlayerName);
}


void AFishingGameMode::Logout(AController* Exiting)
{
    
    Super::Logout(Exiting);
}





void AFishingGameMode::InitializeSaveSystem()
{
    DatabaseManager = GetGameInstance()->GetSubsystem<UDatabaseManager>();
    
    if (!DatabaseManager)
    {
        UE_LOG(FishingGameMode, Error, TEXT("FishingGameMode: DatabaseManager not found!"));
        return;
    }

    int32 PlayerID = DatabaseManager->GetActivePlayerID();
    if (PlayerID == -1)
    {
        UE_LOG(FishingGameMode, Warning, TEXT("FishingGameMode: No active player set!"));
        UE_LOG(FishingGameMode, Warning, TEXT("Please set active player in MainMenu before starting game"));
    }
    else
    {
        UE_LOG(FishingGameMode, Log, TEXT("FishingGameMode: Active PlayerID = %d"), PlayerID);
    }
}

void AFishingGameMode::InitializeSession()
{
    if (!DatabaseManager)
    {
        return;
    }

    
    HostDatabasePlayerID = DatabaseManager->GetActivePlayerID();
    
    if (HostDatabasePlayerID == -1)
    {
        UE_LOG(FishingGameMode, Warning, TEXT("InitializeSession: No active player set!"));
        return;
    }

    
    if (AFishingGameState* FishingGS = GetFishingGameState())
    {
        FishingGS->SetHostPlayerID(HostDatabasePlayerID);
        FishingGS->UpdateSessionPlayerCount(1);
    }

    UE_LOG(FishingGameMode, Log, TEXT("✅ InitializeSession: HostDatabasePlayerID = %d"), HostDatabasePlayerID);
}

void AFishingGameMode::StartAutoSaveTimer()
{
    if (AutoSaveInterval <= 0.0f)
    {
        UE_LOG(FishingGameMode, Warning, TEXT("FishingGameMode: AutoSave disabled (interval <= 0)"));
        return;
    }

    GetWorldTimerManager().SetTimer(
        AutoSaveTimerHandle,
        this,
        &AFishingGameMode::AutoSave,
        AutoSaveInterval,
        true
    );

    UE_LOG(FishingGameMode, Log, TEXT("FishingGameMode: AutoSave timer started (%.0f seconds)"), 
        AutoSaveInterval);
}

void AFishingGameMode::StopAutoSaveTimer()
{
    if (AutoSaveTimerHandle.IsValid())
    {
        GetWorldTimerManager().ClearTimer(AutoSaveTimerHandle);
        UE_LOG(FishingGameMode, Log, TEXT("FishingGameMode: AutoSave timer stopped"));
    }
}

void AFishingGameMode::ReturnToMainMenu()
{
    
    APlayerController* FirstPC = GetWorld()->GetFirstPlayerController();
    if (!FirstPC || !IsPlayerHost(FirstPC))
    {
        UE_LOG(FishingGameMode, Warning, TEXT("ReturnToMainMenu: Only host can return to main menu!"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 
                3.0f, 
                FColor::Red, 
                TEXT("❌ Only the host can return to main menu!")
            );
        }
        return;
    }

    if (!HasAuthority())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("ReturnToMainMenu: Not authority!"));
        return;
    }

    UE_LOG(FishingGameMode, Log, TEXT("=== ReturnToMainMenu: START ==="));

    
    UE_LOG(FishingGameMode, Log, TEXT("Step 1: Saving all player data..."));
    SaveGame();

    
    UE_LOG(FishingGameMode, Log, TEXT("Step 2: Kicking all guests..."));
    KickAllGuests();

    
    FTimerHandle DelayTimer;
    GetWorldTimerManager().SetTimer(
        DelayTimer,
        [this, FirstPC]()
        {
            UE_LOG(FishingGameMode, Log, TEXT("Step 3: Traveling to MainMenu..."));
            UGameplayStatics::OpenLevel(this, TEXT("MainMenu"), false);
        },
        1.0f,
        false
    );

    UE_LOG(FishingGameMode, Log, TEXT("=== ReturnToMainMenu: Initiated ==="));
}





int32 AFishingGameMode::GetOrCreateSessionPlayerID(APlayerController* PC)
{
    if (!PC || !DatabaseManager)
    {
        return -1;
    }

    
    if (int32* ExistingID = PlayerDatabaseIDMap.Find(PC))
    {
        return *ExistingID;
    }

    
    FString PlayerName = PC->PlayerState ? PC->PlayerState->GetPlayerName() : TEXT("Player");

    
    if (IsPlayerHost(PC))
    {
        
        PlayerDatabaseIDMap.Add(PC, HostDatabasePlayerID);
        UE_LOG(FishingGameMode, Log, TEXT("✅ GetOrCreateSessionPlayerID: Host (DatabasePlayerID=%d)"), 
               HostDatabasePlayerID);
        return HostDatabasePlayerID;
    }

    
    int32 DBPlayerID = DatabaseManager->FindSessionPlayer(HostDatabasePlayerID, PlayerName);
    
    if (DBPlayerID == -1)
    {
        
        DBPlayerID = DatabaseManager->CreateSessionPlayer(HostDatabasePlayerID, PlayerName);
        
        if (DBPlayerID != -1)
        {
            UE_LOG(FishingGameMode, Log, TEXT("✅ New session player: %s (DatabasePlayerID=%d)"),
                   *PlayerName, DBPlayerID);
        }
    }
    else
    {
        UE_LOG(FishingGameMode, Log, TEXT("✅ Returning session player: %s (DatabasePlayerID=%d)"),
               *PlayerName, DBPlayerID);
    }

    
    if (DBPlayerID != -1)
    {
        PlayerDatabaseIDMap.Add(PC, DBPlayerID);
    }

    return DBPlayerID;
}





void AFishingGameMode::RemovePlayerID(APlayerController* PC)
{
    if (PC && PlayerDatabaseIDMap.Contains(PC))
    {
        int32 DBPlayerID = PlayerDatabaseIDMap[PC];
        PlayerDatabaseIDMap.Remove(PC);
        UE_LOG(FishingGameMode, Log, TEXT("RemovePlayerID: Removed DatabasePlayerID=%d"), DBPlayerID);
    }
}

bool AFishingGameMode::IsPlayerHost(APlayerController* PC) const
{
    return PC == GetWorld()->GetFirstPlayerController();
}





void AFishingGameMode::SaveGame()
{
    
    APlayerController* FirstPC = GetWorld()->GetFirstPlayerController();
    if (!FirstPC)
    {
        UE_LOG(FishingGameMode, Warning, TEXT("SaveGame: PC not found!"));
        return;
    }

    if (!IsPlayerHost(FirstPC))
    {
        UE_LOG(FishingGameMode, Warning, TEXT("SaveGame: Only host can save!"));
        return;
    }

    if (!HasAuthority())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("SaveGame: Not authority!"));
        return;
    }

    if (!DatabaseManager)
    {
        UE_LOG(FishingGameMode, Error, TEXT("SaveGame: DatabaseManager is null!"));
        return;
    }

    if (HostDatabasePlayerID == -1)
    {
        UE_LOG(FishingGameMode, Error, TEXT("SaveGame: No host PlayerID!"));
        return;
    }

    UE_LOG(FishingGameMode, Log, TEXT("=== SaveGame: START (Host PlayerID=%d) ==="), HostDatabasePlayerID);

    
    SavePlayerData();

    
    SaveInventories();

    

    UE_LOG(FishingGameMode, Log, TEXT("=== SaveGame: COMPLETE ==="));

    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 
            3.0f, 
            FColor::Green, 
            TEXT("✅ Game Saved! (All Players)")
        );
    }
}

void AFishingGameMode::EndSessionAndReturnToMainMenu()
{
    
    APlayerController* FirstPC = GetWorld()->GetFirstPlayerController();
    if (!FirstPC || !IsPlayerHost(FirstPC))
    {
        UE_LOG(FishingGameMode, Warning, TEXT("EndSession: Only host can end session!"));
        return;
    }

    if (!HasAuthority())
    {
        return;
    }

    UE_LOG(FishingGameMode, Log, TEXT("=== EndSessionAndReturnToMainMenu: START ==="));

    
    UE_LOG(FishingGameMode, Log, TEXT("Step 1: Saving all players..."));
    SaveGame();

    
    UE_LOG(FishingGameMode, Log, TEXT("Step 2: Kicking guests..."));
    KickAllGuests();

    
    FTimerHandle DelayTimer;
    GetWorldTimerManager().SetTimer(
        DelayTimer,
        [this]()
        {
            UE_LOG(FishingGameMode, Log, TEXT("Step 3: Host traveling to MainMenu..."));
            UGameplayStatics::OpenLevel(this, TEXT("MainMenu"), false);
        },
        1.0f,
        false
    );

    UE_LOG(FishingGameMode, Log, TEXT("=== EndSessionAndReturnToMainMenu: Complete ==="));
}

AFishingGameState* AFishingGameMode::GetFishingGameState() const
{
    return Cast<AFishingGameState>(GameState);
}

void AFishingGameMode::KickAllGuests()
{
    if (!HasAuthority())
    {
        return;
    }

    TArray<APlayerController*> GuestsToKick;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && !IsPlayerHost(PC))
        {
            GuestsToKick.Add(PC);
        }
    }

    for (APlayerController* GuestPC : GuestsToKick)
    {
        if (GuestPC)
        {
            FString PlayerName = GuestPC->PlayerState ? 
                GuestPC->PlayerState->GetPlayerName() : TEXT("Guest");

            UE_LOG(FishingGameMode, Log, TEXT("Kicking guest: %s"), *PlayerName);
            
            GuestPC->ClientReturnToMainMenuWithTextReason(KickMessage);
        }
    }

    if (GuestsToKick.Num() > 0)
    {
        UE_LOG(FishingGameMode, Log, TEXT("✅ Kicked %d guests"), GuestsToKick.Num());
    }
}

void AFishingGameMode::SavePlayerByController(APlayerController* PC)
{
    if (!PC || !HasAuthority())
    {
        return;
    }

    AFishingPlayerState* MyPlayerState = Cast<AFishingPlayerState>(PC->GetPlayerState<APlayerState>());
    const int32 PlayerID = MyPlayerState->DatabasePlayerID;

    if (PlayerID == -1)
    {
        UE_LOG(FishingGameMode, Warning, TEXT("SavePlayerByController: Invalid PlayerID"));
        return;
    }

    SavePlayerDataForID(PC, PlayerID);
    
    UE_LOG(FishingGameMode, Log, TEXT("✅ SavePlayerByController: Saved PlayerID=%d"), PlayerID);
}

void AFishingGameMode::LoadGame()
{
    
    APlayerController* FirstPC = GetWorld()->GetFirstPlayerController();
    if (!FirstPC || !IsPlayerHost(FirstPC))
    {
        UE_LOG(FishingGameMode, Warning, TEXT("LoadGame: Only host can load!"));
        return;
    }

    if (!HasAuthority())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("LoadGame: Not authority!"));
        return;
    }

    if (!DatabaseManager)
    {
        UE_LOG(FishingGameMode, Error, TEXT("LoadGame: DatabaseManager is null!"));
        return;
    }

    if (HostDatabasePlayerID == -1)
    {
        UE_LOG(FishingGameMode, Warning, TEXT("LoadGame: No host PlayerID, starting new game"));
        return;
    }

    UE_LOG(FishingGameMode, Log, TEXT("=== LoadGame: START (Host PlayerID=%d) ==="), HostDatabasePlayerID);

    
    LoadPlayerData();

    
    LoadInventories();

    
    LoadFishRecords();

    UE_LOG(FishingGameMode, Log, TEXT("=== LoadGame: COMPLETE ==="));

    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 
            3.0f, 
            FColor::Cyan, 
            TEXT("✅ Game Loaded! (All Players)")
        );
    }
}

void AFishingGameMode::AutoSave()
{
    UE_LOG(FishingGameMode, Log, TEXT("💾 AutoSave triggered"));
    SaveGame();
}

void AFishingGameMode::LoadPlayerDataDelayed(APlayerController* PC, int32 PlayerID)
{
    if (!PC || !PC->GetPawn())
    {
        UE_LOG(FishingGameMode, Warning, TEXT("LoadPlayerDataDelayed: Invalid player or no pawn"));
        return;
    }

    AFishingCharacter* Character = Cast<AFishingCharacter>(PC->GetPawn());
    if (!Character)
    {
        return;
    }

    
    UInventoryComponent* Inventory = Character->FindComponentByClass<UInventoryComponent>();
    if (Inventory)
    {
        Inventory->LoadInventoryFromDatabase(PlayerID);
    }

    
    FString PlayerName, VillageName;
    int32 Money;
    if (DatabaseManager && DatabaseManager->LoadPlayerData(PlayerID, PlayerName, VillageName, Money))
    {
        Character->SetGold(Money);
        UE_LOG(FishingGameMode, Log, TEXT("✅ LoadPlayerDataDelayed: Loaded data for PlayerID=%d"), PlayerID);
    }
}

void AFishingGameMode::SavePlayerDataForID(APlayerController* PC, int32 PlayerID)
{
    if (!PC || !PC->GetPawn() || !DatabaseManager)
    {
        return;
    }

    AFishingCharacter* Character = Cast<AFishingCharacter>(PC->GetPawn());
    if (!Character)
    {
        return;
    }

    
    UInventoryComponent* Inventory = Character->FindComponentByClass<UInventoryComponent>();
    if (Inventory)
    {
        Inventory->SaveInventoryToDatabase(PlayerID);
    }

    
    DatabaseManager->SavePlayerMoney(PlayerID, Character->GetGold());
}





void AFishingGameMode::SavePlayerData()
{
    if (!DatabaseManager)
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC || !PC->GetPawn())
        {
            continue;
        }

        AFishingCharacter* Character = Cast<AFishingCharacter>(PC->GetPawn());
        if (!Character)
        {
            continue;
        }

        int32 PlayerID = GetOrCreateSessionPlayerID(PC);
        if (PlayerID == -1)
        {
            continue;
        }

        
        int32 Gold = Character->GetGold();
        DatabaseManager->SavePlayerMoney(PlayerID, Gold);
        
        UE_LOG(FishingGameMode, Log, TEXT("SavePlayerData: Saved Gold=%d for PlayerID=%d"), Gold, PlayerID);
    }
}

void AFishingGameMode::LoadPlayerData()
{
    if (!DatabaseManager)
    {
        return;
    }

    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC || !PC->GetPawn())
        {
            continue;
        }

        AFishingCharacter* Character = Cast<AFishingCharacter>(PC->GetPawn());
        if (!Character)
        {
            continue;
        }

        int32 PlayerID = GetOrCreateSessionPlayerID(PC);
        if (PlayerID == -1)
        {
            continue;
        }

        FString PlayerName, VillageName;
        int32 Money;

        if (DatabaseManager->LoadPlayerData(PlayerID, PlayerName, VillageName, Money))
        {
            Character->SetGold(Money);
            UE_LOG(FishingGameMode, Log, TEXT("LoadPlayerData: Loaded Money=%d for PlayerID=%d"), 
                Money, PlayerID);
        }
    }
}

void AFishingGameMode::SaveInventories()
{
    if (!DatabaseManager)
    {
        UE_LOG(FishingGameMode, Error, TEXT("SaveInventories: DatabaseManager is null!"));
        return;
    }

    int32 TotalSaved = 0;

    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC || !PC->GetPawn())
        {
            continue;
        }

        AFishingCharacter* Character = Cast<AFishingCharacter>(PC->GetPawn());
        if (!Character)
        {
            continue;
        }

        
        int32 PlayerID = GetOrCreateSessionPlayerID(PC);
        if (PlayerID == -1)
        {
            continue;
        }

        
        UInventoryComponent* Inventory = Character->FindComponentByClass<UInventoryComponent>();
        if (Inventory && Inventory->SaveInventoryToDatabase(PlayerID))
        {
            TotalSaved++;
            UE_LOG(FishingGameMode, Log, TEXT("✅ SaveInventories: Saved inventory for PlayerID=%d"), PlayerID);
        }
    }

    UE_LOG(FishingGameMode, Log, TEXT("=== SaveInventories: Saved %d players ==="), TotalSaved);
    
    if (GEngine && TotalSaved > 0)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.0f, FColor::Green,
            FString::Printf(TEXT("💾 Saved %d player(s)"), TotalSaved)
        );
    }
}

void AFishingGameMode::LoadInventories()
{
    if (!DatabaseManager)
    {
        UE_LOG(FishingGameMode, Error, TEXT("LoadInventories: DatabaseManager is null!"));
        return;
    }

    int32 TotalLoaded = 0;

    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC || !PC->GetPawn())
        {
            continue;
        }

        AFishingCharacter* Character = Cast<AFishingCharacter>(PC->GetPawn());
        if (!Character)
        {
            continue;
        }

        
        int32 PlayerID = GetOrCreateSessionPlayerID(PC);
        if (PlayerID == -1)
        {
            continue;
        }

        
        UInventoryComponent* Inventory = Character->FindComponentByClass<UInventoryComponent>();
        if (Inventory && Inventory->LoadInventoryFromDatabase(PlayerID))
        {
            TotalLoaded++;
            UE_LOG(FishingGameMode, Log, TEXT("✅ LoadInventories: Loaded inventory for PlayerID=%d"), PlayerID);
        }
    }

    UE_LOG(FishingGameMode, Log, TEXT("=== LoadInventories: Loaded %d players ==="), TotalLoaded);
    
    if (GEngine && TotalLoaded > 0)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.0f, FColor::Cyan,
            FString::Printf(TEXT("📦 Loaded %d player(s)"), TotalLoaded)
        );
    }
}

void AFishingGameMode::SaveFishRecords()
{
    
    
    UE_LOG(FishingGameMode, Verbose, TEXT("SaveFishRecords: Fish records saved on catch"));
}

void AFishingGameMode::LoadFishRecords()
{
    if (!DatabaseManager)
    {
        return;
    }

    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC)
        {
            continue;
        }

        int32 PlayerID = GetOrCreateSessionPlayerID(PC);
        if (PlayerID == -1)
        {
            continue;
        }

        
        TMap<FString, int32> FishCatalog = DatabaseManager->LoadFishCatalog(PlayerID);
        
        
        
        UE_LOG(FishingGameMode, Log, TEXT("LoadFishRecords: Loaded %d fish species for PlayerID=%d"), 
            FishCatalog.Num(), PlayerID);
    }
}