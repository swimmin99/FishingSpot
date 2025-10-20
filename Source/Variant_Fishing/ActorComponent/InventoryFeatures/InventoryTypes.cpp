#include "InventoryTypes.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"

FItemSyncData::FItemSyncData(const UItemBase* Item, int32 InIndex)
	: ItemGuid()
	, DataAssetPath()  
	, bIsRotated(false)
	, TopLeftIndex(InIndex)
	, SpecificData()
{
	if (!Item)
	{
		return;
	}

	ItemGuid = Item->ItemGuid;
	bIsRotated = Item->bIsRotated;
	TopLeftIndex = InIndex;
	SpecificData = Item->SpecificData;
    
	
	if (Item->ItemDataProvider)
	{
		UObject* Provider = Item->ItemDataProvider.GetObject();
		if (Provider)
		{
			
			DataAssetPath = FSoftObjectPath(Provider);
            
			UE_LOG(LogTemp, Log, TEXT("FItemSyncData: Stored asset path: %s"), 
				   *DataAssetPath.ToString());
		}
	}
}