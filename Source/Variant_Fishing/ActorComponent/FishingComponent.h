#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FishingComponent.generated.h"

class AItemActor;
class AFishingCharacter;
DECLARE_LOG_CATEGORY_EXTERN(LogFishingComponent, Log, All);

class UStaticMeshComponent;
class USkeletalMeshComponent;
class ACharacter;
class AFish;
class UItemBase;
class UAnimMontage;
class UNiagaraComponent;
class UNiagaraSystem;

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

UENUM(BlueprintType)
enum class EBobberState : uint8
{
	Hidden UMETA(DisplayName="Hidden"),
	Idle UMETA(DisplayName="Idle"),
	FakeBite UMETA(DisplayName="FakeBite"),
	RealBite UMETA(DisplayName="RealBite")
};

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

	UPROPERTY(ReplicatedUsing=OnRep_CurrentSocket)
	FName CurrentFishingRodSocket = NAME_None;

	static const TCHAR* FishingStateToStr(EFishingState S);
	void Initialize(USkeletalMeshComponent* InCharacterMesh,
	                UStaticMeshComponent* InFishingRod,
	                UStaticMeshComponent* InBobber,
	                UNiagaraComponent* InSplashEffect,
	                AFishingCharacter* InOwnerCharacter);

	UFUNCTION(BlueprintCallable, Category="Fishing")
	void OnPrimaryInput();

	UFUNCTION()
	void OnRep_CurrentSocket();

	UFUNCTION()
	void OnRep_CurrentDisplayFishItem();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowFishItem();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayFishingAnimation(FName SectionName, bool bPositionRotHand);

	UFUNCTION(BlueprintCallable, Category="Fishing")
	bool IsFishing() const { return bIsFishing; }

	UFUNCTION(BlueprintCallable, Category="Fishing")
	bool IsInBiteWindow() const { return bInBiteWindow; }

	UFUNCTION(BlueprintCallable, Category="Fishing")
	EFishingState GetFishState() const { return FishState; }

	UFUNCTION(BlueprintCallable, Category="Fishing")
	AFish* GetCurrentBitingFish() const { return CurrentBitingFish; }

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

	UPROPERTY(BlueprintAssignable, Category="Fishing|Events")
	FOnFishingStateChanged OnStateChanged;

	UFUNCTION(BlueprintCallable, Category="Fishing")
	void OnFishBite(class AFish* Fish);

	UFUNCTION(BlueprintCallable, Category="Fishing")
	void OnFishFakeBite(AFish* Fish, float FakeBiteTime);
	void AttachAndRevealShowOffItem();

protected:
	void PlayFishing(FName SectionName, bool bPositionRotHand);
	void PlayFishing(FName SectionName);
	void PlayFishingMontageSection(FName SectionName);
	void PlaySplashEffect(float Scale);

	void EnterFishing_Server();
	void ExitFishing_Server();
	void LockMovement(bool bLock);
	void OnBiteTimeout_Server();

	bool CheckCastingCollision(FVector& OutTargetLocation);
	void ShowBobber();
	void HideBobber();
	void StartBiteWindowTimer();
	void UpdateBobberMovement(float DeltaSeconds);
	void UpdateFishingRodSocket(FName DesiredSocket);
	void DrawCastingDebug(const FVector& StartLoc, const FVector& EndLoc, const FVector& HitLoc,
										 bool bValid, const FString& Message);

	void HandleBiteWindow();
	void InitBiteFight();
	void HandleBiteFight(float DeltaSeconds);
	void FinishBiteFight();
	void ProcessFishingSuccess();

	void PlayBobberFakeBite();
	void PlayBobberRealBite();

	UFUNCTION(Server, Reliable)
	void Server_RequestPrimary();

	UFUNCTION(Server, Reliable)
	void Server_SetState(EFishingState NewState);

	UFUNCTION()
	void OnRep_FishState();

	UFUNCTION()
	void OnRep_BobberActive();

	UFUNCTION()
	void OnRep_BobberTarget();

protected:
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

	UPROPERTY()
	USkeletalMeshComponent* CharacterMesh = nullptr;

	UPROPERTY()
	UStaticMeshComponent* FishingRod = nullptr;

	UPROPERTY()
	UStaticMeshComponent* Bobber = nullptr;

	UPROPERTY()
	UNiagaraComponent* SplashEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fishing|Sockets")
	FName IdleFishingSocket = TEXT("IdleFishingRotSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fishing|Sockets")
	FName ActiveFishingSocket = TEXT("ik_hand_gun");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fishing|Sockets")
	FName ShowOffSocket = TEXT("HandGrip_R");

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Bobber")
	float BobberForwardOffset = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Fishing|Bobber")
	float BobberFloatOffset = 50.f;

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

	UPROPERTY()
	AFish* CurrentBitingFish = nullptr;

	UPROPERTY(Replicated)
	AItemActor* CurrentDisplayFishItem = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish|Inventory")
	TSubclassOf<AItemActor> ItemActorSubClass;

	UPROPERTY()
	AFishingCharacter* OwnerCharacter = nullptr;

protected:
	EBobberState BobberState = EBobberState::Hidden;
	float BobberAnimTimer = 0.f;
	bool bBobberAnimating = false;
	bool bWaitingForRealBiteAnimComplete = false;

	FTimerHandle Th_BiteDur;
	FTimerHandle Th_BiteFightDur;
	float BiteFightTimer = 0.f;
	bool bBiteFighting = false;

	static constexpr float BobberOrbitRadius =64.0f;
	static constexpr float BobberOrbitAngularSpeed = 4.8f;

	FVector BobberOrbitCenter = FVector::ZeroVector;
	float BobberOrbitAngle = 0.f;
};
