

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ToolTipWidget.generated.h"


USTRUCT(BlueprintType)
struct FToolTipData
{
	GENERATED_BODY()
	FToolTipData()
	: Title(FText::GetEmpty())
	, Category(FText::GetEmpty())
	, Description(FText::GetEmpty())
	{}

	FToolTipData(const FText& InTitle, const FText& InCategory, const FText& InDescription)
			: Title(InTitle)
			, Category(InCategory)
			, Description(InDescription)
	{}
	
	UPROPERTY()
	FText Title;

	UPROPERTY()
	FText Category;

	UPROPERTY()
	FText Description;
};



class UTextBlock;

UCLASS()
class FISHING_API UToolTipWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Item ToolTip")
	void SetItemToolTipData(FToolTipData Item);
	void ShowAtDPIScaled(const FVector2D& DesiredScreenPosDPI);

	UFUNCTION(BlueprintCallable, Category = "Item ToolTip")
	void ClearItemToolTipData();
	void UpdateToolTipPosition(FVector2D Pos);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
	UTextBlock* TooltipNameText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
	UTextBlock* TooltipCategoryText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
	UTextBlock* TooltipDescriptionText;
};
