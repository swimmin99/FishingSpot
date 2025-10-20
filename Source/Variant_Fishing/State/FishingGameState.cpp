
#include "FishingGameState.h"
#include "Net/UnrealNetwork.h"

AFishingGameState::AFishingGameState()
{
	
	bReplicates = true;
}

void AFishingGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	
	DOREPLIFETIME(AFishingGameState, HostPlayerID);
	DOREPLIFETIME(AFishingGameState, SessionPlayerCount);
	DOREPLIFETIME(AFishingGameState, SessionStartTime);
}

void AFishingGameState::SetHostPlayerID(int32 InHostPlayerID)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("SetHostPlayerID: Not authority!"));
		return;
	}

	HostPlayerID = InHostPlayerID;
	UE_LOG(LogTemp, Log, TEXT("✅ GameState: HostPlayerID set to %d"), HostPlayerID);
}

void AFishingGameState::OnRep_HostPlayerID()
{
	UE_LOG(LogTemp, Log, TEXT("🔄 Client: Received HostPlayerID = %d"), HostPlayerID);
}

void AFishingGameState::UpdateSessionPlayerCount(int32 Count)
{
	if (!HasAuthority())
	{
		return;
	}

	SessionPlayerCount = Count;
	UE_LOG(LogTemp, Log, TEXT("✅ GameState: SessionPlayerCount = %d"), SessionPlayerCount);
}