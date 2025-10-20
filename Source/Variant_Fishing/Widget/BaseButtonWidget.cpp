


#include "Variant_Fishing/Widget/BaseButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UBaseButtonWidget::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;
	if (BaseButton)
	{
		BaseButton->SetColorAndOpacity(bIsSelected ? SelectedBGColor : UnselectedBGColor);
	}
	if (BaseText)
	{
		BaseText->SetColorAndOpacity(bIsSelected ? SelectedTextColor : UnselectedTextColor);
	}
}
