



#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MultiplayerPlayerController.generated.h"

class AFishingGameMode;
class UFishingGameInstance;

UCLASS()
class FISHING_API AMultiplayerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMultiplayerPlayerController();
	
	
	virtual void BeginPlay() override;

	
	UFUNCTION(BlueprintCallable, Category = "Session")
	void LeaveSession();

protected:
	
	UFUNCTION(Server, Reliable)
	void Server_SaveMyData();

	
	UFUNCTION(Client, Reliable)
	void Client_OnDataSaved();

	
	UFUNCTION(Server, Reliable)
	void Server_SendPlayerName(const FString& InPlayerName);

	
	virtual void ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason) override;

	UPROPERTY()
	FString PlayerNameForCharacterInit;

	void virtual AllReadyToStart() {};
private:
	
	FString GetPendingPlayerName();

	
	void KickoffPlayerNameFlow();


};
