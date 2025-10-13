#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingBiteModule.generated.h"

class AFish;
class UFishingComponent;

/**
 * Handles fish bite mechanics: fake bites, real bites, bite windows, and bite fights
 */
UCLASS()
class FISHING_API UFishingBiteModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner);
	
	// Bite handling
	void OnFishBite(AFish* Fish);
	void OnFishFakeBite(AFish* Fish, float FakeBiteTime);
	void HandleBiteWindow();
	
	// Bite fight
	void InitBiteFight();
	void UpdateBiteFight(float DeltaSeconds);
	void FinishBiteFight();
	
	// Bite window timer
	void StartBiteWindowTimer();
	void OnBiteTimeout();
	void ClearTimers();
	
	// State queries
	bool IsInBiteWindow() const { return bInBiteWindow; }
	bool IsBiteFighting() const { return bBiteFighting; }
	AFish* GetCurrentBitingFish() const { return CurrentBitingFish; }
	
	// State setters
	void SetInBiteWindow(bool bInWindow) { bInBiteWindow = bInWindow; }
	void SetCurrentBitingFish(AFish* Fish) { CurrentBitingFish = Fish; }
	void ClearCurrentBitingFish() { CurrentBitingFish = nullptr; }

	// Orbit data getters
	float GetOrbitAngle() const { return BobberOrbitAngle; }
	void SetOrbitAngle(float Angle) { BobberOrbitAngle = Angle; }
	void SetOrbitCenter(const FVector& Center) { BobberOrbitCenter = Center; }

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	AFish* CurrentBitingFish = nullptr;
	
	// Bite state
	bool bInBiteWindow = false;
	bool bBiteFighting = false;
	float BiteFightTimer = 0.f;
	
	// Timers
	FTimerHandle Th_BiteDur;
	FTimerHandle Th_BiteFightDur;
	
	// Orbit data for bite fight
	FVector BobberOrbitCenter = FVector::ZeroVector;
	float BobberOrbitAngle = 0.f;
	
	// Configuration constants
	static constexpr float BobberOrbitRadius = 64.0f;
	static constexpr float BobberOrbitAngularSpeed = 4.8f;
	
	friend class UFishingComponent;
};