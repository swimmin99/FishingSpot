#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishSpawnPool.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFishSpawnPool, Log, All);

class UBoxComponent;
class UFishData;
class AFish;
class UStaticMeshComponent;

USTRUCT()
struct FFishBobberAssignment
{
	GENERATED_BODY()

	UPROPERTY()
	AFish* Fish = nullptr;

	UPROPERTY()
	UStaticMeshComponent* Bobber = nullptr;

	float AssignedTime = 0.f;
};

UCLASS()
class FISHING_API AFishSpawnPool : public AActor
{
	GENERATED_BODY()

public:

	AFishSpawnPool();
	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="FishPool")
	int32 GetSpawnedFishCount() const { return SpawnedFish.Num(); }

	UFUNCTION(BlueprintCallable, Category="FishPool")
	int32 GetPooledFishCount() const { return InactiveFishPool.Num(); }

	UFUNCTION(BlueprintCallable, Category="FishPool")
	int32 GetTotalFishCount() const { return SpawnedFish.Num() + InactiveFishPool.Num(); }

	UFUNCTION(BlueprintCallable, Category="FishPool")
	UBoxComponent* GetSpawnBox() const { return SpawnBox; }

	UFUNCTION(BlueprintCallable, Category="FishPool")
	bool IsLocationInBounds(const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category="FishPool")
	FVector ClampLocationToBounds(const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category="FishPool")
	void RemoveFish(AFish* Fish, bool bCaught);

	void OnFishStartVanishing(AFish* Fish);
	void OnFishVanished(AFish* Fish);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FishPool")
	UBoxComponent* SpawnBox;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Spawn")
	TArray<TObjectPtr<UFishData>> FishDataList;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Spawn")
	float SpawnCooldown = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Spawn")
	float SpawnChance = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Spawn")
	int32 MaxFish = 5;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Spawn")
	float MaxEmptyTime = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Movement")
	float BorderOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Movement")
	float MinDistanceBetweenFish = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Movement")
	float ForwardSpacePreference = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Pooling")
	int32 PrewarmPoolSize = 10;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Pooling")
	bool bEnablePooling = true;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Management")
	float ManagementTickInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Debug")
	bool bShowDebugBox = true;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Debug")
	bool bShowDebugInfo = true;

	UPROPERTY(EditDefaultsOnly, Category="FishPool|Debug")
	bool bShowDebugWanderArea = true;

	UPROPERTY()
	TArray<AFish*> SpawnedFish;

	UPROPERTY()
	TArray<AFish*> InactiveFishPool;

	TArray<FFishBobberAssignment> BobberAssignments;

	FTimerHandle SpawnTimerHandle;
	float EmptyTime;
	float ManagementTickTimer;

	void TrySpawnFish();
	AFish* SpawnFish(UFishData* FishData);
	AFish* CreateNewFish(UFishData* FishData, const FVector& SpawnLocation);
	FVector GetRandomSpawnLocation();
	UFishData* GetRandomFishData();

	void PrewarmPool();
	AFish* GetFromPool();
	void ReturnToPool(AFish* Fish);

	void ManageFish(float DeltaTime);
	void UpdateWanderingFish(AFish* Fish);
	void UpdateMovingToBobberFish(AFish* Fish);
	void UpdateBitingFish(AFish* Fish);

	FVector GenerateWanderTarget(AFish* ForFish);
	bool IsGoodWanderTarget(const FVector& Target, AFish* ForFish) const;
	FVector GetSafeAreaCenter() const;
	FVector GetSafeAreaExtent() const;

	void TryAssignBobberToFish();
	bool IsBobberAlreadyAssigned(UStaticMeshComponent* Bobber) const;
	void AssignBobberToFish(AFish* Fish, UStaticMeshComponent* Bobber);
	void UnassignFish(AFish* Fish);
	FFishBobberAssignment* FindAssignmentByFish(AFish* Fish);
	FFishBobberAssignment* FindAssignmentByBobber(UStaticMeshComponent* Bobber);

	TArray<UStaticMeshComponent*> FindAllVisibleBobbers();
};
