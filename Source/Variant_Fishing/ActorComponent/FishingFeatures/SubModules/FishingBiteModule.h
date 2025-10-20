#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingBiteModule.generated.h"

class AFish;
class UFishingComponent;


UCLASS()
class FISHING_API UFishingBiteModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner);
	
	
	void OnFishBite(AFish* Fish);
	void OnFishFakeBite(AFish* Fish, float FakeBiteTime);
	void HandleBiteWindow();
	
	
	void InitBiteFight();
	void UpdateBiteFight(float DeltaSeconds);
	void FinishBiteFight();
	
	
	void StartBiteWindowTimer();
	void OnBiteTimeout();
	void ClearTimers();
	
	
	bool IsInBiteWindow() const { return bInBiteWindow; }
	bool IsBiteFighting() const { return bBiteFighting; }
	AFish* GetCurrentBitingFish() const { return CurrentBitingFish; }
	
	
	void SetInBiteWindow(bool bInWindow) { bInBiteWindow = bInWindow; }
	void SetCurrentBitingFish(AFish* Fish) { CurrentBitingFish = Fish; }
	void ClearCurrentBitingFish() { CurrentBitingFish = nullptr; }

	
	float GetOrbitAngle() const { return BobberOrbitAngle; }
	void SetOrbitAngle(float Angle) { BobberOrbitAngle = Angle; }
	void SetOrbitCenter(const FVector& Center) { BobberOrbitCenter = Center; }

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	AFish* CurrentBitingFish = nullptr;
	
	
	bool bInBiteWindow = false;
	bool bBiteFighting = false;
	float BiteFightTimer = 0.f;
	
	
	FTimerHandle Th_BiteDur;
	FTimerHandle Th_BiteFightDur;
	
	
	FVector BobberOrbitCenter = FVector::ZeroVector;
	float BobberOrbitAngle = 0.f;
	
	
	static constexpr float BobberOrbitRadius = 64.0f;
	static constexpr float BobberOrbitAngularSpeed = 4.8f;
	
	friend class UFishingComponent;
};