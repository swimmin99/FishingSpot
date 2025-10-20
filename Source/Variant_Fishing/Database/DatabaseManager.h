
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SQLiteDatabase.h"
#include "../Data/ItemSpecificData.h"
#include "DatabaseManager.generated.h"

struct FLeaderboardEntry;
class UItemBase;

USTRUCT(BlueprintType)
struct FSaveSlotInfo
{
	GENERATED_BODY() 

	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	int32 PlayerID = -1;  

	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	FString VillageName;

	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	int32 TotalMoney = 0; 
    
	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	FString LastSaveTime;
	
	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	int32 HostPlayerID = -1;
	
	UPROPERTY(BlueprintReadOnly, Category = "SaveSlot")
	bool bIsHost = false;
};

UCLASS()
class FISHING_API UDatabaseManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	
	
	
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Database")
	bool OpenDatabase(const FString& DatabaseName = TEXT("FishingGame.db"));

	UFUNCTION(BlueprintCallable, Category = "Database")
	void CloseDatabase();

	UFUNCTION(BlueprintCallable, Category = "Database")
	bool InitializeTables(); 

	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Database|SaveSlot")
	TArray<FSaveSlotInfo> GetAllSaveSlots(); 

	UFUNCTION(BlueprintCallable, Category = "Database|SaveSlot")
	int32 FindOrCreateSaveSlot(const FString& PlayerName, const FString& VillageName); 

	UFUNCTION(BlueprintCallable, Category = "Database|SaveSlot")
	bool DeleteSaveSlot(const FString& VillageName);  

	UFUNCTION(BlueprintCallable, Category = "Database|SaveSlot")
	void SetActivePlayer(int32 PlayerID); 

	UFUNCTION(BlueprintPure, Category = "Database|SaveSlot")
	int32 GetActivePlayerID() const { return CurrentPlayerID; }

	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Database|Session")
	int32 FindSessionPlayer(int32 HostPlayerID, const FString& PlayerName);
	
	UFUNCTION(BlueprintCallable, Category = "Database|Session")
	int32 CreateSessionPlayer(int32 HostPlayerID, const FString& PlayerName);
	
	UFUNCTION(BlueprintCallable, Category = "Database|Session")
	TArray<int32> GetSessionPlayers(int32 HostPlayerID);
	
	UFUNCTION(BlueprintCallable, Category = "Database|Session")
	bool IsPlayerHost(int32 PlayerID);

	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Database|Player")
	bool SavePlayerMoney(int32 PlayerID, int32 TotalMoney); 

	UFUNCTION(BlueprintCallable, Category = "Database|Player")
	bool LoadPlayerData(int32 PlayerID, FString& OutPlayerName, FString& OutVillageName, int32& OutMoney);

	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Database|Inventory")
	bool SaveInventory(int32 PlayerID, const TMap<UItemBase*, FIntPoint>& ItemsWithPositions);

	UFUNCTION(BlueprintCallable, Category = "Database|Inventory")
	TMap<UItemBase*, FIntPoint> LoadInventory(int32 PlayerID, UObject* Outer);

	UFUNCTION(BlueprintCallable, Category = "Database|Inventory")
	bool ClearInventory(int32 PlayerID);
	
	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Database|Fish")
	bool RecordCaughtFish(int32 PlayerID, const FString& FishDataPath, const FString& FishName,
	    float Length, float Weight); 

	UFUNCTION(BlueprintCallable, Category = "Database|Fish")
	TMap<FString, int32> LoadFishCatalog(int32 PlayerID); 

	UFUNCTION(BlueprintCallable, Category = "Database|Fish")
	bool GetLargestFish(int32 PlayerID, const FString& FishDataPath, 
	    float& OutLength, float& OutWeight); 

	UFUNCTION(BlueprintCallable, Category = "Database|Leaderboard")
	TArray<FLeaderboardEntry> GetSessionLeaderboard(int32 HostPlayerID);
	
	UFUNCTION(BlueprintCallable, Category = "Database|Leaderboard")
	TArray<FString> GetSessionFishTypes(int32 HostPlayerID);
	
	
	
	
	
	UFUNCTION(BlueprintPure, Category = "Database")
	FString GetDatabasePath() const { return DatabaseFilePath; }  

	UFUNCTION(BlueprintCallable, Category = "Database")
	bool BackupDatabase(const FString& BackupName);

protected:
	bool MigrateDatabase();
	
	FSQLiteDatabase* Database = nullptr; 
	FString DatabaseFilePath;
	int32 CurrentPlayerID = -1;  
	
	UPROPERTY()
	FString PendingPlayerName;

	
	
	
	
	bool ExecuteQuery(const FString& Query);
	
	
	bool ExecutePreparedQuery(const FString& Query, TFunction<void(FSQLitePreparedStatement&)> BindFunc);
	
	
	bool ExecuteScalarInt(const FString& Query, TFunction<void(FSQLitePreparedStatement&)> BindFunc, int32& OutValue);
	
	
	bool ExecuteScalarString(const FString& Query, TFunction<void(FSQLitePreparedStatement&)> BindFunc, FString& OutValue);
	
	
	
	
	
	
	FString BuildPlayerIDList(const TArray<int32>& PlayerIDs) const;
	
	
	bool DeletePlayerData(int32 PlayerID);
	
	
	
	
	
	FString GetCurrentTimestamp();
	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();
	
	
	FString SerializeSpecificData(const FItemSpecificData& Data);
	void DeserializeSpecificData(const FString& JSON, FItemSpecificData& OutData);
};