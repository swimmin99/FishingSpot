#pragma once

#include "CoreMinimal.h"
#include "InteractableCharacter.h"
#include "GameFramework/Character.h"
#include "FishingCharacter.generated.h"

class UInventoryComponent;
DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UFishingComponent;
class UStaticMeshComponent;
class AShopCharacter;
class IInteractable;
class UCapsuleComponent;
class UNiagaraComponent;

UCLASS(Blueprintable)
class FISHING_API AFishingCharacter : public AInteractableCharacter
{
	GENERATED_BODY()

public:
	AFishingCharacter();

	virtual void Tick(float DeltaSeconds) override;
	void AssignMeshIndex();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* ShowOffCameraBoom;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* FollowCamera;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* ShowOffCamera;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
	UStaticMeshComponent* FishingRod;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
	UStaticMeshComponent* Bobber;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
	UFishingComponent* CoreFishingComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UInventoryComponent* CoreInventoryComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	UCapsuleComponent* InteractionCapsule;
	uint32 MeshIndex;


	UFUNCTION()
	void OnInteractionBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
	                               class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                               bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


	UFUNCTION()
	void OnFishingStateChanged(EFishingState OldState, EFishingState NewState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* SplashEffectComponent;

	UFUNCTION()
	void SwitchToShowOffCamera(bool val);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

	UPROPERTY()
	bool bTearingDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	TArray<TObjectPtr<USkeletalMesh>> MeshOptions;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* JumpAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* InventoryAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* MoveAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* LookAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* MouseLookAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* PrimaryAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* InteractAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	const UInputAction* EscapeAction = nullptr;


	void ApplyMeshIndex();

	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);
	void DoMove(float Right, float Forward);
	void DoLook(float Yaw, float Pitch);
	void OnToggleInventory();
	void OnPrimary();

public:
	void ExecuteInteract();
	void ExecuteEscape();


	UFUNCTION(Server, Reliable)
	void Server_TryPickup(AActor* Target);


	UPROPERTY()
	AActor* CurrentOverlapActor = nullptr;
};
