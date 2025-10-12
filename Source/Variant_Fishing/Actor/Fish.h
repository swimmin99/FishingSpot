#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Fish.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFish, Log, All);

class UFishData;
class UStaticMeshComponent;
class AFishSpawnPool;

UENUM(BlueprintType)
enum class EFishMovementState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Rotating UMETA(DisplayName="Rotating"),
	Moving UMETA(DisplayName="Moving"),
};

UENUM(BlueprintType)
enum class EFishBehaviorState : uint8
{
	Wandering UMETA(DisplayName="Wandering"),
	MovingToBobber UMETA(DisplayName="MovingToBobber"),
	FakeBiting UMETA(DisplayName="FakeBiting"),
	Biting UMETA(DisplayName="Biting"),
	Vanishing UMETA(DisplayName="Vanishing")
};

enum class EFakeBitePhase : uint8
{
	None,
	Freeze,
	BackOff,
	Return
};

UCLASS()
class FISHING_API AFish : public AActor
{
	GENERATED_BODY()

public:
	AFish();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Initialize(UFishData* InFishData, AFishSpawnPool* InSpawnPool, const FVector& SpawnLocation);
	void Activate(UFishData* InFishData, const FVector& SpawnLocation);
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category="Fish")
	bool IsActive() const { return bIsActive; }

	UFUNCTION(BlueprintCallable, Category="Fish")
	EFishBehaviorState GetBehaviorState() const { return BehaviorState; }

	UFUNCTION(BlueprintCallable, Category="Fish")
	EFishMovementState GetMovementState() const { return MovementState; }

	UFUNCTION(BlueprintCallable, Category="Fish")
	UFishData* GetFishData() const { return FishData; }

	void SetWanderTarget(const FVector& TargetLocation);
	UStaticMeshComponent* DetectBobberInView();
	void StartMovingToBobber(UStaticMeshComponent* InTargetBobber);
	bool IsWithinBiteRange(const FVector& BobberLocation) const;

	void SetStateWandering();
	void SetStateBiting();
	void SetStateFakeBiting();
	void TickVanishing(float DeltaTime);
	void SetStateVanishing();
	void OnBobberLost();

	void OnCaught();
	void OnEscaped();
	void PlayBackOff();

	void DrawDetectionDebug(UStaticMeshComponent* DetectedBobber);
	void DrawMovementDebug();

	FVector GetHeadWorldLocation() const;
	FVector GetTailWorldLocation() const;
	float GetFishLength() const;
	void UpdateTailMarkerPosition();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fish")
	USceneComponent* HeadMarker;  

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fish")
	USceneComponent* TailMarker; 

	UStaticMeshComponent* GetTargetBobber() const { return TargetBobber; }
	bool NeedsNewWanderTarget() const { return bNeedsNewTarget && !IsIdling(); }
	bool IsIdling() const { return MovementState == EFishMovementState::Idle && IdleTimer < IdleDuration; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Behavior")
	float FakeBiteTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Behavior")
	float BackOffDistance = 50.f;

	float FakeBiteTimer = 0.f;
	EFakeBitePhase FakeBitePhase = EFakeBitePhase::None;
	FVector FakeBiteReturnTarget = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fish")
	UStaticMeshComponent* FishMesh;

	int32 MaxFakeBiteCount = 0;
	int32 CurrentFakeBiteCount = 0;

	UPROPERTY()
	AFishSpawnPool* SpawnPool = nullptr;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Fish")
	EFishBehaviorState BehaviorState = EFishBehaviorState::Wandering;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fish")
	EFishMovementState MovementState = EFishMovementState::Idle;

	UPROPERTY(VisibleAnywhere, Category="Fish")
	USceneComponent* Pivot;

	FVector CurrentTarget = FVector::ZeroVector;
	FVector InitialSpawnLocation = FVector::ZeroVector;
	FVector MovementStartLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;

	float IdleTimer = 0.f;
	float IdleDuration = 0.f;

	float VanishingTime = 0.8f;
	float VanishSpeed = 250.f;
	float VanishingTimer = 0.f;

	float MovementTimer = 0.f;
	float TotalMovementTime = 0.f;
	float TotalMovementDistance = 0.f;

	bool bNeedsNewTarget = false;

	UPROPERTY()
	UStaticMeshComponent* TargetBobber = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float RotationSpeed = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float ArrivalThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float MinIdleTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float MaxIdleTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float AccelerationTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float DecelerationTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float MinSpeedMultiplier = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float BobberSpeedMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category="Fish|Movement")
	float SpeedVariation = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Debug")
	bool bShowDebugDetection = false;

	UPROPERTY(EditDefaultsOnly, Category="Debug")
	bool bShowDebugMovement = true;
	bool bRemovedFromPoolOnVanishing;

	void TickMovement(float DeltaTime);
	void TickIdle(float DeltaTime);
	void TickRotating(float DeltaTime);
	void TickMoving(float DeltaTime);
	void TickFakeBite(float DeltaTime);

	void StartRotatingToTarget(const FVector& TargetLocation);
	void StartMovingToTarget();
	void OnReachedTarget();
	void RequestNewWanderTarget();

	void StartFakeBiteFreeze();
	void StartFakeBiteBackOff();

	float GetCurrentMoveSpeed() const;
	float CalculateSpeedMultiplier() const;
	void ResetInternalState();

	UPROPERTY(ReplicatedUsing=OnRep_FishData, VisibleAnywhere, BlueprintReadOnly)
	UFishData* FishData = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_IsActive)
	bool bIsActive = false;

	UFUNCTION()
	void OnRep_IsActive();

	UFUNCTION()
	void OnRep_FishData();
};
