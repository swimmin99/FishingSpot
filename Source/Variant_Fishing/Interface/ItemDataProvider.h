
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemDataProvider.generated.h"

class UItemBase;

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
  Bronze UMETA(DisplayName = "Bronze"),
  Silver UMETA(DisplayName = "Silver"),
  Gold UMETA(DisplayName = "Gold")
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
 All         UMETA(DisplayName = "All"),
 Fish        UMETA(DisplayName = "Fish"),
 Equipment   UMETA(DisplayName = "Equipment"),
 Consumable  UMETA(DisplayName = "Consumable"),
 Material    UMETA(DisplayName = "Material"),
 Quest       UMETA(DisplayName = "Quest"),
 Misc        UMETA(DisplayName = "Misc")
};



UINTERFACE(MinimalAPI, BlueprintType)
class UItemDataProvider : public UInterface
{
    GENERATED_BODY()
};

class FISHING_API IItemDataProvider
{
    GENERATED_BODY()

public:
    
    
    
    
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Display")
    FText GetDisplayName() const;
    virtual FText GetDisplayName_Implementation() const = 0;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Display")
    UItemBase* CreateBaseItem(UObject* Outer, const FString& LocationName = TEXT("Unknown"));
    virtual UItemBase* CreateBaseItem_Implementation(UObject* Outer, const FString& LocationName = TEXT("Unknown")) = 0;

 
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Display")
    FText GetDescription() const;
    virtual FText GetDescription_Implementation() const = 0;

    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Display")
    UMaterialInterface* GetIcon() const;
    virtual UMaterialInterface* GetIcon_Implementation() const = 0;

    
    
    
    
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Economy")
    int32 GetPrice() const;
    virtual int32 GetPrice_Implementation() const = 0;

    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Inventory")
    FIntPoint GetBaseDimensions() const;
    virtual FIntPoint GetBaseDimensions_Implementation() const = 0;

    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Inventory")
    EItemCategory GetCategory() const;
    virtual EItemCategory GetCategory_Implementation() const = 0;

    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Inventory")
    EItemRarity GetRarity() const;
    virtual EItemRarity GetRarity_Implementation() const { return EItemRarity::Bronze; }

    
    
    
    
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|World")
    UStaticMesh* GetWorldMesh() const;
    virtual UStaticMesh* GetWorldMesh_Implementation() const = 0;

    
    
    
    
    
    virtual FString GetDataAssetName() const
    {
        if (const UObject* Obj = Cast<UObject>(this))
        {
            return Obj->GetName();
        }
        return TEXT("Unknown");
    }

    
    virtual FPrimaryAssetId GetItemAssetId() const
    {
        if (const UPrimaryDataAsset* Asset = Cast<UPrimaryDataAsset>(this))
        {
            return Asset->GetPrimaryAssetId();
        }
        return FPrimaryAssetId();
    }
};