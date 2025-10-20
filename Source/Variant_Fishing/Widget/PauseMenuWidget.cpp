

#include "Variant_Fishing/Widget/PauseMenuWidget.h"

#include "BaseButtonWidget.h"
#include "FishingGameMode.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogPauseMenu, Log, All);

static const TCHAR* VisibilityToString(ESlateVisibility V)
{
	switch (V)
	{
	case ESlateVisibility::Visible: return TEXT("Visible");
	case ESlateVisibility::Collapsed: return TEXT("Collapsed");
	case ESlateVisibility::Hidden: return TEXT("Hidden");
	case ESlateVisibility::HitTestInvisible: return TEXT("HitTestInvisible");
	case ESlateVisibility::SelfHitTestInvisible: return TEXT("SelfHitTestInvisible");
	default: return TEXT("Unknown");
	}
}


bool UPauseMenuWidget::IsPanelOpen() const
{
	if (GetVisibility() == ESlateVisibility::Visible) { return true; } else { return false; }
}

void UPauseMenuWidget::TogglePanel()
{
	UE_LOG(LogPauseMenu, Log, TEXT("TogglePanel: Current Visibility=%s"),
		VisibilityToString(GetVisibility()));

	if (GetVisibility() == ESlateVisibility::Collapsed)
	{
		UE_LOG(LogPauseMenu, Log, TEXT("TogglePanel: Opening panel"));
		OnOpen();
	}
	else
	{
		UE_LOG(LogPauseMenu, Log, TEXT("TogglePanel: Closing panel"));
		OnClosed();
	}
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogPauseMenu, Log, TEXT("NativeConstruct: Begin"));

	CachedMultiplayerController = Cast<AMultiplayerPlayerController>(GetOwningPlayer());
	if (!CachedMultiplayerController)
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: OwningPlayer is null or not AMultiplayerPlayerController"));
		return;
	}
	UE_LOG(LogPauseMenu, Log, TEXT("NativeConstruct: CachedMultiplayerController set (Authority=%s)"),
		CachedMultiplayerController->HasAuthority() ? TEXT("true") : TEXT("false"));

	
	if (ResumeButton)
	{
		if (UButton* Base = ResumeButton->GetBaseButton())
		{
			Base->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnClickedResume);
			ResumeButton->SetText(FText::FromString(TEXT("Resume")));
			UE_LOG(LogPauseMenu, Log, TEXT("NativeConstruct: ResumeButton bound"));
		}
		else
		{
			UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: ResumeButton->GetBaseButton() is null"));
		}
	}
	else
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: ResumeButton is null"));
	}

	
	if (!CachedMultiplayerController->HasAuthority())
	{
		if (SaveButton)
		{
			if (UButton* Base = SaveButton->GetBaseButton())
			{
				SaveButton->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: SaveButton->GetBaseButton() is null"));
			}
		}
		else
		{
			UE_LOG(LogPauseMenu, Verbose, TEXT("NativeConstruct: SaveButton null (non-authority)"));
		}
	}
	else 
	{
		if (SaveButton)
		{
			SaveButton->SetText(FText::FromString(TEXT("Save")));
			if (UButton* Base = SaveButton->GetBaseButton())
			{
				Base->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnClickedSave);
				UE_LOG(LogPauseMenu, Log, TEXT("NativeConstruct: SaveButton bound (authority)"));
			}
			else
			{
				UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: SaveButton->GetBaseButton() is null (authority)"));
			}
		}
		else
		{
			UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: SaveButton is null (authority)"));
		}
	}

	
	if (MainMenuButton)
	{
		MainMenuButton->SetText(FText::FromString(TEXT("Main Menu")));
		if (UButton* Base = MainMenuButton->GetBaseButton())
		{
			Base->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnClickedMainMenu);
		}
		else
		{
			UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: MainMenuButton->GetBaseButton() is null"));
		}
	}
	else
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("NativeConstruct: MainMenuButton is null"));
	}

	UE_LOG(LogPauseMenu, Log, TEXT("NativeConstruct: End"));
}

void UPauseMenuWidget::OnClickedResume()
{
	UE_LOG(LogPauseMenu, Log, TEXT("OnClickedResume: invoked"));
	OnClosed();
}



void UPauseMenuWidget::OnClickedMainMenu()
{
	UE_LOG(LogPauseMenu, Log, TEXT("OnClickedMainMenuFromVisitor: invoked"));

	AFishingGameMode* GameMode = Cast<AFishingGameMode>(UGameplayStatics::GetGameMode(this));
	UE_LOG(LogPauseMenu, Verbose, TEXT("OnClickedMainMenuFromVisitor: GameMode=%s"),
		GameMode ? *GameMode->GetName() : TEXT("null"));

	if (!CachedMultiplayerController)
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("OnClickedMainMenuFromVisitor: CachedMultiplayerController is null"));
		return;
	}

	const bool bIsHost = CachedMultiplayerController->HasAuthority() && CachedMultiplayerController->IsLocalController();
	UE_LOG(LogPauseMenu, Log, TEXT("OnClickedMainMenuFromVisitor: bIsHost=%s"), bIsHost ? TEXT("true") : TEXT("false"));

	if (bIsHost)
	{
		if (GameMode)
		{
			UE_LOG(LogPauseMenu, Log, TEXT("OnClickedMainMenuFromVisitor: Host -> EndSessionAndReturnToMainMenu"));
			GameMode->EndSessionAndReturnToMainMenu();
		}
		else
		{
			UE_LOG(LogPauseMenu, Warning, TEXT("OnClickedMainMenuFromVisitor: GameMode is null (cannot end session)"));
		}
	}
	else
	{
		UE_LOG(LogPauseMenu, Log, TEXT("OnClickedMainMenuFromVisitor: Client -> LeaveSession"));
		CachedMultiplayerController->LeaveSession();
	}

	OnClosed();
	UE_LOG(LogPauseMenu, Log, TEXT("OnClickedMainMenuFromVisitor: Visibility -> Collapsed"));
}

void UPauseMenuWidget::OnClickedSave()
{
	UE_LOG(LogPauseMenu, Log, TEXT("OnClickedSave: invoked"));

	if (!CachedMultiplayerController || !CachedMultiplayerController->HasAuthority())
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("OnClickedSave: Not authority or controller null. Aborting."));
		return;
	}

	AFishingGameMode* GM = Cast<AFishingGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);
	if (GM)
	{
		UE_LOG(LogPauseMenu, Log, TEXT("OnClickedSave: Calling GM->SaveGame()"));
		GM->SaveGame();
	}
	else
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("OnClickedSave: AuthGameMode is null"));
	}

	OnClosed();
}

void UPauseMenuWidget::OnOpen()
{
	UE_LOG(LogPauseMenu, Log, TEXT("OnOpen: Opening Pause Menu (PrevVisibility=%s)"),
		VisibilityToString(GetVisibility()));

	SetVisibility(ESlateVisibility::Visible);

	if (CachedMultiplayerController)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(TakeWidget()); 
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		CachedMultiplayerController->SetInputMode(Mode);
		CachedMultiplayerController->SetShowMouseCursor(true);
	}
	else
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("OnOpen: CachedMultiplayerController is null"));
	}

	UE_LOG(LogPauseMenu, Log, TEXT("OnOpen: Visibility -> %s"),
		VisibilityToString(GetVisibility()));
}

void UPauseMenuWidget::OnClosed()
{
	UE_LOG(LogPauseMenu, Log, TEXT("OnClosed: Closing Pause Menu (PrevVisibility=%s)"),
		VisibilityToString(GetVisibility()));

	SetVisibility(ESlateVisibility::Collapsed);

	if (CachedMultiplayerController)
	{
		CachedMultiplayerController->SetShowMouseCursor(false);
		CachedMultiplayerController->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogPauseMenu, Log, TEXT("OnClosed: MouseCursor=false, InputMode=GameOnly"));
	}
	else
	{
		UE_LOG(LogPauseMenu, Warning, TEXT("OnClosed: CachedMultiplayerController is null"));
	}

	UE_LOG(LogPauseMenu, Log, TEXT("OnClosed: Visibility -> %s"),
		VisibilityToString(GetVisibility()));
}
