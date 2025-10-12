#include "ShopInventoryWidget.h"
#include "Variant_Fishing/Data/ShopTransactionData.h"
#include "ShopInventoryGridWidget.h"
#include "FishingCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Variant_Fishing/ActorComponent/InventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogShopWidget, Log, All);

void UShopInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UShopInventoryWidget::OnConfirmButtonClicked);
		UE_LOG(LogShopWidget, Log, TEXT("NativeConstruct: Confirm Button bound"));
	}

	if (UShopInventoryGridWidget* ShopGrid = Cast<UShopInventoryGridWidget>(InventoryGridWidget))
	{
		if (TransactionManager)
		{
			ShopGrid->SetTransactionManager(TransactionManager);
		}
	}
}

void UShopInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdatePriceDisplay();
	UpdateConfirmButtonState();
}

void UShopInventoryWidget::SetTransactionManager(UShopTransactionManager* InManager)
{
	TransactionManager = InManager;

	UE_LOG(LogShopWidget, Log, TEXT("SetTransactionManager: Manager=%s"),
	       InManager ? TEXT("Valid") : TEXT("NULL"));

	if (UShopInventoryGridWidget* ShopGrid = Cast<UShopInventoryGridWidget>(InventoryGridWidget))
	{
		ShopGrid->SetTransactionManager(InManager);
	}
}

void UShopInventoryWidget::UpdatePriceDisplay()
{
	if (!PriceText || !TransactionManager)
	{
		return;
	}

	int32 Price = 0;
	FString Label;

	if (bIsPlayerInventory)
	{
		Price = TransactionManager->GetTotalBuyPrice();
		Label = FString::Printf(TEXT("Buy: %d G"), Price);
	}
	else
	{
		Price = TransactionManager->GetTotalSellPrice();
		Label = FString::Printf(TEXT("Sell: %d G"), Price);
	}

	PriceText->SetText(FText::FromString(Label));
}

void UShopInventoryWidget::UpdateConfirmButtonState()
{
	if (!ConfirmButton || !TransactionManager)
	{
		return;
	}

	bool bHasTransaction = false;

	if (bIsPlayerInventory)
	{
		bHasTransaction = (TransactionManager->GetTotalBuyPrice() > 0);
	}
	else
	{
		bHasTransaction = (TransactionManager->GetTotalSellPrice() > 0);
	}

	ConfirmButton->SetIsEnabled(bHasTransaction);
}

void UShopInventoryWidget::OnConfirmButtonClicked()
{
	if (!TransactionManager)
	{
		UE_LOG(LogShopWidget, Error, TEXT("OnConfirmButtonClicked: TransactionManager is null"));
		return;
	}

	AFishingCharacter* Player = Cast<AFishingCharacter>(GetOwningPlayerPawn());
	if (!Player)
	{
		UE_LOG(LogShopWidget, Error, TEXT("OnConfirmButtonClicked: Player is null"));
		return;
	}

	if (bIsPlayerInventory)
	{
		UE_LOG(LogShopWidget, Log, TEXT("OnConfirmButtonClicked: Confirming BUY"));
		TransactionManager->ConfirmBuy(Player);
	}
	else
	{
		UE_LOG(LogShopWidget, Log, TEXT("OnConfirmButtonClicked: Confirming SELL"));
		TransactionManager->ConfirmSell(Player);
	}

	UpdatePriceDisplay();
	UpdateConfirmButtonState();
}
