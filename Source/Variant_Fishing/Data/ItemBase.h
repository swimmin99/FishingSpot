#pragma once
#include "CoreMinimal.h"
#include "ItemSpecificData.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "UObject/Object.h"
#include "FishData.h"

#include "ItemBase.generated.h"

class AItemActor;

UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew)
class FISHING_API UItemBase : public UObject
{
    GENERATED_BODY()

public:
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual bool IsNameStableForNetworking() const override { return true; }

    
    
    
    
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    FGuid ItemGuid;
    
    
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Item")
    TScriptInterface<IItemDataProvider> ItemDataProvider;
    
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Item")
    bool bIsRotated = false;

    
    
    
    
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Type Specific")
    FItemSpecificData SpecificData;

    
    
    
    
    

    
    
    
    
    UFUNCTION(BlueprintCallable, Category="Item")
    FIntPoint GetCurrentDimensions() const
    {
        if (!ItemDataProvider)
        {
            return FIntPoint::ZeroValue;
        }
        
        FIntPoint BaseDims = IItemDataProvider::Execute_GetBaseDimensions(ItemDataProvider.GetObject());
        return bIsRotated ? FIntPoint(BaseDims.Y, BaseDims.X) : BaseDims;
    }

    UFUNCTION(BlueprintCallable, Category="Item")
    bool GetIsRotated() const { return bIsRotated; }

    UFUNCTION(BlueprintCallable, Category="Item")
    void RotateItem() { bIsRotated = !bIsRotated; }


    
    
    
    
    
    FString DisplayName() const 
    { 
        if (!ItemDataProvider)
        {
            return TEXT("Unknown Item");
        }
        return IItemDataProvider::Execute_GetDisplayName(ItemDataProvider.GetObject()).ToString();
    }

    FString GetDisplayNameString () const
    {
        if (!ItemDataProvider)
        {
            return FString("Unknown Item");
        }
        return IItemDataProvider::Execute_GetDisplayName(ItemDataProvider.GetObject()).ToString();
    }

    FText GetDisplayNameText () const
    {
        if (!ItemDataProvider)
        {
            return FText::FromString("Unknown Item");
        }
        return IItemDataProvider::Execute_GetDisplayName(ItemDataProvider.GetObject());
    }
   
    
    FString GetPriceString() const 
    { 
        if (!ItemDataProvider)
        {
            return TEXT("0");
        }
        return FString::FromInt(IItemDataProvider::Execute_GetPrice(ItemDataProvider.GetObject()));
    }

    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetPrice() const 
    { 
        if (!ItemDataProvider)
        {
            return 0;
        }
        return IItemDataProvider::Execute_GetPrice(ItemDataProvider.GetObject());
    }

    FString Description() const 
    { 
        if (!ItemDataProvider)
        {
            return TEXT("No Description");
        }
        
        FString BaseDesc = IItemDataProvider::Execute_GetDescription(ItemDataProvider.GetObject()).ToString();
        
        if (SpecificData.IsFish())
        {
            const FFishSpecificData* FishData = SpecificData.GetFishData();
            if (FishData && FishData->IsValid())
            {
                BaseDesc += FString::Printf(TEXT("\n\nLength: %.1fcm\nWeight: %.2fkg\nCaught at: %s"),
                    FishData->ActualLength,
                    FishData->ActualWeight,
                    *FishData->CaughtLocation);
            }
        }
        
        return BaseDesc;
    }

    UMaterialInterface* GetIcon() 
    { 
        if (!ItemDataProvider)
        {
            return nullptr;
        }
        return IItemDataProvider::Execute_GetIcon(ItemDataProvider.GetObject());
    }

    UFUNCTION(BlueprintPure, Category = "Item")
    EItemCategory GetCategory() const
    {
        if (!ItemDataProvider)
        {
            return EItemCategory::Misc;
        }
        return IItemDataProvider::Execute_GetCategory(ItemDataProvider.GetObject());
    }

    UFUNCTION(BlueprintPure, Category = "Item")
    FString GetCategoryString() const
    {
        EItemCategory Category = GetCategory();
        switch (Category)
        {
            case EItemCategory::Fish:
                return TEXT("Fish");
            case EItemCategory::Equipment:
                return TEXT("Equipment");
            case EItemCategory::Consumable:
                return TEXT("Consumable");
            case EItemCategory::Material:
                return TEXT("Material");
            case EItemCategory::Quest:
                return TEXT("Quest");
            case EItemCategory::Misc:
            default:
                return TEXT("Misc");
        }
    }

    
    
    
    
    UFUNCTION(BlueprintPure, Category = "Item|Fish")
    bool IsFish() const
    {
        return SpecificData.IsFish();
    }

    UFUNCTION(BlueprintPure, Category = "Item|Fish")
    bool GetFishInfo(float& OutLength, float& OutWeight, FString& OutFishName) const
    {
        const FFishSpecificData* FishData = SpecificData.GetFishData();
        if (FishData && FishData->IsValid())
        {
            OutLength = FishData->ActualLength;
            OutWeight = FishData->ActualWeight;
            OutFishName = FishData->FishDataName;
            return true;
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Item|Fish")
    void SetFishData(const FFishSpecificData& InFishData)
    {
        SpecificData.SetFishData(InFishData);
    }

    UFUNCTION(BlueprintPure, Category = "Item|Fish")
    FFishSpecificData GetFishData() const
    {
        const FFishSpecificData* Data = SpecificData.GetFishData();
        return Data ? *Data : FFishSpecificData();
    }

    
    
    
    
    UFUNCTION(BlueprintPure, Category = "Item|Equipment")
    bool IsEquipment() const
    {
        return SpecificData.IsEquipment();
    }

    UFUNCTION(BlueprintCallable, Category = "Item|Equipment")
    void SetEquipmentData(const FEquipmentSpecificData& InEquipmentData)
    {
        SpecificData.SetEquipmentData(InEquipmentData);
    }

    
    
    
    
    UFUNCTION(BlueprintPure, Category = "Item|Consumable")
    bool IsConsumable() const
    {
        return SpecificData.IsConsumable();
    }

    UFUNCTION(BlueprintCallable, Category = "Item|Consumable")
    void SetConsumableData(const FConsumableSpecificData& InConsumableData)
    {
        SpecificData.SetConsumableData(InConsumableData);
    }

    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Item|Consumable")
    AItemActor* SpawnItemActor(UWorld* World, const FTransform& Transform, 
                               const TSubclassOf<AItemActor> ActorClass,
                               AActor* Owner);

    
    
    
    
    UFUNCTION(BlueprintPure, Category = "Item|Type")
    UFishData* GetFishDataAsset() const
    {
        if (!ItemDataProvider)
        {
            return nullptr;
        }
        return Cast<UFishData>(ItemDataProvider.GetObject());
    }

    
    
    
    
    UFUNCTION(BlueprintPure, Category = "Item|Debug")
    FString GetDebugString() const
    {
        FString Result = FString::Printf(TEXT("Item: %s (GUID: %s)"), 
            *DisplayName(), 
            *ItemGuid.ToString());
        
        if (SpecificData.HasSpecificData())
        {
            Result += FString::Printf(TEXT("\n%s"), *SpecificData.ToString());
        }
        
        return Result;
    }

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};