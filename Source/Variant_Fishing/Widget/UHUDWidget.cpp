#include "UHUDWidget.h"

#include "FishingPlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "InteractableCharacter.h"
#include "Components/CanvasPanel.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UHUDWidget::OnClickSettings);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UHUDWidget::OnClickClose);
	}

	if (AInteractableCharacter* Char = GetOwningPlayerPawn<AInteractableCharacter>())
	{
		Char->OnGoldChanged.AddUniqueDynamic(this, &UHUDWidget::HandleGoldChanged);
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
