#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	Item UMETA(DisplayName = "Item"),
	Shop UMETA(DisplayName = "Shop"),
	Exchange UMETA(DisplayName = "Exchange")
};

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class FISHING_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FString GetInteractableName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractionType GetType() const;
};
