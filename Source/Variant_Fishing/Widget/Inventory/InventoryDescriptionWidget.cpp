#include "InventoryDescriptionWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Variant_Fishing/Data/ItemBase.h"

void UInventoryDescriptionWidget::UpdateContent(UItemBase* Item)
{
	if (!Item)
	{
		ClearContent();
		return;
	}

	const FString TitleStr = Item->DisplayName();
	const FString PriceStr = Item->GetPriceString();
	const FString DescStr = Item->Description();

	SetTitleText(TitleStr);
	SetPriceText(PriceStr);
	SetDescriptionText(DescStr);

	if (DescriptionScroll)
	{
		DescriptionScroll->SetScrollOffset(0.f);
	}
}

void UInventoryDescriptionWidget::ClearContent()
{
	ClearTitleText();
	ClearPriceText();
	ClearDescriptionText();

	if (DescriptionScroll)
	{
		DescriptionScroll->SetScrollOffset(0.f);
	}
}

void UInventoryDescriptionWidget::InitializeWidget(UInventoryComponent* InventoryComponent)
{
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryDescriptionWidget: InventoryComponent is null"));
		return;
	}

	LinkedInventoryComponent = InventoryComponent;
}

void UInventoryDescriptionWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UInventoryDescriptionWidget::SetTitleText(const FString& InTitle)
{
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(InTitle));
	}
}

void UInventoryDescriptionWidget::ClearTitleText()
{
	if (TitleText)
	{
		TitleText->SetText(FText::GetEmpty());
	}
}

void UInventoryDescriptionWidget::SetPriceText(const FString& InPrice)
{
	if (PriceValueText)
	{
		PriceLabelText->SetText(FText::FromString(MoneyChar));
		PriceValueText->SetText(FText::FromString(InPrice));
	}
}

void UInventoryDescriptionWidget::ClearPriceText()
{
	if (PriceValueText)
	{
		PriceLabelText->SetText(FText::GetEmpty());
		PriceValueText->SetText(FText::GetEmpty());
	}
}

void UInventoryDescriptionWidget::SetDescriptionText(const FString& InDesc)
{
	if (DescriptionText)
	{
		DescriptionText->SetText(FText::FromString(InDesc));
	}
}

void UInventoryDescriptionWidget::ClearDescriptionText()
{
	if (DescriptionText)
	{
		DescriptionText->SetText(FText::GetEmpty());
	}
}
