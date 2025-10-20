
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InteractableCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

UCLASS()
class FISHING_API AInteractableCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AInteractableCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USceneCaptureComponent2D* InventoryPortraitCamera;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Gold, Category="Player")
	int32 Gold = 10000;

	UPROPERTY(BlueprintAssignable, Category="Player")
	FOnGoldChanged OnGoldChanged;

	UFUNCTION(BlueprintCallable, Category="Player")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Player")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintCallable, Category="Player")
	void SetGold(int32 NewGold);

	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetPortraitRenderTarget(UTextureRenderTarget2D* RenderTarget);

	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetPortraitCaptureEnabled(bool bEnabled);

	UFUNCTION(Server, Reliable)
	void Server_AddGold(int32 Amount);

	UFUNCTION(Server, Reliable)
	void Server_SetGold(int32 NewGold);
	
protected:
	virtual void PostInitializeComponents() override;

	UFUNCTION()
	void OnRep_Gold();

	void ApplyGoldDelta(int32 Delta);

	

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	
	void SetupPortraitCamera();
};