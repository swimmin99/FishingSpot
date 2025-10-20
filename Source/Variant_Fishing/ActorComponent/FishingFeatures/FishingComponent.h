#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SubModules/FishingStateModule.h"
#include "FishingComponent.generated.h"


class AItemActor;
class AFishingCharacter;
class AFish;
class UItemBase;
class UAnimMontage;
class UNiagaraComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;


class UFishingBobberModule;
class UFishingAnimationModule;
class UFishingBiteModule;
class UFishingCastModule;
class UFishingInventoryModule;
class UFishingStateModule;

class UDatabaseManager;

DECLARE_LOG_CATEGORY_EXTERN(LogFishingComponent, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFishingStateChanged, EFishingState, OldState, EFishingState, NewState);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FISHING_API UFishingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFishingComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	void Initialize(USkeletalMeshComponent* InCharacterMesh,
	                UStaticMeshComponent* InFishingRod,
	                UStaticMeshComponent* InBobber,
	                UNiagaraComponent* InSplashEffect,
	                AFishingCharacter* InOwnerCharacter);

	
	
	UFUNCTION(BlueprintCallable, Category="Fishing")
	void OnPrimaryInput();

	UFUNCTION(BlueprintCallable, Category="Fishing")
	bool IsFishing() const;

	UFUNCTION(BlueprintCallable, Category="Fishing")
	bool IsInBiteWindow() const;

	UFUNCTION(BlueprintCallable, Category="Fishing")
	EFishingState GetFishState() const;

	UFUNCTION(BlueprintCallable, Category="Fishing")
	AFish* GetCurrentBitingFish() const;

	UFUNCTION(BlueprintCallable, Category="Fishing")
	UStaticMeshComponent* GetFishingRod() const { return FishingRod; }

	UFUNCTION(BlueprintCallable, Category="Fishing")
	UStaticMeshComponent* GetBobber() const { return Bobber; }

	
	UFUNCTION(BlueprintCallable, Category="Fishing|AnimNotify")
	void OnAnimNotify_CastEnd();

	UFUNCTION(BlueprintCallable, Category="Fishing|AnimNotify")
	void OnAnimNotify_PullOutEnd();

	UFUNCTION(BlueprintCallable, Category="Fishing|AnimNotify")
	void OnAnimNotify_TakingOffEnd();

	UFUNCTION(BlueprintCallable, Category="Fishing|AnimNotify")
	void OnAnimNotify_ShowOffEnd();

	
	UFUNCTION(BlueprintCallable, Category="Fishing")
	void OnFishBite(AFish* Fish);

	UFUNCTION(BlueprintCallable, Category="Fishing")
	void OnFishFakeBite(AFish* Fish, float FakeBiteTime);

	
	UPROPERTY(BlueprintAssignable, Category="Fishing|Events")
	FOnFishingStateChanged OnStateChanged;

	
	UFUNCTION()
	void OnRep_CurrentSocket();

	UFUNCTION()
	void OnRep_CurrentDisplayFishItem();

	UFUNCTION()
	void OnRep_FishState();

	UFUNCTION()
	void OnRep_BobberActive();

	UFUNCTION()
	void OnRep_BobberTarget();

	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowFishItem();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayFishingAnimation(FName SectionName, bool bPositionRotHand);

protected:
	
	UFUNCTION(Server, Reliable)
	void Server_RequestPrimary();

	UFUNCTION(Server, Reliable)
	void Server_SetState(EFishingState NewState);

	
	void ProcessFishingSuccess();

	
	
	
	UPROPERTY()
	UDatabaseManager* DatabaseManager = nullptr;
	
	UPROPERTY()
	UFishingBobberModule* BobberModule;
	
	UPROPERTY()
	UFishingAnimationModule* AnimationModule;
	
	UPROPERTY()
	UFishingBiteModule* BiteModule;
	
	UPROPERTY()
	UFishingCastModule* CastModule;
	
	UPROPERTY()
	UFishingInventoryModule* InventoryModule;
	
	UPROPERTY()
	UFishingStateModule* StateModule;

	
	
	UPROPERTY()
	USkeletalMeshComponent* CharacterMesh = nullptr;

	UPROPERTY()
	UStaticMeshComponent* FishingRod = nullptr;

	UPROPERTY()
	UStaticMeshComponent* Bobber = nullptr;

	UPROPERTY()
	UNiagaraComponent* SplashEffect = nullptr;

	UPROPERTY()
	AFishingCharacter* OwnerCharacter = nullptr;

	
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentSocket)
	FName CurrentFishingRodSocket = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_BobberActive)
	bool bBobberActive = false;

	UPROPERTY(ReplicatedUsing=OnRep_BobberTarget)
	FVector BobberTargetLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing=OnRep_FishState)
	EFishingState FishState = EFishingState::None;

	UPROPERTY(Replicated)
	bool bIsFishing = false;

	UPROPERTY(Replicated)
	bool bInBiteWindow = false;

	UPROPERTY(Replicated)
	AItemActor* CurrentDisplayFishItem = nullptr;

	
	
	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	UAnimMontage* FishingMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_Cast = TEXT("Sec_Cast");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_Idle = TEXT("Sec_Idle");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_Bite = TEXT("Sec_Bite");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_PullOut = TEXT("Sec_PullOut");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_ShowOff = TEXT("Sec_ShowOff");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_TakingOff = TEXT("Sec_TakingOff");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Animation")
	FName Sec_FishFight = TEXT("Sec_FishFight");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fishing|Sockets")
	FName IdleFishingSocket = TEXT("IdleFishingRotSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fishing|Sockets")
	FName ActiveFishingSocket = TEXT("ik_hand_gun");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fishing|Sockets")
	FName ShowOffSocket = TEXT("HandGrip_R");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Bobber")
	float FakeBiteDipAmount = 15.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Bobber")
	float FakeBiteAnimSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Bobber")
	float RealBiteAnimSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Bobber")
	float RealBiteSinkAmount = 15.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|CastCheck")
	float CastCheckForwardOffset = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|CastCheck")
	float CastCheckDownOffset = 130.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|CastCheck")
	float CastMaxHeight = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|CastCheck")
	FName FishingWaterTag = TEXT("FishingWater");
	
	UPROPERTY(EditDefaultsOnly, Category="Fishing|Debug")
	bool bShowDebugCasting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Inventory")
	TSubclassOf<AItemActor> ItemActorSubClass;

	
	friend class UFishingBobberModule;
	friend class UFishingAnimationModule;
	friend class UFishingBiteModule;
	friend class UFishingCastModule;
	friend class UFishingInventoryModule;
	friend class UFishingStateModule;
};