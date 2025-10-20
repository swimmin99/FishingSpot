
#include "LeaderboardEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"

void ULeaderboardEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void ULeaderboardEntryWidget::SetEntryData(const FLeaderboardEntry& Entry)
{
    
    if (RankText)
    {
        RankText->SetText(FText::AsNumber(Entry.Rank));
    }

    
    UpdateMedalIcon(Entry.Rank);

    
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(Entry.PlayerName));
    }

    
    if (FishNameText)
    {
        FishNameText->SetText(FText::FromString(Entry.FishName));
    }

    
    if (LengthText)
    {
        LengthText->SetText(FText::FromString(
            FString::Printf(TEXT("%.1f cm"), Entry.Length)
        ));
    }

    
    if (WeightText)
    {
        WeightText->SetText(FText::FromString(
            FString::Printf(TEXT("%.2f kg"), Entry.Weight)
        ));
    }

    
    if (CountText)
    {
        CountText->SetText(FText::FromString(
            FString::Printf(TEXT("%d"), Entry.CaughtCount)
        ));
    }

    
    if (DateText)
    {
        DateText->SetText(FText::FromString(Entry.CaughtDate));
    }

    
    

    
    UpdateHighlight(Entry.bIsLocalPlayer);
}

void ULeaderboardEntryWidget::UpdateMedalIcon(int32 Rank)
{
    if (!MedalIcon)
    {
        return;
    }

    
    
    switch (Rank)
    {
    case 1:
    case 2:
    case 3:
        MedalIcon->SetVisibility(ESlateVisibility::Visible);
        break;
    default:
        MedalIcon->SetVisibility(ESlateVisibility::Collapsed);
        break;
    }
}

void ULeaderboardEntryWidget::UpdateHighlight(bool bIsLocalPlayer)
{
    if (!BackgroundBorder)
    {
        return;
    }

    FLinearColor Color = bIsLocalPlayer ? HighlightColor : NormalColor;
    BackgroundBorder->SetBrushColor(Color);
}