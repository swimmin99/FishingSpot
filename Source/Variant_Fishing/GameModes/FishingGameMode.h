
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FishingPlayerState.h"
#include "FishingGameMode.generated.h"

class UDatabaseManager;
class AFishingGameState;

UCLASS(abstract)
class FISHING_API AFishingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AFishingGameMode();

    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Save")
    virtual void SaveGame();
    
    UFUNCTION(BlueprintCallable, Category = "Save")
    virtual void LoadGame();
    
    UFUNCTION(BlueprintCallable, Category = "GameMode|Navigation")
    void EndSessionAndReturnToMainMenu();

    UFUNCTION(BlueprintPure, Category = "Session")
    AFishingGameState* GetFishingGameState() const;
    
    
    UFUNCTION(BlueprintCallable, Category = "GameMode|Save")
    void SavePlayerByController(APlayerController* PC);

    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "GameMode")
    void ServerReceivePlayerName(APlayerController* PC, const FString& PlayerName);


protected:
    
    
    
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    
    
    
    
    
    UPROPERTY(EditDefaultsOnly, Category = "Save", meta = (ClampMin = "60.0", ClampMax = "3600.0"))
    float AutoSaveInterval = 300.0f;  

    
    UPROPERTY(EditDefaultsOnly, Category = "Save")
    bool bAutoLoadOnStart = true;

    
    UPROPERTY(EditDefaultsOnly, Category = "Save|Multiplayer")
    bool bAutoLoadOnJoin = true;

    
    UPROPERTY(EditDefaultsOnly, Category = "Save|Multiplayer")
    bool bAutoSaveOnLogout = true;

    UPROPERTY(EditDefaultsOnly, Category = "GameMode|Navigation")
    FText KickMessage = FText::FromString(TEXT("Host ended the session"));

    
    
    
    
    UPROPERTY()
    UDatabaseManager* DatabaseManager = nullptr;

    FTimerHandle AutoSaveTimerHandle;

    
    UPROPERTY()
    TMap<APlayerController*, int32> PlayerDatabaseIDMap;
       
    
    UPROPERTY()
    int32 HostDatabasePlayerID = -1;

    
    int32 CachedHostDatabasePlayerID = -1;
    
    
    
    
    
    void InitializeSaveSystem();
    void InitializeSession();
    void StartAutoSaveTimer();
    void StopAutoSaveTimer();
    void ReturnToMainMenu();
    void KickAllGuests();
    
    
    int32 GetOrCreateSessionPlayerID(APlayerController* PC);
    void RemovePlayerID(APlayerController* PC);
    bool IsPlayerHost(APlayerController* PC) const;
    
    
    void LoadPlayerDataDelayed(APlayerController* PC, int32 PlayerID);
    void SavePlayerDataForID(APlayerController* PC, int32 PlayerID);
    
    
    UFUNCTION()
    void AutoSave();
    
    
    virtual void SavePlayerData();
    virtual void LoadPlayerData();
    
    virtual void SaveInventories();
    virtual void LoadInventories();
    
    virtual void SaveFishRecords();
    virtual void LoadFishRecords();
};