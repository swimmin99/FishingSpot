
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class UMainMenuWidget;

UCLASS()
class FISHING_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	UMainMenuWidget* CurrentMenuWidget;

	
	virtual void BeginPlay() override;
};