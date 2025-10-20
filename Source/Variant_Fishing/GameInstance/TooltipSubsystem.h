
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TooltipSubsystem.generated.h"

class UToolTipWidget;
class UItemBase;


UCLASS()
class FISHING_API UTooltipSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	

	
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void ShowTooltip(FToolTipData Data, FVector2D MousePos);
	
	
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void UpdateTooltipPosition(FVector2D MousePos);
	
	
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void HideTooltip();

	
	UFUNCTION(BlueprintPure, Category = "Tooltip")
	bool IsTooltipVisible() const;

	
	static UTooltipSubsystem* Get(const UWorld* World);

	
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void SetTooltipWidgetClass(TSubclassOf<UToolTipWidget> WidgetClass);

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tooltip")
	TSubclassOf<UToolTipWidget> TooltipWidgetClass;

	
	UPROPERTY()
	UToolTipWidget* TooltipWidget;


private:
	
	void LazyInitializeTooltip();

	
	bool IsTooltipValid() const;
};