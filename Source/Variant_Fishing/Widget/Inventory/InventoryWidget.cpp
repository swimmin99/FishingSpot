#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"

#include "CategoryFilterButton.h"
#include "InventoryGridWidget.h"
#include "Variant_Fishing/Data/ItemDragOperation.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryPortrait.h"
#include "Components/TextBlock.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Fishing.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Variant_Fishing/Data/ItemBase.h"


void UInventoryWidget::InitializeWidget(UInventoryComponent* InInventoryComponent, 
										AInteractableCharacter* InInteractableCharacter, 
										UMaterialInstanceDynamic* PortraitMaterial)
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
	ConnectedCharacter = InInteractableCharacter;
	
	InventoryGridWidget->InitializeWidget(InInventoryComponent);
	InventoryPortrait->InitializeWidget(InInteractableCharacter, PortraitMaterial);
	
	
	CreateCategoryFilterButtons();
	
	UE_LOG(LogInventoryWidget, Log, TEXT("InitializeWidget: Successfully connected"));
}

void UInventoryWidget::CreateCategoryFilterButtons()
{
	if (!CategoryFilterContainer || !CategoryFilterButtonClass)
	{
		return;
	}

	CategoryFilterContainer->ClearChildren();
	CategoryFilterButtons.Empty();

	TArray<EItemCategory> AllowedCategories = ConnectedInventoryComponent->GetAllowedCategoriesArray();

	for (EItemCategory Category : AllowedCategories)
	{
		
		UCategoryFilterButton* FilterButton = CreateWidget<UCategoryFilterButton>(
			GetWorld(), 
			CategoryFilterButtonClass
		);

		if (!FilterButton)
		{
			continue;
		}

		
		FString ButtonID = StaticEnum<EItemCategory>()->GetNameStringByValue((int64)Category);
		FText DisplayText = GetCategoryDisplayName(Category);
		bool bIsSelected = (Category == EItemCategory::All);

		FilterButton->InitializeButton(ButtonID, DisplayText, bIsSelected);

		
		FilterButton->OnFilterSelected.AddDynamic(this, &UInventoryWidget::OnCategoryFilterSelected);

		CategoryFilterContainer->AddChild(FilterButton);
		CategoryFilterButtons.Add(FilterButton);
	}
}

void UInventoryWidget::OnCategoryFilterSelected(FString ButtonID)
{
	
	int64 EnumValue = StaticEnum<EItemCategory>()->GetValueByNameString(ButtonID);
	EItemCategory SelectedCategory = static_cast<EItemCategory>(EnumValue);

	CurrentSelectedCategory = SelectedCategory;

	
	for (UCategoryFilterButton* Button : CategoryFilterButtons)
	{
		if (Button)
		{
			Button->SetSelected(Button->GetButtonID() == ButtonID);
		}
	}

	
	if (InventoryGridWidget)
	{
		InventoryGridWidget->SetCategoryFilter(SelectedCategory);
	}
}

FText UInventoryWidget::GetCategoryDisplayName(EItemCategory Category) const
{
	switch (Category)
	{
	case EItemCategory::All:        return FText::FromString(TEXT("All"));
	case EItemCategory::Fish:       return FText::FromString(TEXT("ðŸŸ Fish"));
	case EItemCategory::Equipment:  return FText::FromString(TEXT("âš”ï¸ Equip"));
	case EItemCategory::Consumable: return FText::FromString(TEXT("ðŸŽ Consum"));
	case EItemCategory::Material:   return FText::FromString(TEXT("ðŸªµ Material"));
	case EItemCategory::Quest:      return FText::FromString(TEXT("ðŸ“œ Quest"));
	case EItemCategory::Misc:       return FText::FromString(TEXT("ðŸ“¦ Misc"));
	default:                        return FText::FromString(TEXT("???"));
	}
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