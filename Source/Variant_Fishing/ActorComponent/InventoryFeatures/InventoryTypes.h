#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Variant_Fishing/Data/ItemSpecificData.h"
#include "InventoryTypes.generated.h"


USTRUCT(BlueprintType)
struct FItemSyncData
{
    GENERATED_BODY()
    
    UPROPERTY()
    FGuid ItemGuid;

    
    UPROPERTY()
    FSoftObjectPath DataAssetPath;  

    UPROPERTY()
    bool bIsRotated = false;

    UPROPERTY()
    int32 TopLeftIndex = -1;

    UPROPERTY()
    FItemSpecificData SpecificData;

    
    FItemSyncData()
        : ItemGuid()
        , DataAssetPath()
        , bIsRotated(false)
        , TopLeftIndex(-1)
        , SpecificData()
    {
    }

    FItemSyncData(
        const FGuid& InGuid,
        const FSoftObjectPath& InDataAssetPath,  
        bool InRotated,
        int32 InIndex,
        const FItemSpecificData& InSpecificData = FItemSpecificData()
    )
        : ItemGuid(InGuid)
        , DataAssetPath(InDataAssetPath)  
        , bIsRotated(InRotated)
        , TopLeftIndex(InIndex)
        , SpecificData(InSpecificData)
    {
    }

    explicit FItemSyncData(const class UItemBase* Item, int32 InIndex);

    bool IsValid() const
    {
        return ItemGuid.IsValid() && DataAssetPath.IsValid() && TopLeftIndex >= 0;
    }

    FString ToString() const
    {
        return FString::Printf(TEXT("SyncData: GUID=%s, Asset=%s, Rotated=%s, Index=%d"),
            *ItemGuid.ToString(),
            *DataAssetPath.ToString(),
            bIsRotated ? TEXT("Yes") : TEXT("No"),
            TopLeftIndex);
    }
};