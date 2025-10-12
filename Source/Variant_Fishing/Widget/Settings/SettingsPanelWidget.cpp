#include "SettingsPanelWidget.h"
#include "BasicMenuWidget.h"
#include "MultiplayerMenuWidget.h"
#include "HostingMenuWidget.h"
#include "JoiningMenuWidget.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSettingsPanel, Log, All);

void USettingsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BasicMenuWidget || !MultiplayerMenuWidget || !HostingMenuWidget || !JoinMenuWidget)
	{
		UE_LOG(LogSettingsPanel, Error, TEXT("One or more sub menus failed to bind!"));
		return;
	}

	BasicMenuWidget->SetParentPanel(this);
	MultiplayerMenuWidget->SetParentPanel(this);
	HostingMenuWidget->SetParentPanel(this);
	JoinMenuWidget->SetParentPanel(this);

	HideAllMenus();

	UE_LOG(LogSettingsPanel, Log, TEXT("✅ SettingsPanel constructed successfully"));
}

void USettingsPanelWidget::TogglePanel()
{
	if (bIsPanelOpen)
	{
		HidePanel();
	}
	else
	{
		ShowPanel();
	}
}

void USettingsPanelWidget::ShowPanel()
{
	if (bIsPanelOpen)
	{
		return;
	}

	bIsPanelOpen = true;

	ShowBasicMenu();

	SetupUIMode();

	SetVisibility(ESlateVisibility::Visible);

	UE_LOG(LogSettingsPanel, Log, TEXT("⚙️ Settings Panel opened"));
}

void USettingsPanelWidget::HidePanel()
{
	if (!bIsPanelOpen)
	{
		return;
	}

	bIsPanelOpen = false;

	MenuStack.Empty();
	CurrentMenu = nullptr;

	HideAllMenus();

	RestoreGameMode();

	SetVisibility(ESlateVisibility::Collapsed);

	UE_LOG(LogSettingsPanel, Log, TEXT("⚙️ Settings Panel closed"));
}

void USettingsPanelWidget::ShowBasicMenu()
{
	ShowMenu(BasicMenuWidget);
	UE_LOG(LogSettingsPanel, Log, TEXT("📋 Basic Menu shown"));
}

void USettingsPanelWidget::ShowMultiplayerMenu()
{
	ShowMenu(MultiplayerMenuWidget);
	UE_LOG(LogSettingsPanel, Log, TEXT("🌐 Multiplayer Menu shown"));
}

void USettingsPanelWidget::ShowHostingMenu()
{
	ShowMenu(HostingMenuWidget);
	UE_LOG(LogSettingsPanel, Log, TEXT("🖥️ Hosting Menu shown"));
}

void USettingsPanelWidget::ShowJoinMenu()
{
	ShowMenu(JoinMenuWidget);
	UE_LOG(LogSettingsPanel, Log, TEXT("🔌 Join Menu shown"));
}

void USettingsPanelWidget::GoBack()
{
	if (MenuStack.Num() == 0)
	{
		HidePanel();
		return;
	}

	UUserWidget* PreviousMenu = MenuStack.Pop();

	if (CurrentMenu)
	{
		CurrentMenu->SetVisibility(ESlateVisibility::Collapsed);
	}

	CurrentMenu = PreviousMenu;
	if (CurrentMenu)
	{
		CurrentMenu->SetVisibility(ESlateVisibility::Visible);
	}

	UE_LOG(LogSettingsPanel, Log, TEXT("⬅️ Navigated back"));
}

void USettingsPanelWidget::ShowMenu(UUserWidget* Menu)
{
	if (!Menu)
	{
		UE_LOG(LogSettingsPanel, Warning, TEXT("ShowMenu: Menu is null!"));
		return;
	}

	if (CurrentMenu && CurrentMenu != Menu)
	{
		CurrentMenu->SetVisibility(ESlateVisibility::Collapsed);
		MenuStack.Add(CurrentMenu);
	}

	CurrentMenu = Menu;
	CurrentMenu->SetVisibility(ESlateVisibility::Visible);
}

void USettingsPanelWidget::HideAllMenus()
{
	if (BasicMenuWidget)
	{
		BasicMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MultiplayerMenuWidget)
	{
		MultiplayerMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HostingMenuWidget)
	{
		HostingMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (JoinMenuWidget)
	{
		JoinMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USettingsPanelWidget::SetupUIMode()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void USettingsPanelWidget::RestoreGameMode()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	PC->SetShowMouseCursor(false);
	PC->SetInputMode(FInputModeGameOnly());
}
