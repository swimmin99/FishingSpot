
#include "SaveSlotItemWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Variant_Fishing/Widget/BaseButtonWidget.h"

void USaveSlotItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (LoadButton)
    {
        LoadButton->GetBaseButton()->OnClicked.AddDynamic(this, &USaveSlotItemWidget::OnLoadClicked);
        LoadButton->SetText(FText::FromString(TEXT("Load")));
    }

    if (DeleteButton)
    {
        DeleteButton->GetBaseButton()->OnClicked.AddDynamic(this, &USaveSlotItemWidget::OnDeleteClicked);
        DeleteButton->SetText(FText::FromString(TEXT("Delete")));

    }
}


void USaveSlotItemWidget::SetSaveSlotInfo(const FSaveSlotInfo& Info)
{
    SaveSlotInfo = Info;

    
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(Info.PlayerName));
    }

    if (VillageNameText)
    {
        VillageNameText->SetText(FText::FromString(Info.VillageName));
    }

    if (MoneyText)
    {
        MoneyText->SetText(FText::FromString(FString::Printf(TEXT("💰 %d"), Info.TotalMoney)));
    }

    if (LastSaveTimeText)
    {
        FString TimeText = Info.LastSaveTime.IsEmpty() ? TEXT("Never") : Info.LastSaveTime;
        LastSaveTimeText->SetText(FText::FromString(TimeText));
    }
}

void USaveSlotItemWidget::OnLoadClicked()
{
    OnSlotSelected.Broadcast(SaveSlotInfo.PlayerID);  
}

void USaveSlotItemWidget::OnDeleteClicked()
{
    OnSlotDeleted.Broadcast(SaveSlotInfo.VillageName);  
}