
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "CategoryFilterButton.generated.h"

class UBaseButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFilterSelected, FString, ButtonID);

UCLASS()
class FISHING_API UCategoryFilterButton : public UUserWidget
{
	GENERATED_BODY()
	
	public:
	
	
	
	
	
	UPROPERTY(BlueprintAssignable, Category = "Filter Button")
	FOnFilterSelected OnFilterSelected;

	
	
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Filter Button")
	void InitializeButton(const FString& InButtonID, const FText& InDisplayText, bool bSelected = false);

	
	UFUNCTION(BlueprintPure, Category = "Filter Button")
	FString GetButtonID() const { return ButtonID; }

	
	UFUNCTION(BlueprintCallable, Category = "Filter Button")
	void SetSelected(bool bInSelected);

	
	UFUNCTION(BlueprintPure, Category = "Filter Button")
	bool IsSelected() const { return bIsSelected; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

	
	
	
	
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UBaseButtonWidget* Base = nullptr;

	
	
	
	
	
	UPROPERTY(EditAnywhere, Category = "Filter Button|Preview")
	FString DesignTimeButtonID = TEXT("PreviewButton");

	
	UPROPERTY(EditAnywhere, Category = "Filter Button|Preview")
	FText DesignTimeDisplayText = FText::FromString(TEXT("Preview"));

	
	UPROPERTY(EditAnywhere, Category = "Filter Button|Preview")
	bool bDesignTimeSelected = false;

private:
	
	
	
	
	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	FString ButtonID;

	UPROPERTY(Transient)
	bool bIsSelected = false;
};
