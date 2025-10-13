#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Variant_Fishing/ActorComponent/SubModules/FishingStateModule.h"
#include "FishingBobberModule.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UFishingComponent;

UENUM(BlueprintType)
enum class EBobberState : uint8
{
	Hidden UMETA(DisplayName="Hidden"),
	Idle UMETA(DisplayName="Idle"),
	FakeBite UMETA(DisplayName="FakeBite"),
	RealBite UMETA(DisplayName="RealBite")
};

/**
 * Handles all bobber-related logic: visibility, animations, movement
 */
UCLASS()
class FISHING_API UFishingBobberModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner, UStaticMeshComponent* InBobber, UNiagaraComponent* InSplashEffect);
	
	// Bobber visibility
	void Show(const FVector& TargetLocation);
	void Hide();
	bool IsActive() const { return bBobberActive; }
	
	// Bobber animations
	void PlayFakeBite();
	void PlayRealBite();
	void StartOrbitMovement(const FVector& Center);
	
	// Update
	void UpdateMovement(float DeltaSeconds, EFishingState CurrentState);
	void UpdateOrbitMovement(float DeltaSeconds, float OrbitAngle);
	
	// Getters
	FVector GetTargetLocation() const { return BobberTargetLocation; }
	EBobberState GetState() const { return BobberState; }
	bool IsAnimating() const { return bBobberAnimating; }
	bool IsWaitingForRealBiteComplete() const { return bWaitingForRealBiteAnimComplete; }
	
	// Setters
	void SetTargetLocation(const FVector& Location) { BobberTargetLocation = Location; }
	void SetWaitingForRealBiteComplete(bool bWaiting) { bWaitingForRealBiteAnimComplete = bWaiting; }
	void SetState(EBobberState NewState) { BobberState = NewState; }
	void ResetAnimation() { BobberAnimTimer = 0.f; bBobberAnimating = false; }

	// Configuration setters
	void SetFakeBiteDipAmount(float Amount) { FakeBiteDipAmount = Amount; }
	void SetFakeBiteAnimSpeed(float Speed) { FakeBiteAnimSpeed = Speed; }
	void SetRealBiteAnimSpeed(float Speed) { RealBiteAnimSpeed = Speed; }
	void SetRealBiteSinkAmount(float Amount) { RealBiteSinkAmount = Amount; }

protected:
	void PlaySplashEffect(float Scale);
	void ConfigureBobberCollision();

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	UStaticMeshComponent* Bobber = nullptr;
	
	UPROPERTY()
	UNiagaraComponent* SplashEffect = nullptr;
	
	// State
	bool bBobberActive = false;
	FVector BobberTargetLocation = FVector::ZeroVector;
	EBobberState BobberState = EBobberState::Hidden;
	float BobberAnimTimer = 0.f;
	bool bBobberAnimating = false;
	bool bWaitingForRealBiteAnimComplete = false;
	
	// Configuration (can be set from FishingComponent)
	float FakeBiteDipAmount = 15.f;
	float FakeBiteAnimSpeed = 8.f;
	float RealBiteAnimSpeed = 8.f;
	float RealBiteSinkAmount = 15.f;
};