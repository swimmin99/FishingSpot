
#include "MainMenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/Widget/Menu/MainMenuWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	
	DefaultPawnClass = nullptr;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}

	if (MainMenuWidget)
	{
		CurrentMenuWidget = CreateWidget<UMainMenuWidget>(GetWorld(), MainMenuWidget);
		if (CurrentMenuWidget)
		{
			CurrentMenuWidget->AddToViewport();
		}
	}
}