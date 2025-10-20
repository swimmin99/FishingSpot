#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FishingGameInstance.generated.h"

UCLASS()
class FISHING_API UFishingGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UPROPERTY()
	FString PendingPlayerName;
    
	void SetPendingPlayerName(const FString& InName)
	{
		PendingPlayerName = InName;
		UE_LOG(LogTemp, Log, TEXT("✅ GameInstance: PendingPlayerName set to '%s'"), *PendingPlayerName);
	}
    
	FString GetAndClearPendingPlayerName()
	{
		FString Result = PendingPlayerName;
		PendingPlayerName.Empty();
		return Result;
	}
	
protected:
	UFUNCTION()
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
	                      const FString& ErrorString);

	UFUNCTION()
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

private:
	void ShowNetworkErrorMessage(const FString& ErrorMessage);
};
