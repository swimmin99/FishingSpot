#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingStateModule.generated.h"

class UFishingComponent;
class AFishingCharacter;

UENUM(BlueprintType)
enum class EFishingState : uint8
{
	None UMETA(DisplayName="None"),
	Casting UMETA(DisplayName="Casting"),
	Idle UMETA(DisplayName="Idle"),
	BiteWindow UMETA(DisplayName="BiteWindow"),
	BiteFight UMETA(DisplayName="BiteFight"),
	PullOut UMETA(DisplayName="PullOut"),
	ShowOff UMETA(DisplayName="ShowOff"),
	Losing UMETA(DisplayName="Losing"),
	Exit UMETA(DisplayName="Exit"),
};

/**
 * Handles fishing state transitions and state-related logic
 */
UCLASS()
class FISHING_API UFishingStateModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner, AFishingCharacter* InCharacter);
	
	// State management
	void SetState(EFishingState NewState);
	EFishingState GetState() const { return FishState; }
	static const TCHAR* StateToString(EFishingState State);
	
	// Fishing session
	void EnterFishing();
	void ExitFishing();
	bool IsFishing() const { return bIsFishing; }
	void SetIsFishing(bool bFishing) { bIsFishing = bFishing; }
	
	// Movement lock
	void LockMovement(bool bLock);

protected:
	void OnStateEntered(EFishingState NewState);

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	AFishingCharacter* OwnerCharacter = nullptr;
	
	EFishingState FishState = EFishingState::None;
	bool bIsFishing = false;
};