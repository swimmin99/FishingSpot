#include "UHUDWidget.h"

#include "BaseButtonWidget.h"
#include "FishingPlayerController.h"
#include "FishingPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "InteractableCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Inventory/ItemAcquiredBannerWidget.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SettingsButton)
	{
		SettingsButton->GetBaseButton()->OnClicked.AddDynamic(this, &UHUDWidget::OnClickSettings);
		SettingsButton->SetText(FText::FromString(FString(TEXT("Settings"))));
	}

	if (CloseButton)
	{
		CloseButton->GetBaseButton()->OnClicked.AddDynamic(this, &UHUDWidget::OnClickClose);
		CloseButton->SetText(FText::FromString(FString(TEXT("Close"))));
	}

	if (AInteractableCharacter* Char = GetOwningPlayerPawn<AInteractableCharacter>())
	{
		Char->OnGoldChanged.AddUniqueDynamic(this, &UHUDWidget::HandleGoldChanged);
	}

	if (BannerWidgetClass && !ItemBanner)
	{
		ItemBanner = CreateWidget<UItemAcquiredBannerWidget>(GetOwningPlayer(), BannerWidgetClass);
		if (ItemBanner)
		{
			NoticeVerticalBox->AddChild(ItemBanner);
            
			UE_LOG(LogTemp, Log, TEXT("HUDWidget: ItemBanner created"));
		}
	}

	HideCloseSection();

	RefreshMoney();
	RefreshTime();
}

void UHUDWidget::NativeDestruct()
{
	if (AInteractableCharacter* Char = GetOwningPlayerPawn<AInteractableCharacter>())
	{
		Char->OnGoldChanged.RemoveDynamic(this, &UHUDWidget::HandleGoldChanged);
	}
	Super::NativeDestruct();
}


void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshTime();
}

void UHUDWidget::AttachWidgetToRight(UWidget* Widget)
{
	if (!RightSection || !Widget)
	{
		return;
	}

	RightSection->ClearChildren();
	RightSection->AddChild(Widget);
	RightSection->SetVisibility(ESlateVisibility::Visible);

	ShowCloseSection();
}

void UHUDWidget::AttachWidgetToLeft(UWidget* Widget)
{
	if (!LeftSection || !Widget)
	{
		return;
	}

	LeftSection->ClearChildren();
	LeftSection->AddChild(Widget);
	LeftSection->SetVisibility(ESlateVisibility::Visible);

	ShowCloseSection();
}

void UHUDWidget::ClearWidgetInLeft()
{
	if (!LeftSection)
	{
		return;
	}

	LeftSection->ClearChildren();
	LeftSection->SetVisibility(ESlateVisibility::Hidden);

	HideCloseSection();
}

void UHUDWidget::ClearWidgetInRight()
{
	if (!RightSection)
	{
		return;
	}

	RightSection->ClearChildren();
	RightSection->SetVisibility(ESlateVisibility::Hidden);

	HideCloseSection();
}

void UHUDWidget::ShowCloseSection()
{
	if (WindowSection)
	{
		WindowSection->SetVisibility(ESlateVisibility::Visible);
	}
}

void UHUDWidget::HideCloseSection()
{
	if (WindowSection)
	{
		WindowSection->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UHUDWidget::RefreshMoney()
{
	if (!MoneyText)
	{
		return;
	}

	if (AInteractableCharacter* Char = GetOwningPlayerPawn<AInteractableCharacter>())
	{
		MoneyText->SetText(FText::FromString(FString::Printf(TEXT("Money: %d"), Char->GetGold())));
	}
}

void UHUDWidget::RefreshTime()
{
	if (!TimeText)
	{
		return;
	}

	AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetOwningPlayer());
	if (PC)
	{
		FString TimeStr = PC->GetLocalTimeString();
		TimeText->SetText(FText::FromString(TimeStr));
	}
}

void UHUDWidget::SetPlayerName(FString PlayerName)
{
	if (PlayerNameLabel)
	{
		PlayerNameLabel->SetText(FText::FromString(PlayerName));
	}
}

void UHUDWidget::OnClickSettings()
{
	UE_LOG(LogTemp, Log, TEXT("Settings button clicked"));
}

void UHUDWidget::OnClickClose()
{
	AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}

	if (PC->IsShopOpen())
	{
		PC->CloseShopUI();
	}
	else
	{
		PC->CloseInventory();
	}

	UE_LOG(LogTemp, Log, TEXT("Close button clicked - UI closed"));
}

void UHUDWidget::HandleGoldChanged(int32)
{
	RefreshMoney();
}

void UHUDWidget::ShowItemAcquiredBanner(UItemBase* Item)
{
	if (ItemBanner)
	{
		ItemBanner->ShowBanner(Item);
	}
}