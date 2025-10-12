#include "BasicMenuWidget.h"
#include "SettingsPanelWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogBasicMenu, Log, All);

void UBasicMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UBasicMenuWidget::OnResumeClicked);
		UE_LOG(LogBasicMenu, Log, TEXT("✅ Resume button bound"));
	}
	else
	{
		UE_LOG(LogBasicMenu, Error, TEXT("❌ ResumeButton not found!"));
	}

	if (MultiplayerButton)
	{
		MultiplayerButton->OnClicked.AddDynamic(this, &UBasicMenuWidget::OnMultiplayerClicked);
		UE_LOG(LogBasicMenu, Log, TEXT("✅ Multiplayer button bound"));
	}
	else
	{
		UE_LOG(LogBasicMenu, Error, TEXT("❌ MultiplayerButton not found!"));
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UBasicMenuWidget::OnQuitClicked);
		UE_LOG(LogBasicMenu, Log, TEXT("✅ Quit button bound"));
	}
	else
	{
		UE_LOG(LogBasicMenu, Error, TEXT("❌ QuitButton not found!"));
	}
}

void UBasicMenuWidget::OnResumeClicked()
{
	UE_LOG(LogBasicMenu, Log, TEXT("▶️ Resume clicked"));

	if (ParentPanel)
	{
		ParentPanel->HidePanel();
	}
}

void UBasicMenuWidget::OnMultiplayerClicked()
{
	UE_LOG(LogBasicMenu, Log, TEXT("🌐 Multiplayer clicked"));

	if (ParentPanel)
	{
		ParentPanel->ShowMultiplayerMenu();
	}
}

void UBasicMenuWidget::OnQuitClicked()
{
	UE_LOG(LogBasicMenu, Log, TEXT("🚪 Quit clicked"));

	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}
