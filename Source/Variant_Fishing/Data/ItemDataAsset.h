#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UFishData;
class UItemBase;

UCLASS(BlueprintType)
class FISHING_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Display")
	FText DisplayName = FText::FromString(TEXT("Unknown Item"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Display")
	FText Description = FText::FromString(TEXT("A item description"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Display")
	int32 Price = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Inventory")
	FIntPoint BaseDimensions = FIntPoint(1, 1);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Inventory")
	UMaterialInterface* Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Fish")
	UFishData* SourceFishData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSubclassOf<UItemBase> ItemClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	UStaticMesh* WorldMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	float SphereRadius = 0.f;
};
