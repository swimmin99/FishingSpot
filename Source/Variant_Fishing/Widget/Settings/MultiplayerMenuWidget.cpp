#include "MultiplayerMenuWidget.h"
#include "SettingsPanelWidget.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerMenu, Log, All);

void UMultiplayerMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UMultiplayerMenuWidget::OnHostClicked);
		UE_LOG(LogMultiplayerMenu, Log, TEXT("✅ Host button bound"));
	}
	else
	{
		UE_LOG(LogMultiplayerMenu, Error, TEXT("❌ HostButton not found!"));
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMultiplayerMenuWidget::OnJoinClicked);
		UE_LOG(LogMultiplayerMenu, Log, TEXT("✅ Join button bound"));
	}
	else
	{
		UE_LOG(LogMultiplayerMenu, Error, TEXT("❌ JoinButton not found!"));
	}

	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UMultiplayerMenuWidget::OnBackClicked);
		UE_LOG(LogMultiplayerMenu, Log, TEXT("✅ Back button bound"));
	}
	else
	{
		UE_LOG(LogMultiplayerMenu, Error, TEXT("❌ BackButton not found!"));
	}

	UpdateHostButtonVisibility();
}

void UMultiplayerMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateHostButtonVisibility();
}

void UMultiplayerMenuWidget::OnHostClicked()
{
	UE_LOG(LogMultiplayerMenu, Log, TEXT("🖥️ Host clicked"));

	if (ParentPanel)
	{
		ParentPanel->ShowHostingMenu();
	}
}

void UMultiplayerMenuWidget::OnJoinClicked()
{
	UE_LOG(LogMultiplayerMenu, Log, TEXT("🔌 Join clicked"));

	if (ParentPanel)
	{
		ParentPanel->ShowJoinMenu();
	}
}

void UMultiplayerMenuWidget::OnBackClicked()
{
	UE_LOG(LogMultiplayerMenu, Log, TEXT("⬅️ Back clicked"));

	if (ParentPanel)
	{
		ParentPanel->GoBack();
	}
}

void UMultiplayerMenuWidget::UpdateHostButtonVisibility()
{
	if (!HostButton)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	if (PC->HasAuthority())
	{
		HostButton->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		HostButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}
