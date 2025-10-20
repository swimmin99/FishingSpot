


#include "FishingPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Database/DatabaseManager.h"

AFishingPlayerState::AFishingPlayerState()
{
    
    bReplicates = true;
    bAlwaysRelevant = true;
}

void AFishingPlayerState::BeginPlay()
{
    Super::BeginPlay();
    
    
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 Client PlayerState: DatabasePlayerID=%d, Host=%d, IsHost=%s"),
            DatabasePlayerID, HostDatabasePlayerID, bIsHost ? TEXT("Yes") : TEXT("No"));
    }
}

void AFishingPlayerState::InitializeAsHost(int32 InDatabasePlayerID)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeAsHost called on client!"));
        return;
    }

    DatabasePlayerID = InDatabasePlayerID;
    HostDatabasePlayerID = -1;  
    bIsHost = true;

    UE_LOG(LogTemp, Log, TEXT("✅ Host PlayerState initialized: DatabasePlayerID=%d, Name=%s"),
        DatabasePlayerID, *GetPlayerName());
}



void AFishingPlayerState::InitializeAsGuest(int32 InDatabasePlayerID, int32 InHostDatabasePlayerID)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeAsGuest called on client!"));
        return;
    }

    DatabasePlayerID = InDatabasePlayerID;
    HostDatabasePlayerID = InHostDatabasePlayerID;
    bIsHost = false;

    UE_LOG(LogTemp, Log, TEXT("✅ Guest PlayerState initialized: DatabasePlayerID=%d, Host=%d, Name=%s"),
        DatabasePlayerID, HostDatabasePlayerID, *GetPlayerName());
}

void AFishingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    
    DOREPLIFETIME(AFishingPlayerState, DatabasePlayerID);
    DOREPLIFETIME(AFishingPlayerState, HostDatabasePlayerID);
    DOREPLIFETIME(AFishingPlayerState, bIsHost);
}