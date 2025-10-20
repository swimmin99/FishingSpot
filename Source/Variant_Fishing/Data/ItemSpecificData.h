
#pragma once

#include "CoreMinimal.h"
#include "ItemSpecificData.generated.h"




UENUM(BlueprintType)
enum class EItemSpecificDataType : uint8
{
    None        UMETA(DisplayName = "None"),
    Fish        UMETA(DisplayName = "Fish"),
    Equipment   UMETA(DisplayName = "Equipment"),
    Consumable  UMETA(DisplayName = "Consumable")
};




USTRUCT(BlueprintType)
struct FFishSpecificData
{
    GENERATED_BODY()

    
    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    float ActualLength = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    float ActualWeight = 0.0f;

    
    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    FString FishDataName;

    
    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    FString CaughtLocation;

    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    FDateTime CaughtTime;

    
    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    float MinLength = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    float MaxLength = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    float MinWeight = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Fish")
    float MaxWeight = 0.0f;

    
    bool IsValid() const 
    { 
        return ActualLength > 0.0f && ActualWeight > 0.0f && !FishDataName.IsEmpty(); 
    }

    
    float GetLengthPercentile() const
    {
        if (MaxLength <= MinLength) return 0.0f;
        return FMath::Clamp((ActualLength - MinLength) / (MaxLength - MinLength), 0.0f, 1.0f);
    }

    float GetWeightPercentile() const
    {
        if (MaxWeight <= MinWeight) return 0.0f;
        return FMath::Clamp((ActualWeight - MinWeight) / (MaxWeight - MinWeight), 0.0f, 1.0f);
    }

    
    FString GetDisplayString() const
    {
        return FString::Printf(TEXT("%s - %.1fcm, %.2fkg"), 
            *FishDataName, ActualLength, ActualWeight);
    }
};




USTRUCT(BlueprintType)
struct FEquipmentSpecificData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Equipment")
    float Durability = 100.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Equipment")
    float MaxDurability = 100.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Equipment")
    int32 EnhancementLevel = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Equipment")
    bool bIsEquipped = false;

    bool IsValid() const { return MaxDurability > 0.0f; }
};




USTRUCT(BlueprintType)
struct FConsumableSpecificData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Consumable")
    int32 StackCount = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Consumable")
    int32 MaxStackCount = 99;

    UPROPERTY(BlueprintReadWrite, Category = "Consumable")
    FDateTime ExpirationDate;

    bool IsValid() const { return StackCount > 0 && StackCount <= MaxStackCount; }
};




USTRUCT(BlueprintType)
struct FItemSpecificData
{
    GENERATED_BODY()

    
    UPROPERTY(BlueprintReadWrite, Category = "Type")
    EItemSpecificDataType ActiveType = EItemSpecificDataType::None;

    
    UPROPERTY(BlueprintReadWrite, Category = "Data")
    FFishSpecificData FishData;

    UPROPERTY(BlueprintReadWrite, Category = "Data")
    FEquipmentSpecificData EquipmentData;

    UPROPERTY(BlueprintReadWrite, Category = "Data")
    FConsumableSpecificData ConsumableData;

    
    
    

    bool IsFish() const { return ActiveType == EItemSpecificDataType::Fish; }
    bool IsEquipment() const { return ActiveType == EItemSpecificDataType::Equipment; }
    bool IsConsumable() const { return ActiveType == EItemSpecificDataType::Consumable; }
    bool HasSpecificData() const { return ActiveType != EItemSpecificDataType::None; }

    
    FFishSpecificData* GetFishData()
    {
        return IsFish() ? &FishData : nullptr;
    }

    const FFishSpecificData* GetFishData() const
    {
        return IsFish() ? &FishData : nullptr;
    }

    FEquipmentSpecificData* GetEquipmentData()
    {
        return IsEquipment() ? &EquipmentData : nullptr;
    }

    const FEquipmentSpecificData* GetEquipmentData() const
    {
        return IsEquipment() ? &EquipmentData : nullptr;
    }

    FConsumableSpecificData* GetConsumableData()
    {
        return IsConsumable() ? &ConsumableData : nullptr;
    }

    const FConsumableSpecificData* GetConsumableData() const
    {
        return IsConsumable() ? &ConsumableData : nullptr;
    }

    
    void SetFishData(const FFishSpecificData& InData)
    {
        ActiveType = EItemSpecificDataType::Fish;
        FishData = InData;
    }

    void SetEquipmentData(const FEquipmentSpecificData& InData)
    {
        ActiveType = EItemSpecificDataType::Equipment;
        EquipmentData = InData;
    }

    void SetConsumableData(const FConsumableSpecificData& InData)
    {
        ActiveType = EItemSpecificDataType::Consumable;
        ConsumableData = InData;
    }

    
    void Reset()
    {
        ActiveType = EItemSpecificDataType::None;
        FishData = FFishSpecificData();
        EquipmentData = FEquipmentSpecificData();
        ConsumableData = FConsumableSpecificData();
    }

    
    FString ToString() const
    {
        switch (ActiveType)
        {
        case EItemSpecificDataType::Fish:
            return FString::Printf(TEXT("Fish: %s"), *FishData.GetDisplayString());
        case EItemSpecificDataType::Equipment:
            return FString::Printf(TEXT("Equipment: Dur %.1f/%.1f, +%d"), 
                EquipmentData.Durability, EquipmentData.MaxDurability, EquipmentData.EnhancementLevel);
        case EItemSpecificDataType::Consumable:
            return FString::Printf(TEXT("Consumable: Stack %d/%d"), 
                ConsumableData.StackCount, ConsumableData.MaxStackCount);
        default:
            return TEXT("None");
        }
    }
};