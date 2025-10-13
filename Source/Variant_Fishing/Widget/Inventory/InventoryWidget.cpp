#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"

#include "InventoryGridWidget.h"
#include "Variant_Fishing/Data/ItemDragOperation.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDescriptionWidget.h"
#include "Components/TextBlock.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogInventoryWidget, Log, All);

void UInventoryWidget::InitializeWidget(UInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("InitializeWidget: InInventoryComponent is null"));
		return;
	}

	if (!InventoryGridWidget)
	{
		UE_LOG(LogInventoryWidget, Error, TEXT("InitializeWidget: InventoryGridWidget is null (check BindWidget)"));
		return;
	}

	UE_LOG(LogInventoryWidget, Log, TEXT("InitializeWidget: Connecting to InventoryComponent from %s"),
	       InInventoryComponent->GetOwner() ? *InInventoryComponent->GetOwner()->GetName() : TEXT("Unknown"));

	ConnectedInventoryComponent = InInventoryComponent;

	InventoryGridWidget->InitializeWidget(InInventoryComponent);
	ItemDescriptionWidget->InitializeWidget(InInventoryComponent);
	UE_LOG(LogInventoryWidget, Log, TEXT("InitializeWidget: Successfully connected"));
}

void UInventoryWidget::SetLabel(const FText& InText)
{
	if (TextTitle)
	{
		TextTitle->SetText(InText);
	}
}

void UInventoryWidget::SetFocusGridWidget()
{
	if (!InventoryGridWidget)
	{
		return;
	}
	InventoryGridWidget->SetUserFocus(GetOwningPlayer());
}

void UInventoryWidget::RefreshGrid()
{
	if (!InventoryGridWidget)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("RefreshGrid: Inventory Grid Widget Invalid"));
		return;
	}
	InventoryGridWidget->Refresh();
}

void UInventoryWidget::UpdateDescription(UItemBase* TargetItem)
{
	if (!ItemDescriptionWidget)
	{
		return;
	}
	ItemDescriptionWidget->UpdateContent(TargetItem);
}

void UInventoryWidget::ClearDescription()
{
	if (!ItemDescriptionWidget)
	{
		return;
	}
	ItemDescriptionWidget->ClearContent();
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogInventoryWidget, Log, TEXT("NativeConstruct: InventoryWidget created"));
}

FReply UInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();
}

bool UInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                    UDragDropOperation* InOperation)
{
	if (bDroppable == false)
	{
		return false;
	}

	UE_LOG(LogInventoryWidget, Warning, TEXT("==== [InventoryWidget::OnDrop] START ===="));

	if (!InOperation)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("OnDrop -> InOperation is nullptr"));
		return false;
	}

	auto* MyOp = Cast<UItemDragOperation>(InOperation);
	if (!MyOp)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("OnDrop -> Cast to UItemDragOperation failed"));
		return false;
	}

	if (!MyOp->Item)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("OnDrop -> MyOp->Item is nullptr"));

		return false;
	}

	if (!ConnectedInventoryComponent)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("OnDrop -> InventoryComponent nullptr"));
		return false;
	}

	if (!ConnectedInventoryComponent)
	{
		UE_LOG(LogInventoryWidget, Warning, TEXT("OnDrop -> CharacterReference nullptr"));
		return false;
	}

	UE_LOG(LogInventoryWidget, Warning, TEXT("OnDrop -> Attempting RemoveItem for %s"), *MyOp->Item->GetName());
	ConnectedInventoryComponent->Server_DropItemToWorld(MyOp->Item);

	UE_LOG(LogInventoryWidget, Warning, TEXT("==== [InventoryWidget::OnDrop] END ===="));

	return true;
}
