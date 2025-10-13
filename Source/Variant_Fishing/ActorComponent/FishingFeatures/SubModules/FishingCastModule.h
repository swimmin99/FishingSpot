#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingCastModule.generated.h"

class UFishingComponent;
class AFishingCharacter;

/**
 * Handles casting validation and collision checking
 */
UCLASS()
class FISHING_API UFishingCastModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner, AFishingCharacter* InCharacter);
	
	// Casting validation
	bool CheckCastingCollision(FVector& OutTargetLocation);
	
	// Configuration
	void SetForwardOffset(float Offset) { CastCheckForwardOffset = Offset; }
	void SetDownOffset(float Offset) { CastCheckDownOffset = Offset; }
	void SetMaxHeight(float Height) { CastMaxHeight = Height; }
	void SetFishingWaterTag(FName Tag) { FishingWaterTag = Tag; }
	void SetShowDebug(bool bShow) { bShowDebugCasting = bShow; }

protected:
	void DrawCastingDebug(const FVector& StartLoc, const FVector& EndLoc, 
						  const FVector& HitLoc, bool bValid, const FString& Message);

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	AFishingCharacter* OwnerCharacter = nullptr;
	
	// Configuration
	float CastCheckForwardOffset = 150.f;
	float CastCheckDownOffset = 130.f;
	float CastMaxHeight = 200.f;
	FName FishingWaterTag = TEXT("FishingWater");
	bool bShowDebugCasting = true;
};