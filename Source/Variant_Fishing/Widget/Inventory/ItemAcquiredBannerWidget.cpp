
#include "ItemAcquiredBannerWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Animation/WidgetAnimation.h"
#include "TimerManager.h"
#include "Variant_Fishing/Data/ItemBase.h"

void UItemAcquiredBannerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    
    SetVisibility(ESlateVisibility::Collapsed);
}

void UItemAcquiredBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UItemAcquiredBannerWidget::ShowBanner(UItemBase* Item)
{
    if (!Item)
    {
        return;
    }

    
    
    if (ItemNameText)
    {
        const FText DisplayName = Item->GetDisplayNameText();
        ItemNameText->SetText(DisplayName);
    }

    if (ItemDescriptionText)
    {
        FString ShortDesc = Item->GetDesc();
        if (ShortDesc.Len() > 50)
        {
            ShortDesc = ShortDesc.Left(47) + TEXT("...");
        }
        ItemDescriptionText->SetText(FText::FromString(ShortDesc));
    }

    if (ItemIcon)
    {
        UMaterialInterface* IconImage = Item->GetIcon();
        ItemIcon->SetBrushFromMaterial(IconImage);
    }

    
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    
    
    
    
    

    
    GetWorld()->GetTimerManager().SetTimer(
        HideTimerHandle,
        this,
        &UItemAcquiredBannerWidget::HideBanner,
        DisplayDuration,
        false
    );

    UE_LOG(LogTemp, Log, TEXT("ItemAcquiredBanner: Showing %s"), *Item->DisplayName());
}

void UItemAcquiredBannerWidget::HideBanner()
{
    
    OnFadeOutComplete();
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
}


void UItemAcquiredBannerWidget::OnFadeOutComplete()
{
    SetVisibility(ESlateVisibility::Collapsed);
    
    
    if (HideTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(HideTimerHandle);
    }
}