#include "Variant_Fishing/Data/FishData.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Fishing.h"

FFishSpecificData UFishData::GenerateRandomFishStats(const FString& LocationName) const
{
    FFishSpecificData Result;
    
    
    Result.ActualLength = FMath::FRandRange(MinLength, MaxLength);
    Result.ActualWeight = FMath::FRandRange(MinWeight, MaxWeight);
    
    
    Result.FishDataName = FishID.ToString();
    Result.CaughtLocation = LocationName;
    Result.CaughtTime = FDateTime::Now();
    
    
    Result.MinLength = MinLength;
    Result.MaxLength = MaxLength;
    Result.MinWeight = MinWeight;
    Result.MaxWeight = MaxWeight;
    
    UE_LOG(LogFishing, Log, TEXT("Generated Fish Stats: %s - %.1fcm, %.2fkg at %s"),
           *Result.FishDataName, Result.ActualLength, Result.ActualWeight, *Result.CaughtLocation);
    
    return Result;
}

FFishSpecificData UFishData::CreateFishStatsWithValues(float Length, float Weight, const FString& LocationName) const
{
    FFishSpecificData Result;
    
    
    Result.ActualLength = FMath::Clamp(Length, MinLength, MaxLength);
    Result.ActualWeight = FMath::Clamp(Weight, MinWeight, MaxWeight);
    
    
    Result.FishDataName = FishID.ToString();
    Result.CaughtLocation = LocationName;
    Result.CaughtTime = FDateTime::Now();
    
    
    Result.MinLength = MinLength;
    Result.MaxLength = MaxLength;
    Result.MinWeight = MinWeight;
    Result.MaxWeight = MaxWeight;
    
    UE_LOG(LogFishing, Log, TEXT("Created Fish Stats with Values: %s - %.1fcm, %.2fkg at %s"),
           *Result.FishDataName, Result.ActualLength, Result.ActualWeight, *Result.CaughtLocation);
    
    return Result;
}

UItemBase* UFishData::CreateItemFromFishData(UObject* Outer, const FFishSpecificData& InFishData)
{
    if (!InFishData.IsValid())
    {
        UE_LOG(LogFishing, Error, TEXT("UFishData::CreateItemFromFishData - Invalid FishData for %s"), 
               *FishID.ToString());
        return nullptr;
    }

    
    UItemBase* NewItem = NewObject<UItemBase>(Outer);
    if (!NewItem)
    {
        UE_LOG(LogFishing, Error, TEXT("UFishData::CreateItemFromFishData - Failed to create ItemBase"));
        return nullptr;
    }

    
    NewItem->ItemDataProvider = this;
    NewItem->ItemGuid = FGuid::NewGuid();
    
    
    NewItem->SetFishData(InFishData);

    UE_LOG(LogFishing, Log, TEXT("✅ Created Fish Item: %s - %s"),
           *DisplayName.ToString(), *InFishData.GetDisplayString());

    return NewItem;
}

UItemBase* UFishData::CreateRandomFishItem(UObject* Outer, const FString& LocationName)
{
    
    FFishSpecificData RandomStats = GenerateRandomFishStats(LocationName);
    
    
    return CreateItemFromFishData(Outer, RandomStats);
}