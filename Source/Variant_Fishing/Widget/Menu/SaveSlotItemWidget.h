
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/Database/DatabaseManager.h"  
#include "SaveSlotItemWidget.generated.h"

class UBaseButtonWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotSelected, int32, PlayerID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotDeleted, const FString&, VillageName);


UCLASS()
class FISHING_API USaveSlotItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "SaveSlot")
	void SetSaveSlotInfo(const FSaveSlotInfo& Info);  

	
	UPROPERTY(BlueprintAssignable, Category = "SaveSlot")
	FOnSlotSelected OnSlotSelected;

	UPROPERTY(BlueprintAssignable, Category = "SaveSlot")
	FOnSlotDeleted OnSlotDeleted;

protected:
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* VillageNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MoneyText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LastSaveTimeText;

	UPROPERTY(meta = (BindWidget))
	UBaseButtonWidget* LoadButton;

	UPROPERTY(meta = (BindWidget))
	UBaseButtonWidget* DeleteButton;

	UFUNCTION()
	void OnLoadClicked();

	UFUNCTION()
	void OnDeleteClicked();

private:
	FSaveSlotInfo SaveSlotInfo;  
};