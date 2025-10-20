
#include "CategoryFilterButton.h"
#include "Variant_Fishing/Widget/BaseButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCategoryFilterButton::NativeConstruct()
{
	Super::NativeConstruct();

	
	if (Base && Base->BaseButton)
	{
		Base->BaseButton->OnClicked.Clear();
		Base->BaseButton->OnClicked.AddDynamic(this, &UCategoryFilterButton::HandleClicked);
	}
}

void UCategoryFilterButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	
	if (!IsDesignTime())
	{
		return;
	}

	InitializeButton(DesignTimeButtonID, DesignTimeDisplayText, bDesignTimeSelected);
}

void UCategoryFilterButton::InitializeButton(const FString& InButtonID, const FText& InDisplayText, bool bSelected)
{
	ButtonID = InButtonID;

	
	if (Base && Base->BaseText)
	{
		Base->BaseText->SetText(InDisplayText);
	}

	
	SetSelected(bSelected);
}

void UCategoryFilterButton::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;

	
	if (Base)
	{
		Base->SetSelected(bInSelected);
	}
}

void UCategoryFilterButton::HandleClicked()
{
	
	OnFilterSelected.Broadcast(ButtonID);
}