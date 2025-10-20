#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingInventoryModule.generated.h"

class AItemActor;
class AFish;
class USkeletalMeshComponent;
class UFishingComponent;
class AFishingCharacter;


UCLASS()
class FISHING_API UFishingInventoryModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner, 
					AFishingCharacter* InCharacter,
					USkeletalMeshComponent* InCharacterMesh);
	
	
	void CreateFishItemFromCatch(AFish* CaughtFish, int32 PlayerID);
	void DestroyCurrentFishItem();
	
	
	void AttachAndRevealShowOffItem(FName ShowOffSocket);
	void HideShowOffItem();
	
	
	void AddFishToInventory();
	void DropFishOnGround();
	
	
	void SetItemActorClass(TSubclassOf<AItemActor> InClass) { ItemActorSubClass = InClass; }
	void SetShowOffSocket(FName InSocket) { ShowOffSocket = InSocket; }
	
	
	AItemActor* GetCurrentDisplayFishItem() const { return CurrentDisplayFishItem; }
	void SetCurrentDisplayFishItem(AItemActor* Item) { CurrentDisplayFishItem = Item; }

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	AFishingCharacter* OwnerCharacter = nullptr;
	
	UPROPERTY()
	USkeletalMeshComponent* CharacterMesh = nullptr;
	
	UPROPERTY()
	AItemActor* CurrentDisplayFishItem = nullptr;
	
	UPROPERTY()
	TSubclassOf<AItemActor> ItemActorSubClass;
	
	FName ShowOffSocket = TEXT("HandGrip_R");
};