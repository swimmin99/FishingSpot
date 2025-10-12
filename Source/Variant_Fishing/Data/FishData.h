#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FishData.generated.h"

class UItemDataAsset;
class UItemBase;

UCLASS(BlueprintType)
class FISHING_API UFishData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish")
	FName FishID = TEXT("UnknownFish");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish")
	UStaticMesh* FishMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Gameplay")
	float BiteWindowTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Gameplay")
	float BiteFightingTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Gameplay")
	int32 MinFakeBites = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Gameplay")
	int32 MaxFakeBites = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Movement")
	float MoveSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Movement")
	float WanderRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Movement")
	float WanderDirectionChangeInterval = 3.0f;

	float BobberDetectionRange = 120.f;
	float DetectionViewAngle = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Visual")
	FVector MeshScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, Category="Behavior|Bite")
	float FakeBiteBackOffDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, Category="Behavior|Bite")
	float FakeBiteBackOffDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Inventory")
	UItemDataAsset* ItemData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Inventory")
	float Weight = 1.0f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("FishData", GetFName());
	}

	UItemBase* CreateItemFromDataAsset(UObject* Outer);
};
