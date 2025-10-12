#include "Variant_Fishing/Widget/Settings/JoiningMenuWidget.h"

#include "SettingsPanelWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogJoinMenu, Log, All);

void UJoiningMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UJoiningMenuWidget::OnJoinButtonClicked);
		UE_LOG(LogJoinMenu, Log, TEXT("✅ Join button bound"));
	}
	else
	{
		UE_LOG(LogJoinMenu, Error, TEXT("❌ JoinButton not found!"));
	}

	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UJoiningMenuWidget::OnBackClicked);
		UE_LOG(LogJoinMenu, Log, TEXT("✅ Back button bound"));
	}
	else
	{
		UE_LOG(LogJoinMenu, Error, TEXT("❌ BackButton not found!"));
	}

	if (!IPAddressInput)
	{
		UE_LOG(LogJoinMenu, Error, TEXT("❌ IPAddressInput not found!"));
	}
	else
	{
		IPAddressInput->SetText(FText::FromString("127.0.0.1"));
	}

	if (!StatusText)
	{
		UE_LOG(LogJoinMenu, Error, TEXT("❌ StatusText not found!"));
	}

	ClearStatus();
}

void UJoiningMenuWidget::OnJoinButtonClicked()
{
	if (!IPAddressInput)
	{
		ShowStatus("IP Address input not found!", FLinearColor::Red);
		return;
	}

	FString IPAddress = IPAddressInput->GetText().ToString().TrimStartAndEnd();

	if (IPAddress.IsEmpty())
	{
		ShowStatus("Please enter an IP address", FLinearColor::Yellow);
		return;
	}

	UE_LOG(LogJoinMenu, Log, TEXT("🔌 Attempting to join: %s"), *IPAddress);
	ShowStatus("Connecting...", FLinearColor::White);

	AttemptJoin(IPAddress);
}

void UJoiningMenuWidget::OnBackClicked()
{
	UE_LOG(LogJoinMenu, Log, TEXT("⬅️ Back clicked"));

	if (ParentPanel)
	{
		ParentPanel->GoBack();
	}
}

void UJoiningMenuWidget::AttemptJoin(const FString& IPAddress)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		ShowStatus("Failed to get Player Controller", FLinearColor::Red);
		UE_LOG(LogJoinMenu, Error, TEXT("❌ Failed to get Player Controller"));
		return;
	}

	PC->ClientTravel(IPAddress, TRAVEL_Absolute);

	if (ParentPanel)
	{
		ParentPanel->HidePanel();
	}

	UE_LOG(LogJoinMenu, Log, TEXT("✅ Travel initiated to: %s"), *IPAddress);
}

void UJoiningMenuWidget::ShowStatus(const FString& Message, const FLinearColor& Color)
{
	if (!StatusText)
	{
		return;
	}

	StatusText->SetText(FText::FromString(Message));
	StatusText->SetColorAndOpacity(Color);

	UE_LOG(LogJoinMenu, Log, TEXT("📢 Status: %s"), *Message);
}

void UJoiningMenuWidget::ClearStatus()
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(""));
	}
}
