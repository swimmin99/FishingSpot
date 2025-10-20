



#include "MultiplayerPlayerController.h"
#include "FishingGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Fishing.h"
#include "Kismet/GameplayStatics.h"


#include "Variant_Fishing/GameInstance/FishingGameInstance.h"  

AMultiplayerPlayerController::AMultiplayerPlayerController()
{
}

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	if (!IsLocalController() || HasAuthority())
	{
		return;
	}
	
	UE_LOG(LogMultiplayerPC, Log, TEXT("BeginPlay: InLocal PlayerController detected."));
	
	const FString PlayerName = GetPendingPlayerName();
	if (PlayerName.IsEmpty())
	{
		UE_LOG(LogMultiplayerPC, Warning, TEXT("BeginPlay: Pending PlayerName is empty; skip sending."));
		return;
	}

	UE_LOG(LogMultiplayerPC, Log, TEXT("PlayerName for Widget : '%s' set. "), *PlayerName);
	PlayerNameForCharacterInit = PlayerName;
		
	Server_SendPlayerName(PlayerName);
	UE_LOG(LogMultiplayerPC, Log, TEXT("Client invoked Server_SendPlayerName: '%s'"), *PlayerName);
}

FString AMultiplayerPlayerController::GetPendingPlayerName()
{
	if (HasAuthority() && IsLocalController())
	{
		return PlayerState ? PlayerState->GetPlayerName() : FString(TEXT("Player State was Null"));
	}
	
	if (UFishingGameInstance* GI = Cast<UFishingGameInstance>(GetGameInstance()))
	{
		FString RecievedPlayerName = GI->GetAndClearPendingPlayerName();
		UE_LOG(LogMultiplayerPC, Log, TEXT("GetPendingPlayerName : UFishingGameInstance Exists : %s"), *RecievedPlayerName);
		return RecievedPlayerName; 
	}

	return FString();
}

void AMultiplayerPlayerController::KickoffPlayerNameFlow()
{
	
	
	
	
	
	
	
	
	
	
	
	
}



void AMultiplayerPlayerController::Server_SendPlayerName_Implementation(const FString& InPlayerName)
{
	if (InPlayerName.IsEmpty())
	{
		UE_LOG(LogMultiplayerPC, Warning, TEXT("Server_SendPlayerName: PlayerName is empty"));
		return;
	}

	AFishingGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFishingGameMode>() : nullptr;
	if (!GM)
	{
		UE_LOG(LogMultiplayerPC, Error, TEXT("Server_SendPlayerName: GameMode not found on server"));
		return;
	}

	
	GM->ServerReceivePlayerName(this, InPlayerName);
	UE_LOG(LogMultiplayerPC, Log, TEXT("Server_SendPlayerName: forwarded '%s' to GameMode"), *InPlayerName);
}



void AMultiplayerPlayerController::LeaveSession()
{
	UE_LOG(LogMultiplayerPC, Log, TEXT("LeaveSession: Requesting to leave."));

	Server_SaveMyData();

	FTimerHandle DelayTimer;
	GetWorldTimerManager().SetTimer(
		DelayTimer,
		[this]()
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.0f,
					FColor::Cyan,
					TEXT("💾 Your data has been saved. Goodbye!")
				);
			}

			UWorld* World = GetWorld();
			if (!World)
			{
				return;
			}

			
			ConsoleCommand(TEXT("disconnect"));
			
			
			TWeakObjectPtr<UWorld> WeakWorld(World);
			
			FTimerHandle TravelTimer;
			World->GetTimerManager().SetTimer(
				TravelTimer,
				[WeakWorld]()
				{
					if (WeakWorld.IsValid() && !WeakWorld->bIsTearingDown)
					{
						
						UGameplayStatics::OpenLevel(WeakWorld.Get(), TEXT("MainMenu"), false);
					}
				},
				0.3f,
				false
			);
		},
		0.5f,
		false
	);
}

void AMultiplayerPlayerController::Server_SaveMyData_Implementation()
{
	UE_LOG(LogMultiplayerPC, Log, TEXT("Server_SaveMyData: Saving player data."));

	if (AFishingGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFishingGameMode>() : nullptr)
	{
		GameMode->SavePlayerByController(this); 
		Client_OnDataSaved();
	}
}

void AMultiplayerPlayerController::Client_OnDataSaved_Implementation()
{
	UE_LOG(LogMultiplayerPC, Log, TEXT("Client_OnDataSaved: Data saved successfully"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Green,
			TEXT("✅ Data saved!")
		);
	}
}

void AMultiplayerPlayerController::ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason)
{
	UE_LOG(LogMultiplayerPC, Warning, TEXT("ClientReturnToMainMenuWithTextReason: %s"), *ReturnReason.ToString());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 5.0f, FColor::Yellow,
			FString::Printf(TEXT("🚪 %s"), *ReturnReason.ToString())
		);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return; 
	}

	
	TWeakObjectPtr<AMultiplayerPlayerController> WeakPC(this);

	FTimerHandle DelayTimer;
	FTimerDelegate Del;
	Del.BindLambda([WeakPC]()
	{
		if (!WeakPC.IsValid())
		{
			return; 
		}

		AMultiplayerPlayerController* PC = WeakPC.Get();
		UWorld* W = PC->GetWorld();
		if (!W || W->bIsTearingDown)
		{
			return; 
		}

		
		
		PC->ClientTravel(TEXT("/Game/Maps/MainMenu"), TRAVEL_Absolute);
		
	});

	World->GetTimerManager().SetTimer(DelayTimer, Del, 1.5f, false);
}