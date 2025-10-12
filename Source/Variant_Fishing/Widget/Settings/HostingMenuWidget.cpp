#include "HostingMenuWidget.h"
#include "SettingsPanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogHostingMenu, Log, All);

void UHostingMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UHostingMenuWidget::OnHostButtonClicked);
		UE_LOG(LogHostingMenu, Log, TEXT("✅ Host button bound"));
	}
	else
	{
		UE_LOG(LogHostingMenu, Error, TEXT("❌ HostButton not found!"));
	}

	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UHostingMenuWidget::OnBackClicked);
		UE_LOG(LogHostingMenu, Log, TEXT("✅ Back button bound"));
	}
	else
	{
		UE_LOG(LogHostingMenu, Error, TEXT("❌ BackButton not found!"));
	}

	if (!StatusText)
	{
		UE_LOG(LogHostingMenu, Error, TEXT("❌ StatusText not found!"));
	}

	UpdateUI();
}

void UHostingMenuWidget::OnHostButtonClicked()
{
	if (bIsHosting)
	{
		StopHosting();
	}
	else
	{
		StartHosting();
	}

	UpdateUI();
}

void UHostingMenuWidget::OnBackClicked()
{
	UE_LOG(LogHostingMenu, Log, TEXT("⬅️ Back clicked"));

	if (ParentPanel)
	{
		ParentPanel->GoBack();
	}
}

void UHostingMenuWidget::StartHosting()
{
	UE_LOG(LogHostingMenu, Log, TEXT("🖥️ Starting host..."));

	bIsHosting = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogHostingMenu, Error, TEXT("❌ World is null!"));
		return;
	}

	FString CurrentLevel = World->GetMapName();
	CurrentLevel.RemoveFromStart(World->StreamingLevelsPrefix);
	UGameplayStatics::OpenLevel(World, FName(*CurrentLevel), true, "listen");

	UE_LOG(LogHostingMenu, Log, TEXT("✅ Hosting started on level: %s"), *CurrentLevel);
}

void UHostingMenuWidget::StopHosting()
{
	UE_LOG(LogHostingMenu, Log, TEXT("⏹️ Stopping host..."));

	bIsHosting = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogHostingMenu, Error, TEXT("❌ World is null!"));
		return;
	}

	FString CurrentLevel = World->GetMapName();
	CurrentLevel.RemoveFromStart(World->StreamingLevelsPrefix);

	UGameplayStatics::OpenLevel(World, FName(*CurrentLevel), false, "");

	UE_LOG(LogHostingMenu, Log, TEXT("✅ Hosting stopped"));
}

void UHostingMenuWidget::UpdateUI()
{
	if (!StatusText)
	{
		return;
	}

	if (bIsHosting)
	{
		StatusText->SetText(FText::FromString("Hosting..."));
	}
	else
	{
		StatusText->SetText(FText::FromString("Currently Not Hosting"));
	}
}
