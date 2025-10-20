#pragma once

#include "CoreMinimal.h"
#include "InteractableCharacter.h"
#include "GameFramework/Character.h"
#include "Variant_Fishing/Interface/Interactable.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "ShopCharacter.generated.h"

class AFishingPlayerController;
class UInventoryComponent;
class AFishingCharacter;

UCLASS()
class FISHING_API AShopCharacter : public AInteractableCharacter, public IInteractable
{
	GENERATED_BODY()

public:
	AShopCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual FString GetInteractableName_Implementation() const override;
	virtual EInteractionType GetType_Implementation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	UInventoryComponent* ShopInventoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FString ShopName = TEXT("General Store");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 ShopColumns = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 ShopRows = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	float InteractionRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TArray<TObjectPtr<UPrimaryDataAsset>> InitialShopItems;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void InitializeShop();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	UInventoryComponent* GetShopInventory() const { return ShopInventoryComponent; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool IsInInteractionRange(AActor* OtherActor) const;

	void PlayIdleLoop();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimationAsset> IdleLoopAnimation = nullptr;

private:
	void SpawnInitialItems();
};
