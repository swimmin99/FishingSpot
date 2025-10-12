#include "Variant_Fishing/GameInstance/FishingGameInstance.h"

void UFishingGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UFishingGameInstance::OnNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UFishingGameInstance::OnTravelFailure);
	}
}

void UFishingGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
                                            const FString& ErrorString)
{
	FString ErrorMessage;

	switch (FailureType)
	{
	case ENetworkFailure::ConnectionLost:
		ErrorMessage = "Connection Lost";
		break;
	case ENetworkFailure::ConnectionTimeout:
		ErrorMessage = "Connection Timeout";
		break;
	case ENetworkFailure::FailureReceived:
		ErrorMessage = "Failed to Find the Host";
		break;
	default:
		ErrorMessage = "Network Error: " + ErrorString;
		break;
	}

	ShowNetworkErrorMessage(ErrorMessage);
}

void UFishingGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	FString ErrorMessage;

	switch (FailureType)
	{
	case ETravelFailure::ServerTravelFailure:
		ErrorMessage = "Server Travel Failed";
		break;
	case ETravelFailure::ClientTravelFailure:
		ErrorMessage = "Failed to Find the Host";
		break;
	case ETravelFailure::NoLevel:
		ErrorMessage = "Level Not Found";
		break;
	default:
		ErrorMessage = "Travel Error: " + ErrorString;
		break;
	}

	ShowNetworkErrorMessage(ErrorMessage);
}

void UFishingGameInstance::ShowNetworkErrorMessage(const FString& ErrorMessage)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, ErrorMessage);
	}

	UE_LOG(LogTemp, Error, TEXT("Network Error: %s"), *ErrorMessage);
}
