
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FishingGameState.generated.h"


UCLASS()
class FISHING_API AFishingGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFishingGameState();

	
	
	

	
	UFUNCTION(BlueprintPure, Category = "Session")
	int32 GetHostPlayerID() const { return HostPlayerID; }

	
	void SetHostPlayerID(int32 InHostPlayerID);

	
	UFUNCTION(BlueprintPure, Category = "Session")
	int32 GetSessionPlayerCount() const { return SessionPlayerCount; }

	
	UFUNCTION(BlueprintPure, Category = "Session")
	FDateTime GetSessionStartTime() const { return SessionStartTime; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	
	

	
	UPROPERTY(ReplicatedUsing = OnRep_HostPlayerID)
	int32 HostPlayerID = -1;

	
	UPROPERTY(Replicated)
	int32 SessionPlayerCount = 0;

	
	UPROPERTY(Replicated)
	FDateTime SessionStartTime;

	
	
	

	UFUNCTION()
	void OnRep_HostPlayerID();

public:
	
	void UpdateSessionPlayerCount(int32 Count);
};