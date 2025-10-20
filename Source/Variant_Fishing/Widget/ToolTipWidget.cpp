


#include "Variant_Fishing/Widget/ToolTipWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "Fishing.h"
#include "Variant_Fishing/Data/ItemBase.h"

void UToolTipWidget::SetItemToolTipData(FToolTipData Data)
{


	if (TooltipNameText)
	{
		TooltipNameText->SetText(Data.Title);
	}

	if (TooltipCategoryText)
	{
		TooltipCategoryText->SetText(Data.Category);
	}
	
	
	if (TooltipDescriptionText)
	{
		TooltipDescriptionText->SetText(Data.Description);
	}

	
	SetVisibility(ESlateVisibility::HitTestInvisible);
    
}

void UToolTipWidget::ShowAtDPIScaled(const FVector2D& DesiredScreenPosDPI)
{
	
	const FVector2D TipSize = GetDesiredSize();
	const FVector2D ViewSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const float Margin = 8.f;

	
	
	

	
	UE_LOG(LogToolTip, Log, TEXT("===== Tooltip Debug ====="));
	UE_LOG(LogToolTip, Log, TEXT("RequestedPos: (%.2f, %.2f)"), DesiredScreenPosDPI.X, DesiredScreenPosDPI.Y);
	UE_LOG(LogToolTip, Log, TEXT("ViewportSize: (%.2f, %.2f)"), ViewSize.X, ViewSize.Y);
	UE_LOG(LogToolTip, Log, TEXT("TipSize: (%.2f, %.2f)"), TipSize.X, TipSize.Y);
	
	UE_LOG(LogToolTip, Log, TEXT("========================="));

	
	SetAlignmentInViewport(FVector2D(0.f, 0.f));
	SetPositionInViewport(DesiredScreenPosDPI, true);
}
void UToolTipWidget::ClearItemToolTipData()
{
	TooltipNameText->SetText(FText::GetEmpty());
	TooltipCategoryText->SetText(FText::GetEmpty());
	TooltipDescriptionText->SetText(FText::GetEmpty());
	SetVisibility(ESlateVisibility::Collapsed);
}

void UToolTipWidget::UpdateToolTipPosition(FVector2D Pos)
{
	if (GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		return;
	}
    
	ShowAtDPIScaled(Pos);
}
