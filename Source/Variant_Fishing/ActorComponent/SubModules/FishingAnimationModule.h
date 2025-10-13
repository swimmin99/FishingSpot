#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FishingAnimationModule.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UFishingComponent;

/**
 * Handles all fishing animation playback and fishing rod socket management
 */
UCLASS()
class FISHING_API UFishingAnimationModule : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFishingComponent* InOwner, 
					USkeletalMeshComponent* InCharacterMesh,
					UStaticMeshComponent* InFishingRod);
	
	// Animation playback
	void PlayMontageSection(FName SectionName);
	void PlayWithHandPosition(FName SectionName, bool bPositionRotHand);
	void StopMontage(float BlendOutTime = 0.2f);
	
	// Socket management
	void UpdateFishingRodSocket(FName DesiredSocket);
	void AttachToIdleSocket();
	void AttachToActiveSocket();
	
	// Configuration
	void SetMontage(UAnimMontage* InMontage) { FishingMontage = InMontage; }
	void SetIdleSocket(FName InSocket) { IdleFishingSocket = InSocket; }
	void SetActiveSocket(FName InSocket) { ActiveFishingSocket = InSocket; }
	
	// Getters
	FName GetCurrentSocket() const { return CurrentFishingRodSocket; }
	UAnimMontage* GetMontage() const { return FishingMontage; }

protected:
	UPROPERTY()
	UFishingComponent* OwnerComponent = nullptr;
	
	UPROPERTY()
	USkeletalMeshComponent* CharacterMesh = nullptr;
	
	UPROPERTY()
	UStaticMeshComponent* FishingRod = nullptr;
	
	UPROPERTY()
	UAnimMontage* FishingMontage = nullptr;
	
	FName CurrentFishingRodSocket = NAME_None;
	FName IdleFishingSocket = TEXT("IdleFishingRotSocket");
	FName ActiveFishingSocket = TEXT("ik_hand_gun");
};