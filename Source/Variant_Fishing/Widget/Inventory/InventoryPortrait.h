
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryPortrait.generated.h"

class AInteractableCharacter;
class UImage;

UCLASS()
class FISHING_API UInventoryPortrait : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeWidget(AInteractableCharacter* InteractableCharacter, UMaterialInstanceDynamic* AlphaInvertMaterial);

protected:
	UPROPERTY(meta=(BindWidget))
	UImage* PortraitImage;

	UPROPERTY()
	TWeakObjectPtr<AInteractableCharacter> ConnectedCharacter;
};