#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Variant_Fishing/Data/ItemSpecificData.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "FishData.generated.h"

class UItemBase;

UCLASS(BlueprintType)
class FISHING_API UFishData : public UPrimaryDataAsset, public IItemDataProvider
{
    GENERATED_BODY()

public:


    
    
    
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Display")
    FText DisplayName = FText::FromString(TEXT("Unknown Fish"));

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Display")
    FText Description = FText::FromString(TEXT("A mysterious fish."));

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Display")
    UMaterialInterface* Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Economy")
    int32 Price = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Inventory")
    FIntPoint BaseDimensions = FIntPoint(2, 1);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Inventory")
    EItemRarity Rarity = EItemRarity::Bronze;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World")
    UStaticMesh* WorldMesh = nullptr;

    
    virtual FText GetDisplayName_Implementation() const override { return DisplayName; }
    virtual FText GetDescription_Implementation() const override { return Description; }
    virtual UMaterialInterface* GetIcon_Implementation() const override { return Icon; }
    virtual int32 GetPrice_Implementation() const override { return Price; }
    virtual FIntPoint GetBaseDimensions_Implementation() const override { return BaseDimensions; }
    virtual EItemCategory GetCategory_Implementation() const override { return EItemCategory::Fish; }
    virtual EItemRarity GetRarity_Implementation() const override { return Rarity; }
    virtual UStaticMesh* GetWorldMesh_Implementation() const override { return WorldMesh; }
    virtual UItemBase* CreateBaseItem_Implementation(UObject* Outer, const FString& LocationName = TEXT("Unknown")) override { return CreateRandomFishItem(Outer, LocationName); } 
    
    
    
    
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Movement")
    float BobberDetectionRange = 120.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Movement")
    float DetectionViewAngle = 0.5f;

    
    
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Visual")
    FVector MeshScale = FVector(1.0f);

    
    
    
    
    UPROPERTY(EditDefaultsOnly, Category="Fish|Behavior")
    float FakeBiteBackOffDistance = 100.f;

    UPROPERTY(EditDefaultsOnly, Category="Fish|Behavior")
    float FakeBiteBackOffDuration = 0.25f;

    
    
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Stats", meta=(ClampMin="1.0", ClampMax="1000.0"))
    float MinLength = 20.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Stats", meta=(ClampMin="1.0", ClampMax="1000.0"))
    float MaxLength = 100.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Stats", meta=(ClampMin="0.1", ClampMax="1000.0"))
    float MinWeight = 0.5f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Stats", meta=(ClampMin="0.1", ClampMax="1000.0"))
    float MaxWeight = 10.0f;

    
    
    
    
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("FishData", GetFName());
    }

    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Fish")
    FFishSpecificData GenerateRandomFishStats(const FString& LocationName = TEXT("Unknown")) const;

    
    UFUNCTION(BlueprintCallable, Category = "Fish")
    FFishSpecificData CreateFishStatsWithValues(float Length, float Weight, const FString& LocationName = TEXT("Unknown")) const;

    
    UFUNCTION(BlueprintCallable, Category = "Fish")
    UItemBase* CreateItemFromFishData(UObject* Outer, const FFishSpecificData& InFishData);

    
    UFUNCTION(BlueprintCallable, Category = "Fish")
    UItemBase* CreateRandomFishItem(UObject* Outer, const FString& LocationName = TEXT("Unknown"));


   

    
    
    
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
    {
        Super::PostEditChangeProperty(PropertyChangedEvent);
        
        if (MinLength > MaxLength)
        {
            MaxLength = MinLength;
        }
        
        if (MinWeight > MaxWeight)
        {
            MaxWeight = MinWeight;
        }
    }
#endif
};