
#include "Variant_Fishing/Database/DatabaseManager.h"

#include "Fishing.h"
#include "SQLiteDatabase.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Variant_Fishing/Widget/LeaderboardEntryWidget.h"

void UDatabaseManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogDatabase, Log, TEXT("DatabaseManager initialized"));
	OpenDatabase();
}

void UDatabaseManager::Deinitialize()
{
	CloseDatabase();
	Super::Deinitialize();
}

bool UDatabaseManager::OpenDatabase(const FString& DatabaseName)
{
	if (Database && Database->IsValid())
	{
		UE_LOG(LogDatabase, Warning, TEXT("Database already open"));
		return true;
	}

	DatabaseFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SavedGames"), DatabaseName);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString SaveDir = FPaths::GetPath(DatabaseFilePath); 

	if (!PlatformFile.DirectoryExists(*SaveDir))
	{
		PlatformFile.CreateDirectory(*SaveDir);
	}

	UE_LOG(LogDatabase, Log, TEXT("Opening database: %s"), *DatabaseFilePath);

	Database = new FSQLiteDatabase();

	if (!Database->Open(*DatabaseFilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogDatabase, Error, TEXT("Failed to open database: %s"), *Database->GetLastError());
		delete Database;
		Database = nullptr;
		return false;
	}

	InitializeTables();
	return true;
}

void UDatabaseManager::CloseDatabase()
{
	if (!Database)
	{
		return;
	}
	Database->Close();
	delete Database;
	Database = nullptr;
	UE_LOG(LogDatabase, Log, TEXT("Database closed"));
}

bool UDatabaseManager::InitializeTables()
{
    if (!Database || !Database->IsValid())
    {
        return false;
    }

    TArray<FString> Queries = {
        TEXT(
            "CREATE TABLE IF NOT EXISTS Players ("
            "PlayerID INTEGER PRIMARY KEY AUTOINCREMENT, "
            "PlayerName TEXT NOT NULL, "
            "VillageName TEXT NOT NULL UNIQUE, "
            "TotalMoney INTEGER DEFAULT 0, "
            "LastSaveTime TEXT, "
            "CreatedAt TEXT DEFAULT CURRENT_TIMESTAMP)"
        ),
        TEXT(
            "CREATE TABLE IF NOT EXISTS InventoryItems ("
            "ItemID INTEGER PRIMARY KEY AUTOINCREMENT, "
            "PlayerID INTEGER NOT NULL, "
            "ItemGuid TEXT UNIQUE NOT NULL, "
            "ItemDataProviderPath TEXT NOT NULL, "
            "ItemCategory TEXT NOT NULL, "
            "GridX INTEGER DEFAULT 0, "
            "GridY INTEGER DEFAULT 0, "
            "bIsRotated INTEGER DEFAULT 0, "
            "SpecificDataJSON TEXT, "
            "FOREIGN KEY (PlayerID) REFERENCES Players(PlayerID))"
        ),
        TEXT(
            "CREATE TABLE IF NOT EXISTS FishRecords ("
            "RecordID INTEGER PRIMARY KEY AUTOINCREMENT, "
            "PlayerID INTEGER NOT NULL, "
            "FishDataPath TEXT NOT NULL, "
            "FishName TEXT NOT NULL, "
            "CaughtCount INTEGER DEFAULT 0, "
            "LargestLength REAL DEFAULT 0.0, "
            "LargestWeight REAL DEFAULT 0.0, "
            "FirstCaughtAt TEXT, "
            "LastCaughtAt TEXT, "
            "FOREIGN KEY (PlayerID) REFERENCES Players(PlayerID), "
            "UNIQUE(PlayerID, FishDataPath))"
        ),
        TEXT("CREATE INDEX IF NOT EXISTS idx_inventory_player ON InventoryItems(PlayerID)"),
        TEXT("CREATE INDEX IF NOT EXISTS idx_fish_records ON FishRecords(PlayerID, FishDataPath)")
    };

    bool bSuccess = true;
    for (const FString& Query : Queries)
    {
        if (!ExecuteQuery(Query))
        {
            bSuccess = false;
        }
    }

    if (bSuccess)
    {
        bSuccess &= MigrateDatabase();
    }
	
    return bSuccess;
}





TArray<FSaveSlotInfo> UDatabaseManager::GetAllSaveSlots()
{
	TArray<FSaveSlotInfo> SaveSlots;
	if (!Database || !Database->IsValid())
	{
		UE_LOG(LogDatabase, Error, TEXT("Database not open"));
		return SaveSlots;
	}

	FString Query = TEXT(
		"SELECT PlayerID, PlayerName, VillageName, TotalMoney, LastSaveTime, HostPlayerID, IsHost "
		"FROM Players WHERE IsHost = 1 ORDER BY LastSaveTime DESC"
	);

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return SaveSlots;
	}

	while (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		FSaveSlotInfo Info;
		int32 TempIsHost;
        
		Statement.GetColumnValueByIndex(0, Info.PlayerID);
		Statement.GetColumnValueByIndex(1, Info.PlayerName);
		Statement.GetColumnValueByIndex(2, Info.VillageName);
		Statement.GetColumnValueByIndex(3, Info.TotalMoney);
		Statement.GetColumnValueByIndex(4, Info.LastSaveTime);
		Statement.GetColumnValueByIndex(5, Info.HostPlayerID);
		Statement.GetColumnValueByIndex(6, TempIsHost);
		Info.bIsHost = (TempIsHost != 0);
        
		SaveSlots.Add(Info);
	}

	UE_LOG(LogDatabase, Log, TEXT("Loaded %d save slots"), SaveSlots.Num());
	return SaveSlots;
}

int32 UDatabaseManager::FindOrCreateSaveSlot(const FString& PlayerName, const FString& VillageName)
{
	if (!Database || !Database->IsValid())
	{
		UE_LOG(LogDatabase, Error, TEXT("Database not open"));
		return -1;
	}

	
	int32 ExistingID = -1;
	if (ExecuteScalarInt(
		TEXT("SELECT PlayerID FROM Players WHERE VillageName = ?"),
		[&VillageName](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, VillageName);
		},
		ExistingID))
	{
		UE_LOG(LogDatabase, Log, TEXT("Found existing save: VillageName=%s, PlayerID=%d"), 
			*VillageName, ExistingID);
		return ExistingID;
	}

	
	if (ExecutePreparedQuery(
		TEXT("INSERT INTO Players (PlayerName, VillageName, HostPlayerID, IsHost, TotalMoney, LastSaveTime) "
		     "VALUES (?, ?, NULL, 1, 0, ?)"),
		[this, &PlayerName, &VillageName](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, PlayerName);
			Stmt.SetBindingValueByIndex(2, VillageName);
			Stmt.SetBindingValueByIndex(3, GetCurrentTimestamp());
		}))
	{
		int32 NewPlayerID = Database->GetLastInsertRowId();
		UE_LOG(LogDatabase, Log, TEXT("✅ Created new host save: %s (%s) - PlayerID=%d"), 
			*PlayerName, *VillageName, NewPlayerID);
		return NewPlayerID;
	}

	return -1;
}

bool UDatabaseManager::DeleteSaveSlot(const FString& VillageName)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	int32 PlayerID = -1;
	if (!ExecuteScalarInt(
		TEXT("SELECT PlayerID FROM Players WHERE VillageName = ?"),
		[&VillageName](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, VillageName);
		},
		PlayerID))
	{
		UE_LOG(LogDatabase, Warning, TEXT("Save slot not found: %s"), *VillageName);
		return false;
	}

	if (!BeginTransaction())
	{
		return false;
	}

	bool bSuccess = true;
	
	
	TArray<int32> SessionPlayers = GetSessionPlayers(PlayerID);
	for (int32 SessionPlayerID : SessionPlayers)
	{
		if (!DeletePlayerData(SessionPlayerID))
		{
			bSuccess = false;
			break;
		}
	}
	
	
	if (bSuccess)
	{
		bSuccess = DeletePlayerData(PlayerID);
	}

	if (bSuccess)
	{
		CommitTransaction();
		UE_LOG(LogDatabase, Log, TEXT("✅ Deleted save slot and %d session players: %s"), 
			SessionPlayers.Num(), *VillageName);
	}
	else
	{
		RollbackTransaction();
		UE_LOG(LogDatabase, Error, TEXT("❌ Failed to delete save slot: %s"), *VillageName);
	}
	
	return bSuccess;
}

void UDatabaseManager::SetActivePlayer(int32 PlayerID)
{
	CurrentPlayerID = PlayerID;
	UE_LOG(LogDatabase, Log, TEXT("Active player set: PlayerID=%d"), PlayerID);
}





int32 UDatabaseManager::FindSessionPlayer(int32 HostPlayerID, const FString& PlayerName)
{
	if (!Database || !Database->IsValid())
	{
		return -1;
	}

	int32 PlayerID = -1;
	if (ExecuteScalarInt(
		TEXT("SELECT PlayerID FROM Players WHERE HostPlayerID = ? AND PlayerName = ?"),
		[HostPlayerID, &PlayerName](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, HostPlayerID);
			Stmt.SetBindingValueByIndex(2, PlayerName);
		},
		PlayerID))
	{
		UE_LOG(LogDatabase, Log, TEXT("FindSessionPlayer: Found %s (PlayerID=%d)"), *PlayerName, PlayerID);
		return PlayerID;
	}

	UE_LOG(LogDatabase, Log, TEXT("FindSessionPlayer: %s not found"), *PlayerName);
	return -1;
}

int32 UDatabaseManager::CreateSessionPlayer(int32 HostPlayerID, const FString& PlayerName)
{
	if (!Database || !Database->IsValid())
	{
		return -1;
	}

	FString VillageName = FString::Printf(TEXT("Visitor%d"), HostPlayerID);
	
	if (ExecutePreparedQuery(
		TEXT("INSERT INTO Players (PlayerName, VillageName, HostPlayerID, IsHost, TotalMoney, LastSaveTime) "
		     "VALUES (?, ?, ?, 0, 0, ?)"),
		[this, &PlayerName, &VillageName, HostPlayerID](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, PlayerName);
			Stmt.SetBindingValueByIndex(2, VillageName);
			Stmt.SetBindingValueByIndex(3, HostPlayerID);
			Stmt.SetBindingValueByIndex(4, GetCurrentTimestamp());
		}))
	{
		int32 NewPlayerID = Database->GetLastInsertRowId();
		UE_LOG(LogDatabase, Log, TEXT("✅ CreateSessionPlayer: Created %s (PlayerID=%d, Host=%d)"),
			   *PlayerName, NewPlayerID, HostPlayerID);
		return NewPlayerID;
	}

	return -1;
}

TArray<int32> UDatabaseManager::GetSessionPlayers(int32 HostPlayerID)
{
	TArray<int32> PlayerIDs;
	
	if (!Database || !Database->IsValid())
	{
		return PlayerIDs;
	}

	FString Query = TEXT("SELECT PlayerID FROM Players WHERE HostPlayerID = ?");
	FSQLitePreparedStatement Statement;
	
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return PlayerIDs;
	}

	Statement.SetBindingValueByIndex(1, HostPlayerID);

	while (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		int32 PlayerID;
		Statement.GetColumnValueByIndex(0, PlayerID);
		PlayerIDs.Add(PlayerID);
	}

	UE_LOG(LogDatabase, Log, TEXT("GetSessionPlayers: Found %d players"), PlayerIDs.Num());
	return PlayerIDs;
}

bool UDatabaseManager::IsPlayerHost(int32 PlayerID)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	int32 IsHost = 0;
	ExecuteScalarInt(
		TEXT("SELECT IsHost FROM Players WHERE PlayerID = ?"),
		[PlayerID](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, PlayerID);
		},
		IsHost);

	return (IsHost != 0);
}





bool UDatabaseManager::SavePlayerMoney(int32 PlayerID, int32 TotalMoney)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	bool bSuccess = ExecutePreparedQuery(
		TEXT("UPDATE Players SET TotalMoney = ?, LastSaveTime = ? WHERE PlayerID = ?"),
		[this, TotalMoney, PlayerID](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, TotalMoney);
			Stmt.SetBindingValueByIndex(2, GetCurrentTimestamp());
			Stmt.SetBindingValueByIndex(3, PlayerID);
		});

	if (bSuccess)
	{
		UE_LOG(LogDatabase, Verbose, TEXT("💰 Saved player money: PlayerID=%d, Money=%d"), 
			PlayerID, TotalMoney);
	}
    
	return bSuccess;
}

bool UDatabaseManager::LoadPlayerData(int32 PlayerID, FString& OutPlayerName, FString& OutVillageName, int32& OutMoney)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}
	
	FString Query = TEXT("SELECT PlayerName, VillageName, TotalMoney FROM Players WHERE PlayerID = ?");
	FSQLitePreparedStatement Statement;
	
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return false;
	}
	
	Statement.SetBindingValueByIndex(1, PlayerID);

	if (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		Statement.GetColumnValueByIndex(0, OutPlayerName);
		Statement.GetColumnValueByIndex(1, OutVillageName);
		Statement.GetColumnValueByIndex(2, OutMoney);

		UE_LOG(LogDatabase, Log, TEXT("✅ Loaded player: %s (%s), Money=%d"), 
			*OutPlayerName, *OutVillageName, OutMoney);
		return true;
	}
	
	UE_LOG(LogDatabase, Warning, TEXT("Player not found: PlayerID=%d"), PlayerID);
	return false;
}





bool UDatabaseManager::SaveInventory(int32 PlayerID, const TMap<UItemBase*, FIntPoint>& ItemsWithPositions)
{
    if (!Database || !Database->IsValid())
    {
        return false;
    }

    if (!BeginTransaction())
    {
        return false;
    }

    if (!ExecuteQuery(FString::Printf(TEXT("DELETE FROM InventoryItems WHERE PlayerID = %d"), PlayerID)))
    {
        RollbackTransaction();
        return false;
    }

    FString InsertQuery = TEXT(
        "INSERT INTO InventoryItems (PlayerID, ItemGuid, ItemDataProviderPath, ItemCategory, "
        "GridX, GridY, bIsRotated, SpecificDataJSON) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    );

    for (const auto& Pair : ItemsWithPositions)
    {
        const UItemBase* Item = Pair.Key;
        const FIntPoint& GridPos = Pair.Value;

        if (!Item || !Item->ItemDataProvider.GetObject())
        {
            continue;
        }

        UObject* DataProviderObj = Item->ItemDataProvider.GetObject();
        FString ItemPath = DataProviderObj->GetPathName();
        EItemCategory Category = IItemDataProvider::Execute_GetCategory(DataProviderObj);
        FString CategoryStr = StaticEnum<EItemCategory>()->GetNameStringByValue((int64)Category);
        FString SpecificDataJSON = SerializeSpecificData(Item->SpecificData);

        if (!ExecutePreparedQuery(InsertQuery, [&](FSQLitePreparedStatement& Stmt) {
            Stmt.SetBindingValueByIndex(1, PlayerID);
            Stmt.SetBindingValueByIndex(2, Item->ItemGuid.ToString());
            Stmt.SetBindingValueByIndex(3, ItemPath);
            Stmt.SetBindingValueByIndex(4, CategoryStr);
            Stmt.SetBindingValueByIndex(5, GridPos.X);
            Stmt.SetBindingValueByIndex(6, GridPos.Y);
            Stmt.SetBindingValueByIndex(7, Item->bIsRotated ? 1 : 0);
            Stmt.SetBindingValueByIndex(8, SpecificDataJSON);
        }))
        {
            RollbackTransaction();
            return false;
        }
    }

    CommitTransaction();
    UE_LOG(LogDatabase, Log, TEXT("✅ Saved inventory: PlayerID=%d, Items=%d"), 
        PlayerID, ItemsWithPositions.Num());
    return true;
}

TMap<UItemBase*, FIntPoint> UDatabaseManager::LoadInventory(int32 PlayerID, UObject* Outer)
{
    TMap<UItemBase*, FIntPoint> ItemsWithPositions;

    if (!Database || !Database->IsValid() || !Outer)
    {
        return ItemsWithPositions;
    }

    FString Query = TEXT(
        "SELECT ItemGuid, ItemDataProviderPath, ItemCategory, GridX, GridY, "
        "bIsRotated, SpecificDataJSON FROM InventoryItems WHERE PlayerID = ?"
    );

    FSQLitePreparedStatement Statement;
    if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
    {
        return ItemsWithPositions;
    }

    Statement.SetBindingValueByIndex(1, PlayerID);

    while (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
    {
        FString ItemGuidStr, ItemPath, Category, SpecificDataJSON;
        int32 GridX, GridY, bIsRotatedInt;

        Statement.GetColumnValueByIndex(0, ItemGuidStr);
        Statement.GetColumnValueByIndex(1, ItemPath);
        Statement.GetColumnValueByIndex(2, Category);
        Statement.GetColumnValueByIndex(3, GridX);
        Statement.GetColumnValueByIndex(4, GridY);
        Statement.GetColumnValueByIndex(5, bIsRotatedInt);
        Statement.GetColumnValueByIndex(6, SpecificDataJSON);

        TSoftObjectPtr<UObject> ItemDataAsset{FSoftObjectPath(ItemPath)};
        UObject* LoadedAsset = ItemDataAsset.LoadSynchronous();
        
        if (!LoadedAsset || !LoadedAsset->Implements<UItemDataProvider>())
        {
            UE_LOG(LogDatabase, Warning, TEXT("Failed to load: %s"), *ItemPath);
            continue;
        }

        UItemBase* NewItem = NewObject<UItemBase>(Outer);
        if (NewItem)
        {
            FGuid::Parse(ItemGuidStr, NewItem->ItemGuid);
            NewItem->ItemDataProvider.SetObject(LoadedAsset);
            NewItem->ItemDataProvider.SetInterface(Cast<IItemDataProvider>(LoadedAsset));
            NewItem->bIsRotated = (bIsRotatedInt != 0);
            DeserializeSpecificData(SpecificDataJSON, NewItem->SpecificData);
            
            ItemsWithPositions.Add(NewItem, FIntPoint(GridX, GridY));
        }
    }

    UE_LOG(LogDatabase, Log, TEXT("✅ Loaded inventory: PlayerID=%d, Items=%d"), 
        PlayerID, ItemsWithPositions.Num());
    return ItemsWithPositions;
}

bool UDatabaseManager::ClearInventory(int32 PlayerID)
{
	bool bSuccess = ExecuteQuery(
		FString::Printf(TEXT("DELETE FROM InventoryItems WHERE PlayerID = %d"), PlayerID)
	);
    
	if (bSuccess)
	{
		UE_LOG(LogDatabase, Log, TEXT("✅ Cleared inventory: PlayerID=%d"), PlayerID);
	}
    
	return bSuccess;
}





bool UDatabaseManager::RecordCaughtFish(int32 PlayerID, const FString& FishDataPath, 
	const FString& FishName, float Length, float Weight)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	FString CurrentTime = GetCurrentTimestamp();

	bool bSuccess = ExecutePreparedQuery(
		TEXT("INSERT INTO FishRecords (PlayerID, FishDataPath, FishName, CaughtCount, "
		     "LargestLength, LargestWeight, FirstCaughtAt, LastCaughtAt) "
		     "VALUES (?, ?, ?, 1, ?, ?, ?, ?) "
		     "ON CONFLICT(PlayerID, FishDataPath) DO UPDATE SET "
		     "CaughtCount = CaughtCount + 1, "
		     "LargestLength = MAX(LargestLength, ?), "
		     "LargestWeight = MAX(LargestWeight, ?), "
		     "LastCaughtAt = ?"),
		[PlayerID, &FishDataPath, &FishName, Length, Weight, &CurrentTime](FSQLitePreparedStatement& Stmt) {
			Stmt.SetBindingValueByIndex(1, PlayerID);
			Stmt.SetBindingValueByIndex(2, FishDataPath);
			Stmt.SetBindingValueByIndex(3, FishName);
			Stmt.SetBindingValueByIndex(4, Length);
			Stmt.SetBindingValueByIndex(5, Weight);
			Stmt.SetBindingValueByIndex(6, CurrentTime);
			Stmt.SetBindingValueByIndex(7, CurrentTime);
			Stmt.SetBindingValueByIndex(8, Length);
			Stmt.SetBindingValueByIndex(9, Weight);
			Stmt.SetBindingValueByIndex(10, CurrentTime);
		});

	if (bSuccess)
	{
		UE_LOG(LogDatabase, Log, TEXT("✅ Recorded fish: %s (%.1fcm, %.1fg)"), 
			*FishName, Length, Weight);
	}

	return bSuccess;
}

TMap<FString, int32> UDatabaseManager::LoadFishCatalog(int32 PlayerID)
{
	TMap<FString, int32> Catalog;

	if (!Database || !Database->IsValid())
	{
		return Catalog;
	}

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database, 
		TEXT("SELECT FishDataPath, CaughtCount FROM FishRecords WHERE PlayerID = ?"),
		ESQLitePreparedStatementFlags::Persistent))
	{
		return Catalog;
	}

	Statement.SetBindingValueByIndex(1, PlayerID);

	while (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		FString FishPath;
		int32 Count;
		Statement.GetColumnValueByIndex(0, FishPath);
		Statement.GetColumnValueByIndex(1, Count);
		Catalog.Add(FishPath, Count);
	}

	UE_LOG(LogDatabase, Log, TEXT("✅ Loaded fish catalog: %d species"), Catalog.Num());
	return Catalog;
}

bool UDatabaseManager::GetLargestFish(int32 PlayerID, const FString& FishDataPath, 
	float& OutLength, float& OutWeight)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database,
		TEXT("SELECT LargestLength, LargestWeight FROM FishRecords "
		     "WHERE PlayerID = ? AND FishDataPath = ?"),
		ESQLitePreparedStatementFlags::Persistent))
	{
		return false;
	}

	Statement.SetBindingValueByIndex(1, PlayerID);
	Statement.SetBindingValueByIndex(2, FishDataPath);

	if (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		double TempLength, TempWeight;
		Statement.GetColumnValueByIndex(0, TempLength);
		Statement.GetColumnValueByIndex(1, TempWeight);
        
		OutLength = static_cast<float>(TempLength);
		OutWeight = static_cast<float>(TempWeight);
        
		UE_LOG(LogDatabase, Log, TEXT("✅ Largest fish: %s (%.1fcm, %.1fg)"), 
			*FishDataPath, OutLength, OutWeight);
		return true;
	}

	UE_LOG(LogDatabase, Warning, TEXT("Fish not found: %s"), *FishDataPath);
	return false;
}

TArray<FLeaderboardEntry> UDatabaseManager::GetSessionLeaderboard(int32 HostPlayerID)
{
    TArray<FLeaderboardEntry> Entries;

    if (!Database || !Database->IsValid() || HostPlayerID == -1)
    {
        UE_LOG(LogDatabase, Warning, TEXT("GetSessionLeaderboard: Invalid parameters"));
        return Entries;
    }

    TArray<int32> SessionPlayers = GetSessionPlayers(HostPlayerID);
    SessionPlayers.Add(HostPlayerID);

    if (SessionPlayers.Num() == 0)
    {
        return Entries;
    }

    FString PlayerIDList = BuildPlayerIDList(SessionPlayers);
    FString Query = FString::Printf(
        TEXT("SELECT F.PlayerID, P.PlayerName, F.FishName, F.LargestLength, F.LargestWeight, "
             "F.CaughtCount, F.LastCaughtAt "
             "FROM FishRecords F "
             "JOIN Players P ON F.PlayerID = P.PlayerID "
             "WHERE F.PlayerID IN (%s) "
             "ORDER BY F.LargestLength DESC"),
        *PlayerIDList
    );

    FSQLitePreparedStatement Statement;
    if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
    {
        return Entries;
    }

    int32 Rank = 1;
    while (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
    {
        FLeaderboardEntry Entry;
        Entry.Rank = Rank++;
        
        double TempLength, TempWeight;

        Statement.GetColumnValueByIndex(0, Entry.PlayerID);
        Statement.GetColumnValueByIndex(1, Entry.PlayerName);
        Statement.GetColumnValueByIndex(2, Entry.FishName);
        Statement.GetColumnValueByIndex(3, TempLength);
        Statement.GetColumnValueByIndex(4, TempWeight);
        Statement.GetColumnValueByIndex(5, Entry.CaughtCount);
        Statement.GetColumnValueByIndex(6, Entry.CaughtDate);

        Entry.Length = static_cast<float>(TempLength);
        Entry.Weight = static_cast<float>(TempWeight);

        Entries.Add(Entry);
    }

    UE_LOG(LogDatabase, Log, TEXT("✅ GetSessionLeaderboard: Loaded %d entries"), Entries.Num());
    return Entries;
}

TArray<FString> UDatabaseManager::GetSessionFishTypes(int32 HostPlayerID)
{
	TArray<FString> FishTypes;

	if (!Database || !Database->IsValid())
	{
		return FishTypes;
	}

	TArray<int32> SessionPlayers = GetSessionPlayers(HostPlayerID);
	SessionPlayers.Add(HostPlayerID);

	if (SessionPlayers.Num() == 0)
	{
		return FishTypes;
	}

	FString PlayerIDList = BuildPlayerIDList(SessionPlayers);
	FString Query = FString::Printf(
		TEXT("SELECT DISTINCT FishName FROM FishRecords WHERE PlayerID IN (%s) ORDER BY FishName"),
		*PlayerIDList
	);

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return FishTypes;
	}

	while (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		FString FishName;
		Statement.GetColumnValueByIndex(0, FishName);
		FishTypes.Add(FishName);
	}

	UE_LOG(LogDatabase, Log, TEXT("✅ GetSessionFishTypes: Found %d types"), FishTypes.Num());
	return FishTypes;
}





bool UDatabaseManager::BackupDatabase(const FString& BackupName)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	FString BackupPath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Backups"),
		BackupName + TEXT(".db")
	);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString BackupDir = FPaths::GetPath(BackupPath);
    
	if (!PlatformFile.DirectoryExists(*BackupDir))
	{
		PlatformFile.CreateDirectory(*BackupDir);
	}

	if (PlatformFile.CopyFile(*BackupPath, *DatabaseFilePath))
	{
		UE_LOG(LogDatabase, Log, TEXT("✅ Database backed up: %s"), *BackupPath);
		return true;
	}

	UE_LOG(LogDatabase, Error, TEXT("❌ Failed to backup database"));
	return false;
}

bool UDatabaseManager::MigrateDatabase()
{
    if (!Database || !Database->IsValid())
    {
        return false;
    }

    FSQLitePreparedStatement CheckStmt;
    if (!CheckStmt.Create(*Database, TEXT("PRAGMA table_info(Players)"), 
        ESQLitePreparedStatementFlags::Persistent))
    {
        return false;
    }

    bool bHasHostPlayerID = false;
    bool bHasIsHost = false;
    
    while (CheckStmt.Step() == ESQLitePreparedStatementStepResult::Row)
    {
        FString ColumnName;
        CheckStmt.GetColumnValueByIndex(1, ColumnName);
        
        if (ColumnName == TEXT("HostPlayerID"))
        {
            bHasHostPlayerID = true;
        }
        else if (ColumnName == TEXT("IsHost"))
        {
            bHasIsHost = true;
        }
    }

    bool bSuccess = true;

    if (!bHasHostPlayerID)
    {
        bSuccess &= ExecuteQuery(TEXT("ALTER TABLE Players ADD COLUMN HostPlayerID INTEGER DEFAULT NULL"));
        UE_LOG(LogDatabase, Log, TEXT("✅ Added HostPlayerID column"));
    }

    if (!bHasIsHost)
    {
        bSuccess &= ExecuteQuery(TEXT("ALTER TABLE Players ADD COLUMN IsHost INTEGER DEFAULT 0"));
        UE_LOG(LogDatabase, Log, TEXT("✅ Added IsHost column"));
        
        ExecuteQuery(TEXT("UPDATE Players SET IsHost = 1 WHERE IsHost IS NULL OR IsHost = 0"));
    }

    ExecuteQuery(TEXT("CREATE INDEX IF NOT EXISTS idx_session_players ON Players(HostPlayerID)"));

    UE_LOG(LogDatabase, Log, TEXT("✅ Database migration complete"));
    return bSuccess;
}





bool UDatabaseManager::ExecuteQuery(const FString& Query)
{
	if (!Database || !Database->IsValid())
	{
		UE_LOG(LogDatabase, Error, TEXT("Database not open"));
		return false;
	}

	FSQLitePreparedStatement Statement;
	if(!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		UE_LOG(LogDatabase, Error, TEXT("Failed to prepare: %s"), *Query);
		UE_LOG(LogDatabase, Error, TEXT("Error: %s"), *Database->GetLastError());
		return false;
	}
	
	return Statement.Execute();
}

bool UDatabaseManager::ExecutePreparedQuery(const FString& Query, TFunction<void(FSQLitePreparedStatement&)> BindFunc)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return false;
	}

	if (BindFunc)
	{
		BindFunc(Statement);
	}

	return Statement.Execute();
}

bool UDatabaseManager::ExecuteScalarInt(const FString& Query, TFunction<void(FSQLitePreparedStatement&)> BindFunc, int32& OutValue)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return false;
	}

	if (BindFunc)
	{
		BindFunc(Statement);
	}

	if (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		Statement.GetColumnValueByIndex(0, OutValue);
		return true;
	}

	return false;
}

bool UDatabaseManager::ExecuteScalarString(const FString& Query, TFunction<void(FSQLitePreparedStatement&)> BindFunc, FString& OutValue)
{
	if (!Database || !Database->IsValid())
	{
		return false;
	}

	FSQLitePreparedStatement Statement;
	if (!Statement.Create(*Database, *Query, ESQLitePreparedStatementFlags::Persistent))
	{
		return false;
	}

	if (BindFunc)
	{
		BindFunc(Statement);
	}

	if (Statement.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		Statement.GetColumnValueByIndex(0, OutValue);
		return true;
	}

	return false;
}





FString UDatabaseManager::BuildPlayerIDList(const TArray<int32>& PlayerIDs) const
{
	FString Result;
	for (int32 i = 0; i < PlayerIDs.Num(); i++)
	{
		Result += FString::Printf(TEXT("%d"), PlayerIDs[i]);
		if (i < PlayerIDs.Num() - 1)
		{
			Result += TEXT(",");
		}
	}
	return Result;
}

bool UDatabaseManager::DeletePlayerData(int32 PlayerID)
{
	return ExecuteQuery(FString::Printf(TEXT("DELETE FROM InventoryItems WHERE PlayerID = %d"), PlayerID))
		&& ExecuteQuery(FString::Printf(TEXT("DELETE FROM FishRecords WHERE PlayerID = %d"), PlayerID))
		&& ExecuteQuery(FString::Printf(TEXT("DELETE FROM Players WHERE PlayerID = %d"), PlayerID));
}





FString UDatabaseManager::GetCurrentTimestamp()
{
	return FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
}

bool UDatabaseManager::BeginTransaction()
{
	return ExecuteQuery(TEXT("BEGIN TRANSACTION"));
}

bool UDatabaseManager::CommitTransaction()
{
	return ExecuteQuery(TEXT("COMMIT"));
}

bool UDatabaseManager::RollbackTransaction()
{
	return ExecuteQuery(TEXT("ROLLBACK"));
}





FString UDatabaseManager::SerializeSpecificData(const FItemSpecificData& Data)
{
    if (!Data.HasSpecificData())
    {
        return TEXT("{}");
    }

    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    switch (Data.ActiveType)
    {
    case EItemSpecificDataType::Fish:
        {
            const FFishSpecificData* FishData = Data.GetFishData();
            if (FishData && FishData->IsValid())
            {
                RootObject->SetStringField(TEXT("Type"), TEXT("Fish"));
                RootObject->SetNumberField(TEXT("Length"), FishData->ActualLength);
                RootObject->SetNumberField(TEXT("Weight"), FishData->ActualWeight);
                RootObject->SetStringField(TEXT("Name"), FishData->FishDataName);
                RootObject->SetStringField(TEXT("Location"), FishData->CaughtLocation);
                RootObject->SetStringField(TEXT("Time"), FishData->CaughtTime.ToString());
                RootObject->SetNumberField(TEXT("MinLength"), FishData->MinLength);
                RootObject->SetNumberField(TEXT("MaxLength"), FishData->MaxLength);
                RootObject->SetNumberField(TEXT("MinWeight"), FishData->MinWeight);
                RootObject->SetNumberField(TEXT("MaxWeight"), FishData->MaxWeight);
            }
        }
        break;

    case EItemSpecificDataType::Equipment:
        {
            const FEquipmentSpecificData* EquipData = Data.GetEquipmentData();
            if (EquipData && EquipData->IsValid())
            {
                RootObject->SetStringField(TEXT("Type"), TEXT("Equipment"));
                RootObject->SetNumberField(TEXT("Durability"), EquipData->Durability);
                RootObject->SetNumberField(TEXT("MaxDurability"), EquipData->MaxDurability);
                RootObject->SetNumberField(TEXT("EnhancementLevel"), EquipData->EnhancementLevel);
                RootObject->SetBoolField(TEXT("IsEquipped"), EquipData->bIsEquipped);
            }
        }
        break;

    case EItemSpecificDataType::Consumable:
        {
            const FConsumableSpecificData* ConsData = Data.GetConsumableData();
            if (ConsData && ConsData->IsValid())
            {
                RootObject->SetStringField(TEXT("Type"), TEXT("Consumable"));
                RootObject->SetNumberField(TEXT("StackCount"), ConsData->StackCount);
                RootObject->SetNumberField(TEXT("MaxStackCount"), ConsData->MaxStackCount);
                RootObject->SetStringField(TEXT("ExpirationDate"), ConsData->ExpirationDate.ToString());
            }
        }
        break;
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    return OutputString;
}

void UDatabaseManager::DeserializeSpecificData(const FString& JSON, FItemSpecificData& OutData)
{
    OutData.Reset();

    if (JSON.IsEmpty() || JSON == TEXT("{}"))
    {
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSON);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogDatabase, Error, TEXT("Failed to parse JSON: %s"), *JSON);
        return;
    }

    FString Type;
    if (!JsonObject->TryGetStringField(TEXT("Type"), Type))
    {
        return;
    }

    if (Type == TEXT("Fish"))
    {
        FFishSpecificData FishData;
        
        JsonObject->TryGetNumberField(TEXT("Length"), FishData.ActualLength);
        JsonObject->TryGetNumberField(TEXT("Weight"), FishData.ActualWeight);
        JsonObject->TryGetStringField(TEXT("Name"), FishData.FishDataName);
        JsonObject->TryGetStringField(TEXT("Location"), FishData.CaughtLocation);
        
        FString TimeStr;
        if (JsonObject->TryGetStringField(TEXT("Time"), TimeStr))
        {
            FDateTime::Parse(TimeStr, FishData.CaughtTime);
        }
        
        JsonObject->TryGetNumberField(TEXT("MinLength"), FishData.MinLength);
        JsonObject->TryGetNumberField(TEXT("MaxLength"), FishData.MaxLength);
        JsonObject->TryGetNumberField(TEXT("MinWeight"), FishData.MinWeight);
        JsonObject->TryGetNumberField(TEXT("MaxWeight"), FishData.MaxWeight);
        
        OutData.SetFishData(FishData);
    }
    else if (Type == TEXT("Equipment"))
    {
        FEquipmentSpecificData EquipData;
        
        double Durability, MaxDurability;
        int32 EnhancementLevel;
        bool bIsEquipped;
        
        JsonObject->TryGetNumberField(TEXT("Durability"), Durability);
        JsonObject->TryGetNumberField(TEXT("MaxDurability"), MaxDurability);
        JsonObject->TryGetNumberField(TEXT("EnhancementLevel"), EnhancementLevel);
        JsonObject->TryGetBoolField(TEXT("IsEquipped"), bIsEquipped);
        
        EquipData.Durability = static_cast<float>(Durability);
        EquipData.MaxDurability = static_cast<float>(MaxDurability);
        EquipData.EnhancementLevel = EnhancementLevel;
        EquipData.bIsEquipped = bIsEquipped;
        
        OutData.SetEquipmentData(EquipData);
    }
    else if (Type == TEXT("Consumable"))
    {
        FConsumableSpecificData ConsData;
        
        int32 StackCount, MaxStackCount;
        
        JsonObject->TryGetNumberField(TEXT("StackCount"), StackCount);
        JsonObject->TryGetNumberField(TEXT("MaxStackCount"), MaxStackCount);
        
        ConsData.StackCount = StackCount;
        ConsData.MaxStackCount = MaxStackCount;
        
        FString DateStr;
        if (JsonObject->TryGetStringField(TEXT("ExpirationDate"), DateStr))
        {
            FDateTime::Parse(DateStr, ConsData.ExpirationDate);
        }
        
        OutData.SetConsumableData(ConsData);
    }
}