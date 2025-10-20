#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingStateModule.h"
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


UCLASS()
class FISHING_API UFishingBobberModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner, UStaticMeshComponent* InBobber, UNiagaraComponent* InSplashEffect);
	
	
	void Show(const FVector& TargetLocation);
	void Hide();
	bool IsActive() const { return bBobberActive; }
	
	
	void PlayFakeBite();
	void PlayRealBite();
	void StartOrbitMovement(const FVector& Center);
	
	
	void UpdateMovement(float DeltaSeconds, EFishingState CurrentState);
	void UpdateOrbitMovement(float DeltaSeconds, float OrbitAngle);
	
	
	FVector GetTargetLocation() const { return BobberTargetLocation; }
	EBobberState GetState() const { return BobberState; }
	bool IsAnimating() const { return bBobberAnimating; }
	bool IsWaitingForRealBiteComplete() const { return bWaitingForRealBiteAnimComplete; }
	
	
	void SetTargetLocation(const FVector& Location) { BobberTargetLocation = Location; }
	void SetWaitingForRealBiteComplete(bool bWaiting) { bWaitingForRealBiteAnimComplete = bWaiting; }
	void SetState(EBobberState NewState) { BobberState = NewState; }
	void ResetAnimation() { BobberAnimTimer = 0.f; bBobberAnimating = false; }

	
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
	
	
	bool bBobberActive = false;
	FVector BobberTargetLocation = FVector::ZeroVector;
	EBobberState BobberState = EBobberState::Hidden;
	float BobberAnimTimer = 0.f;
	bool bBobberAnimating = false;
	bool bWaitingForRealBiteAnimComplete = false;
	
	
	float FakeBiteDipAmount = 15.f;
	float FakeBiteAnimSpeed = 8.f;
	float RealBiteAnimSpeed = 8.f;
	float RealBiteSinkAmount = 15.f;
};