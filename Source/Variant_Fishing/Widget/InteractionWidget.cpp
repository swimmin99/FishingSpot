#include "InteractionWidget.h"
#include "Components/TextBlock.h"

void UInteractionWidget::SetLabel(const FText& InText)
{
	if (Text)
	{
		Text->SetText(InText);
	}
}
