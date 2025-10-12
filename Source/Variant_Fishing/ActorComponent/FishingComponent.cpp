#include "FishingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "FishingCharacter.h"
#include "InventoryComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraComponent.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Variant_Fishing/Actor/Fish.h"
#include "Variant_Fishing/Actor/FishSpawnPool.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY(LogFishingComponent);

UFishingComponent::UFishingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFishingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFishingComponent, bBobberActive);
	DOREPLIFETIME(UFishingComponent, BobberTargetLocation);
	DOREPLIFETIME(UFishingComponent, FishState);
	DOREPLIFETIME(UFishingComponent, bIsFishing);
	DOREPLIFETIME(UFishingComponent, bInBiteWindow);
	DOREPLIFETIME(UFishingComponent, CurrentDisplayFishItem);
	DOREPLIFETIME(UFishingComponent, CurrentFishingRodSocket);
}

void UFishingComponent::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogFishingComponent, Log, TEXT("FishingComponent BeginPlay - Owner: %s"), *GetNameSafe(GetOwner()));
}

void UFishingComponent::Initialize(USkeletalMeshComponent* InCharacterMesh,
                                   UStaticMeshComponent* InFishingRod,
                                   UStaticMeshComponent* InBobber,
                                   UNiagaraComponent* InSplashEffect,
                                   AFishingCharacter* InOwnerCharacter)
{
	CharacterMesh = InCharacterMesh;
	FishingRod = InFishingRod;
	Bobber = InBobber;
	SplashEffect = InSplashEffect;
	OwnerCharacter = InOwnerCharacter;
	CurrentFishingRodSocket = IdleFishingSocket;
	UpdateFishingRodSocket(IdleFishingSocket);

	if (Bobber)
	{
		Bobber->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Bobber->SetCollisionObjectType(ECC_WorldDynamic);
		Bobber->SetCollisionResponseToAllChannels(ECR_Ignore);
		Bobber->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Bobber->SetNotifyRigidBodyCollision(true);
		Bobber->SetGenerateOverlapEvents(false);
		Bobber->BodyInstance.bNotifyRigidBodyCollision = true;
		Bobber->BodyInstance.bUseCCD = false;
		Bobber->SetSimulatePhysics(false);
		UE_LOG(LogFishingComponent, Log, TEXT("Bobber collision configured"));
	}

	if (SplashEffect)
	{
		SplashEffect->SetVisibility(false);
		SplashEffect->SetAutoActivate(false);
		SplashEffect->Deactivate();
		UE_LOG(LogFishingComponent, Log, TEXT("Niagara SplashEffect initialized"));
	}

	UE_LOG(LogFishingComponent, Log, TEXT("FishingComponent initialized"));
}

void UFishingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBobberActive)
	{
		UpdateBobberMovement(DeltaTime);
	}

	if (FishState == EFishingState::BiteFight)
	{
		HandleBiteFight(DeltaTime);
	}
}

const TCHAR* UFishingComponent::FishingStateToStr(EFishingState S)
{
	switch (S)
	{
	case EFishingState::None: return TEXT("None");
	case EFishingState::Casting: return TEXT("Casting");
	case EFishingState::Idle: return TEXT("Idle");
	case EFishingState::BiteWindow: return TEXT("BiteWindow");
	case EFishingState::BiteFight: return TEXT("BiteFight");
	case EFishingState::PullOut: return TEXT("PullOut");
	case EFishingState::ShowOff: return TEXT("ShowOff");
	case EFishingState::Losing: return TEXT("Losing");
	case EFishingState::Exit: return TEXT("Exit");
	default: return TEXT("Unknown");
	}
}

void UFishingComponent::OnPrimaryInput()
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("OnPrimaryInput: Owner is nullptr!"));
		return;
	}
	if (!OwnerCharacter->IsLocallyControlled())
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("OnPrimaryInput: Not locally controlled, ignoring"));
		return;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("OnPrimaryInput! IsFishing=%d State=%s"),
	       bIsFishing, FishingStateToStr(FishState));

	Server_RequestPrimary();
}

void UFishingComponent::OnRep_CurrentSocket()
{
	if (FishingRod && CharacterMesh && CurrentFishingRodSocket != NAME_None)
	{
		FishingRod->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			CurrentFishingRodSocket);
	}
}

void UFishingComponent::OnRep_CurrentDisplayFishItem()
{
	if (CurrentDisplayFishItem)
	{
		UE_LOG(LogFishingComponent, Log, TEXT("[OnRep] CurrentDisplayFishItem: %s"),
		       *CurrentDisplayFishItem->GetNameFromItem());
		AttachAndRevealShowOffItem();
	}
}

void UFishingComponent::Multicast_ShowFishItem_Implementation()
{
	if (!CurrentDisplayFishItem)
	{
		return;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("[Multicast] Showing fish item on client"));

	if (CharacterMesh && ShowOffSocket != NAME_None)
	{
		CurrentDisplayFishItem->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			ShowOffSocket);

		if (USceneComponent* Root = CurrentDisplayFishItem->GetRootComponent())
		{
			Root->SetRelativeLocation(FVector::ZeroVector);
			Root->SetRelativeRotation(FRotator::ZeroRotator);
		}
	}

	CurrentDisplayFishItem->SetHidden(false);

	if (UStaticMeshComponent* ItemMesh = CurrentDisplayFishItem->Mesh)
	{
		ItemMesh->SetVisibility(true, true);
	}
}

void UFishingComponent::Multicast_PlayFishingAnimation_Implementation(FName SectionName, bool bPositionRotHand)
{
	if (bPositionRotHand)
	{
		UpdateFishingRodSocket(ActiveFishingSocket);
	}
	else
	{
		UpdateFishingRodSocket(IdleFishingSocket);
	}

	if (!FishingMontage || !CharacterMesh)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishingMontage or CharacterMesh not set!"));
		return;
	}

	if (UAnimInstance* Anim = CharacterMesh->GetAnimInstance())
	{
		if (!Anim->Montage_IsPlaying(FishingMontage))
		{
			Anim->Montage_Play(FishingMontage, 1.f);
		}
		Anim->Montage_JumpToSection(SectionName, FishingMontage);
		UE_LOG(LogFishingComponent, Log, TEXT("Playing section: %s"), *SectionName.ToString());
	}
}

void UFishingComponent::Server_RequestPrimary_Implementation()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Server_RequestPrimary: Not on server or no authority!"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("Server_RequestPrimary: GetWorld() returned null!"));
		return;
	}

	if (!bIsFishing)
	{
		Server_SetState(EFishingState::Casting);
		LockMovement(true);
		return;
	}

	switch (FishState)
	{
	case EFishingState::Idle:
		Server_SetState(EFishingState::Exit);
		HideBobber();
		break;

	case EFishingState::BiteWindow:
		HandleBiteWindow();
		break;

	default:
		break;
	}
}

void UFishingComponent::EnterFishing_Server()
{
	UE_LOG(LogFishingComponent, Error, TEXT("EnterFishing_Server: Started!"));

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("EnterFishing_Server: GetWorld() returned null!"));
		return;
	}
	if (!OwnerCharacter)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("EnterFishing_Server: Owner is null!"));
		return;
	}

	FVector TargetLocation;
	if (!CheckCastingCollision(TargetLocation))
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Casting blocked by collision!"));
		Server_SetState(EFishingState::Exit);
		HideBobber();
		return;
	}

	bIsFishing = true;
	bInBiteWindow = false;

	BobberTargetLocation = TargetLocation;
	Server_SetState(EFishingState::Idle);
	ShowBobber();
}

void UFishingComponent::ExitFishing_Server()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("ExitFishing_Server: GetWorld() returned null!"));
		return;
	}

	World->GetTimerManager().ClearTimer(Th_BiteDur);
	World->GetTimerManager().ClearTimer(Th_BiteFightDur);

	bIsFishing = false;
	bInBiteWindow = false;
	bBiteFighting = false;

	Server_SetState(EFishingState::None);

	LockMovement(false);
	HideBobber();

	CurrentBitingFish = nullptr;
}

void UFishingComponent::OnRep_FishState()
{
	UE_LOG(LogFishingComponent, Log, TEXT("State -> %s"), FishingStateToStr(FishState));
	if (FishState == EFishingState::ShowOff && CurrentDisplayFishItem)
	{
		AttachAndRevealShowOffItem();
	}
	if (OwnerCharacter)
	{
		OwnerCharacter->OnFishingStateChanged(EFishingState::None, FishState);
	}
}

void UFishingComponent::LockMovement(bool bLock)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
	{
		if (bLock)
		{
			Move->StopMovementImmediately();
			if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
			{
				PC->SetIgnoreMoveInput(true);
			}
		}
		else
		{
			Move->SetMovementMode(MOVE_Walking);
			if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
			{
				PC->SetIgnoreMoveInput(false);
			}
		}
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Movement locked: %d"), bLock);
}

void UFishingComponent::OnBiteTimeout_Server()
{
	if (!bIsFishing)
	{
		return;
	}

	UE_LOG(LogFishingComponent, Warning, TEXT("Bite window timeout - fish got away!"));

	bInBiteWindow = false;
	bWaitingForRealBiteAnimComplete = false;

	Server_SetState(EFishingState::Exit);
	if (CurrentBitingFish)
	{
		CurrentBitingFish->OnEscaped();
		CurrentBitingFish = nullptr;
	}
}

void UFishingComponent::OnAnimNotify_CastEnd()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	EnterFishing_Server();
	UE_LOG(LogFishingComponent, Log, TEXT("Cast complete - waiting for fish"));
}

void UFishingComponent::OnAnimNotify_PullOutEnd()
{
	UE_LOG(LogFishingComponent, Log, TEXT("PullOut animation End"));

	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (CurrentDisplayFishItem)
	{
		Server_SetState(EFishingState::ShowOff);
		OwnerCharacter->SwitchToShowOffCamera(true);
		Multicast_ShowFishItem();
	}
	else
	{
		Server_SetState(EFishingState::Exit);
	}
}

void UFishingComponent::OnAnimNotify_TakingOffEnd()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("TakingOffEnd animation End"));
	ExitFishing_Server();
}

void UFishingComponent::OnAnimNotify_ShowOffEnd()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	OwnerCharacter->SwitchToShowOffCamera(false);

	const bool bAdded = OwnerCharacter->CoreInventoryComponent->TryAddItemFroActor(CurrentDisplayFishItem);
	if (bAdded)
	{
		UE_LOG(LogFishingComponent, Log, TEXT("Fish successfully added to inventory"));
		CurrentDisplayFishItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		CurrentDisplayFishItem->Destroy();
	}
	else
	{
		if (!GetOwner()->HasAuthority())
		{
			return;
		}

		UE_LOG(LogFishingComponent, Warning, TEXT("Inventory full! Dropping fish on ground"));

		AActor* OwnerActor = GetOwner();
		AItemActor* Item = CurrentDisplayFishItem;
		CurrentDisplayFishItem = nullptr;
		if (!OwnerActor || !Item)
		{
			return;
		}

		Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		const FVector OwnerLoc = OwnerActor->GetActorLocation();
		const FVector OwnerFwd = OwnerActor->GetActorForwardVector();
		const FVector TraceStart = OwnerLoc - OwnerFwd * 25.f + FVector(0.f, 0.f, 50.f);
		const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 500.f);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(DropTrace), false);
		Params.AddIgnoredActor(OwnerActor);
		Params.AddIgnoredActor(Item);

		FVector DropLoc = TraceStart;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) && Hit.bBlockingHit)
		{
			DropLoc = Hit.ImpactPoint + FVector(0.f, 0.f, 2.f);
		}

		const FRotator DropRot(0.f, OwnerActor->GetActorRotation().Yaw, 0.f);

		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Item->GetRootComponent()))
		{
			Prim->SetSimulatePhysics(false);
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetEnableGravity(true);
			Prim->SetVisibility(true, true);

			Item->SetReplicateMovement(true);
			Prim->SetIsReplicated(true);

			Item->SetActorLocationAndRotation(DropLoc, DropRot, false, nullptr, ETeleportType::TeleportPhysics);

			Prim->SetSimulatePhysics(true);
			Prim->WakeAllRigidBodies();
			const FVector Nudge = (-OwnerFwd * 50.f) + FVector(0.f, 0.f, 100.f);
			Prim->AddImpulse(Nudge, NAME_None, true);

			Item->ForceNetUpdate();
		}
		else
		{
			Item->SetActorLocationAndRotation(DropLoc, DropRot, false, nullptr, ETeleportType::TeleportPhysics);
			Item->ForceNetUpdate();
		}
	}

	UE_LOG(LogFishingComponent, Log, TEXT("ShowOff animation complete"));
	Server_SetState(EFishingState::Exit);
}

void UFishingComponent::PlayFishing(FName SectionName, bool bPositionRotHand)
{
	if (bPositionRotHand)
	{
		UpdateFishingRodSocket(ActiveFishingSocket);
	}
	else
	{
		UpdateFishingRodSocket(IdleFishingSocket);
	}

	if (!FishingMontage || !CharacterMesh)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishingMontage or CharacterMesh not set!"));
		return;
	}

	if (UAnimInstance* Anim = CharacterMesh->GetAnimInstance())
	{
		if (!Anim->Montage_IsPlaying(FishingMontage))
		{
			Anim->Montage_Play(FishingMontage, 1.f);
		}
		Anim->Montage_JumpToSection(SectionName, FishingMontage);
		UE_LOG(LogFishingComponent, Log, TEXT("Playing section: %s"), *SectionName.ToString());
	}
	else
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("AnimInstance not found!"));
	}
}

void UFishingComponent::PlayFishing(FName SectionName)
{
	PlayFishingMontageSection(SectionName);
}

void UFishingComponent::PlayFishingMontageSection(FName SectionName)
{
	if (!FishingMontage || !CharacterMesh)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishingMontage or CharacterMesh not set!"));
		return;
	}

	if (UAnimInstance* Anim = CharacterMesh->GetAnimInstance())
	{
		if (!Anim->Montage_IsPlaying(FishingMontage))
		{
			Anim->Montage_Play(FishingMontage, 1.f);
		}
		Anim->Montage_JumpToSection(SectionName, FishingMontage);
		UE_LOG(LogFishingComponent, Log, TEXT("Playing section: %s"), *SectionName.ToString());
	}
	else
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("AnimInstance not found!"));
	}
}

void UFishingComponent::PlaySplashEffect(float Scale)
{
	if (!SplashEffect)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("PlaySplashEffect: SplashEffect is null!"));
		return;
	}

	SplashEffect->SetFloatParameter(FName("Scale"), Scale);

	SplashEffect->Deactivate();
	SplashEffect->Activate(true);

	UE_LOG(LogFishingComponent, Log, TEXT("PlaySplashEffect: Niagara effect activated with scale %.2f"), Scale);
}

bool UFishingComponent::CheckCastingCollision(FVector& OutTargetLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("CheckCastingCollision: GetWorld() returned null!"));
		return false;
	}
	if (!OwnerCharacter)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("CheckCastingCollision: Owner is null!"));
		return false;
	}

	const FVector CharLoc = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();

	const FVector StartLoc = CharLoc + Forward * CastCheckForwardOffset;
	
	const FVector EndLoc = StartLoc + FVector(0.f, 0.f, -CastCheckDownOffset);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	bool bHit = World->LineTraceSingleByChannel(
		Hit,
		StartLoc,
		EndLoc,
		ECC_Visibility,
		QueryParams
	);

	if (!bHit)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, FVector::ZeroVector, false, TEXT("No Hit"));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: No surface hit"));
		return false;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, Hit.Location, false, TEXT("No Actor"));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: Hit location has no actor"));
		return false;
	}

	bool bIsFishingWater = HitActor->ActorHasTag(FishingWaterTag);
	
	AFishSpawnPool* HitPool = Cast<AFishSpawnPool>(HitActor);
	if (!bIsFishingWater && !HitPool)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, Hit.Location, false, 
				FString::Printf(TEXT("Not Water (Actor: %s)"), *HitActor->GetName()));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: Hit actor '%s' is not FishingWater"), 
			*HitActor->GetName());
		return false;
	}

	const float CastHeight = FVector::Dist(StartLoc, Hit.Location);
	if (CastHeight > CastMaxHeight)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, Hit.Location, false, 
				FString::Printf(TEXT("Too High: %.1fcm > %.1fcm"), CastHeight, CastMaxHeight));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: Height %.1f exceeds max %.1f"), 
			CastHeight, CastMaxHeight);
		return false;
	}

	OutTargetLocation = Hit.Location;

	if (bShowDebugCasting)
	{
		DrawCastingDebug(StartLoc, EndLoc, Hit.Location, true, 
			FString::Printf(TEXT("Success! Height: %.1fcm"), CastHeight));
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Cast successful: Height=%.1f, Target=%s"), 
		CastHeight, *OutTargetLocation.ToString());

	return true;
}

void UFishingComponent::DrawCastingDebug(const FVector& StartLoc, const FVector& EndLoc, const FVector& HitLoc,
										 bool bValid, const FString& Message)
{
	if (!GetWorld())
	{
		return;
	}

	constexpr float Duration = 3.0f;
	const FColor LineColor = bValid ? FColor::Green : FColor::Red;

	// 캐릭터에서 StartLoc까지 선
	if (OwnerCharacter)
	{
		DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(), StartLoc, 
			FColor::Cyan, false, Duration, 0, 2.0f);
		DrawDebugSphere(GetWorld(), StartLoc, 10.0f, 8, FColor::Cyan, false, Duration, 0, 1.0f);
	}

	// Trace 라인
	DrawDebugLine(GetWorld(), StartLoc, EndLoc, LineColor, false, Duration, 0, 3.0f);

	// Hit 위치 표시
	if (!HitLoc.IsZero())
	{
		DrawDebugSphere(GetWorld(), HitLoc, 15.0f, 12, LineColor, false, Duration, 0, 2.0f);
		
		// 높이 표시선 (수직)
		const float Height = FVector::Dist(StartLoc, HitLoc);
		DrawDebugLine(GetWorld(), StartLoc, HitLoc, FColor::Yellow, false, Duration, 0, 2.0f);
		
		// 높이 텍스트
		const FVector MidPoint = (StartLoc + HitLoc) * 0.5f;
		DrawDebugString(GetWorld(), MidPoint + FVector(30, 0, 0),
			FString::Printf(TEXT("%.1f cm"), Height),
			nullptr, FColor::White, Duration, true, 1.2f);

		if (bValid)
		{
			// 성공 시 타겟 위치 강조
			DrawDebugSphere(GetWorld(), HitLoc, 25.0f, 12, FColor::Green, false, Duration, 0, 1.0f);
			DrawDebugString(GetWorld(), HitLoc + FVector(0, 0, 50),
				TEXT("✓ VALID TARGET"),
				nullptr, FColor::Green, Duration, true, 1.5f);
		}
	}

	// 메시지 표시
	if (!Message.IsEmpty())
	{
		FVector MessageLoc = HitLoc.IsZero() ? EndLoc : HitLoc;
		DrawDebugString(GetWorld(), MessageLoc + FVector(0, 0, 80),
			Message,
			nullptr, bValid ? FColor::Green : FColor::Red, Duration, true, 1.2f);
	}
}

void UFishingComponent::HandleBiteWindow()
{
	if (!GetOwner() || !GetWorld())
	{
		return;
	}

	Server_SetState(EFishingState::BiteFight);
}

void UFishingComponent::InitBiteFight()
{
	if (!GetOwner() || !GetWorld() || !CurrentBitingFish)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(Th_BiteDur);
	bBiteFighting = true;
	BiteFightTimer = 0.f;

	BobberOrbitCenter = BobberTargetLocation;
	BobberOrbitAngle = 0.f;

	float FightDur = 3.f;
	if (UFishData* FishData = CurrentBitingFish->GetFishData())
	{
		FightDur = FishData->BiteFightingTime;
		UE_LOG(LogFishingComponent, Log, TEXT("BiteFight started (%.2fs) with %s"),
		       FightDur, *FishData->FishID.ToString());
	}
	else
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishData null. Using default BiteFight=3.0s"));
	}

	World->GetTimerManager().SetTimer(Th_BiteFightDur, this, &UFishingComponent::FinishBiteFight, FightDur, false);
}

void UFishingComponent::HandleBiteFight(float DeltaSeconds)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	if (!CurrentBitingFish)
	{
		return;
	}

	// 원 궤도 각도 업데이트
	BobberOrbitAngle += BobberOrbitAngularSpeed * DeltaSeconds;

	// 물고기 길이 가져오기
	const float FishLength = CurrentBitingFish->GetFishLength();
	
	// 물고기 길이에 해당하는 각도 계산 (호의 길이 = 반지름 × 각도)
	const float AngleDelta = FishLength / BobberOrbitRadius;
	
	// 머리 위치 (현재 각도)
	const float HeadAngle = BobberOrbitAngle;
	const float HeadX = FMath::Cos(HeadAngle) * BobberOrbitRadius;
	const float HeadY = FMath::Sin(HeadAngle) * BobberOrbitRadius;
	FVector HeadTargetPos = BobberOrbitCenter;
	HeadTargetPos.X += HeadX;
	HeadTargetPos.Y += HeadY;
	HeadTargetPos.Z += -RealBiteSinkAmount * 0.5f;
	
	// 꼬리 위치 (각도를 뺀 위치 - 뒤따라오는 형태)
	const float TailAngle = BobberOrbitAngle - AngleDelta;
	const float TailX = FMath::Cos(TailAngle) * BobberOrbitRadius;
	const float TailY = FMath::Sin(TailAngle) * BobberOrbitRadius;
	FVector TailTargetPos = BobberOrbitCenter;
	TailTargetPos.X += TailX;
	TailTargetPos.Y += TailY;
	TailTargetPos.Z += -RealBiteSinkAmount * 0.5f;
	
	// Fish를 머리 위치로 부드럽게 이동
	const FVector CurrentLoc = CurrentBitingFish->GetActorLocation();
	const FVector NewLoc = FMath::VInterpTo(CurrentLoc, HeadTargetPos, DeltaSeconds, 8.0f);
	CurrentBitingFish->SetActorLocation(NewLoc, false);
	
	// 꼬리에서 머리로 향하는 방향으로 회전 (물고기가 수영하는 방향)
	FVector SwimDirection = HeadTargetPos - TailTargetPos;
	SwimDirection.Z = 0.f; // 수평 방향만
	
	if (SwimDirection.SizeSquared() > 0.01f)
	{
		SwimDirection.Normalize();
		const FRotator TargetRot = SwimDirection.Rotation();
		const FRotator NewRot = FMath::RInterpTo(
			CurrentBitingFish->GetActorRotation(),
			TargetRot,
			DeltaSeconds,
			8.0f
		);
		CurrentBitingFish->SetActorRotation(NewRot);
	}
	
	// 디버그 표시
	if (bShowDebugCasting)
	{
		// 목표 머리/꼬리 위치
		DrawDebugSphere(GetWorld(), HeadTargetPos, 10.f, 12, FColor::Orange, false, -1.f, 0, 2.f);
		DrawDebugSphere(GetWorld(), TailTargetPos, 10.f, 12, FColor::Magenta, false, -1.f, 0, 2.f);
		DrawDebugLine(GetWorld(), TailTargetPos, HeadTargetPos, FColor::Yellow, false, -1.f, 0, 3.f);
		
		// 궤도 표시
		DrawDebugCircle(GetWorld(), BobberOrbitCenter, BobberOrbitRadius, 32, FColor::Green, false, -1.f, 0, 1.f,
		                FVector(0, 1, 0), FVector(1, 0, 0), false);
		
		// 물고기 방향 화살표
		DrawDebugDirectionalArrow(GetWorld(), CurrentBitingFish->GetActorLocation(),
		                          CurrentBitingFish->GetActorLocation() + CurrentBitingFish->GetActorForwardVector() * 50.f,
		                          20.f, FColor::White, false, -1.f, 0, 3.f);
	}
}

void UFishingComponent::FinishBiteFight()
{
	if (!GetOwner() || !GetWorld())
	{
		return;
	}
	bBiteFighting = false;

	ProcessFishingSuccess();
}

void UFishingComponent::ProcessFishingSuccess()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("ProcessFishingSuccess: GetWorld() returned null!"));
		return;
	}

	Server_SetState(EFishingState::PullOut);
	bInBiteWindow = false;
	World->GetTimerManager().ClearTimer(Th_BiteFightDur);
	HideBobber();

	if (!CurrentBitingFish)
	{
		return;
	}

	if (UFishData* FishData = CurrentBitingFish->GetFishData())
	{
		if (FishData)
		{
			if (CurrentDisplayFishItem)
			{
				CurrentDisplayFishItem->Destroy();
				CurrentDisplayFishItem = nullptr;
			}

			UItemBase* ItemFish = FishData->CreateItemFromDataAsset(this);

			const FTransform SpawnTM(FRotator::ZeroRotator, FVector::ZeroVector);

			CurrentDisplayFishItem = ItemFish->SpawnItemActor(
				GetWorld(),
				SpawnTM,
				ItemActorSubClass,
				GetOwner()
			);

			if (CurrentDisplayFishItem)
			{
				CurrentDisplayFishItem->SetHidden(true);
				UE_LOG(LogFishingComponent, Log, TEXT("Fish item spawned: %s"),
				       *CurrentDisplayFishItem->GetNameFromItem());

				const FVector FinalScale = FishData->MeshScale.IsNearlyZero() ? FVector(10.f) : FishData->MeshScale;
				CurrentDisplayFishItem->SetActorScale3D(FinalScale);

				if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CurrentDisplayFishItem->GetRootComponent()))
				{
					Prim->SetSimulatePhysics(false);
					Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}

				if (CharacterMesh && CharacterMesh->DoesSocketExist(ShowOffSocket))
				{
					CurrentDisplayFishItem->AttachToComponent(
						CharacterMesh,
						FAttachmentTransformRules::SnapToTargetNotIncludingScale,
						ShowOffSocket);

					UE_LOG(LogFishingComponent, Log, TEXT("Attached to socket: %s"), *ShowOffSocket.ToString());
				}

				if (USceneComponent* Root = CurrentDisplayFishItem->GetRootComponent())
				{
					Root->SetRelativeLocation(FVector::ZeroVector);
					Root->SetRelativeRotation(FRotator::ZeroRotator);
				}

				CurrentDisplayFishItem->SetHidden(true);
				UE_LOG(LogFishingComponent, Log, TEXT("Fish item setup complete"));
			}
			else
			{
				UE_LOG(LogFishingComponent, Error, TEXT("Failed to spawn fish item!"));
			}
		}

		CurrentBitingFish->OnCaught();
		CurrentBitingFish = nullptr;
	}
}

void UFishingComponent::UpdateFishingRodSocket(FName DesiredSocket)
{
	if (!FishingRod || !CharacterMesh)
	{
		return;
	}

	if (FishingRod->GetAttachSocketName() != DesiredSocket)
	{
		if (OwnerCharacter && OwnerCharacter->HasAuthority())
		{
			CurrentFishingRodSocket = DesiredSocket;
		}

		FishingRod->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			DesiredSocket);

		UE_LOG(LogFishingComponent, Log, TEXT("FishingRod attached to socket: %s"), *DesiredSocket.ToString());
	}
}

void UFishingComponent::OnRep_BobberActive()
{
	if (bBobberActive)
	{
		ShowBobber();
	}
	else
	{
		HideBobber();
	}
}

void UFishingComponent::OnRep_BobberTarget()
{
	if (bBobberActive)
	{
		UpdateBobberMovement(0.f);
	}
}

void UFishingComponent::UpdateBobberMovement(float DeltaSeconds)
{
	if (!Bobber || !bBobberActive)
	{
		return;
	}

	BobberAnimTimer += DeltaSeconds;

	if (FishState == EFishingState::BiteFight)
	{
		BobberOrbitAngle += BobberOrbitAngularSpeed * DeltaSeconds;

		const float X = FMath::Cos(BobberOrbitAngle) * BobberOrbitRadius;
		const float Y = FMath::Sin(BobberOrbitAngle) * BobberOrbitRadius;

		FVector NewLoc = BobberOrbitCenter;
		NewLoc.X += X;
		NewLoc.Y += Y;

		Bobber->SetWorldLocation(NewLoc);
		return;
	}

	FVector NewLoc = BobberTargetLocation;
	float ZOffset = 0.f;

	switch (BobberState)
	{
	case EBobberState::Idle:
		ZOffset = 0.f;
		break;

	case EBobberState::FakeBite:
		if (bBobberAnimating)
		{
			const float Progress = BobberAnimTimer * FakeBiteAnimSpeed;
			if (Progress >= PI * 2.0f)
			{
				bBobberAnimating = false;
				BobberAnimTimer = 0.f;
				BobberState = EBobberState::Idle;
				ZOffset = 0.f;
			}
			else
			{
				ZOffset = FMath::Sin(Progress) * FakeBiteDipAmount;
			}
		}
		break;

	case EBobberState::RealBite:
		if (bBobberAnimating)
		{
			const float Progress = BobberAnimTimer * RealBiteAnimSpeed;
			UE_LOG(LogFishingComponent, Log, TEXT("RealBite animation complete - %f"), Progress);

			if (Progress >= 1.0f)
			{
				bBobberAnimating = false;
				ZOffset = -RealBiteSinkAmount;

				if (bWaitingForRealBiteAnimComplete && OwnerCharacter && OwnerCharacter->HasAuthority())
				{
					bWaitingForRealBiteAnimComplete = false;
					StartBiteWindowTimer();

					UE_LOG(LogFishingComponent, Log, TEXT("RealBite animation complete - BiteWindow timer started"));
				}
			}
			else
			{
				const float EasedProgress = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
				ZOffset = -RealBiteSinkAmount * EasedProgress;
			}
		}
		else
		{
			ZOffset = -RealBiteSinkAmount;
		}
		break;

	case EBobberState::Hidden:
	default:
		return;
	}

	NewLoc.Z += ZOffset;
	Bobber->SetWorldLocation(NewLoc);
}

void UFishingComponent::ShowBobber()
{
	if (!Bobber)
	{
		return;
	}

	const FVector InitialLoc = BobberTargetLocation;

	Bobber->SetWorldLocation(InitialLoc);
	Bobber->SetVisibility(true, true);

	if (SplashEffect)
	{
		SplashEffect->SetVisibility(true, true);
	}

	bBobberActive = true;
	BobberState = EBobberState::Idle;
	BobberAnimTimer = 0.f;
	bBobberAnimating = false;

	PlaySplashEffect(0.5f);

	UE_LOG(LogFishingComponent, Log, TEXT("Bobber shown at: %s (Idle state)"), *InitialLoc.ToString());
}

void UFishingComponent::HideBobber()
{
	if (!Bobber)
	{
		return;
	}

	Bobber->SetVisibility(false, true);

	if (SplashEffect)
	{
		SplashEffect->Deactivate();
		SplashEffect->SetVisibility(false, true);
	}

	bBobberActive = false;
	BobberState = EBobberState::Hidden;
	BobberAnimTimer = 0.f;
	bBobberAnimating = false;

	UE_LOG(LogFishingComponent, Log, TEXT("Bobber hidden"));
}

void UFishingComponent::StartBiteWindowTimer()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	if (!CurrentBitingFish)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("StartBiteWindowTimer: GetWorld() returned null!"));
		return;
	}

	World->GetTimerManager().ClearTimer(Th_BiteDur);

	float BiteWindowDuration = 3.f;
	if (UFishData* FishData = CurrentBitingFish->GetFishData())
	{
		BiteWindowDuration = FishData->BiteWindowTime;
		UE_LOG(LogFishingComponent, Log, TEXT("BiteWindow timer started (%.2fs) for %s"),
		       BiteWindowDuration, *FishData->FishID.ToString());
	}
	else
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishData null. Using default BiteWindow=3.0s"));
	}

	World->GetTimerManager().SetTimer(
		Th_BiteDur,
		this,
		&UFishingComponent::OnBiteTimeout_Server,
		BiteWindowDuration,
		false);

	UE_LOG(LogFishingComponent, Log, TEXT("BiteWindow timer started successfully"));
}

void UFishingComponent::PlayBobberFakeBite()
{
	if (!bBobberActive)
	{
		return;
	}
	BobberState = EBobberState::FakeBite;
	BobberAnimTimer = 0.f;
	bBobberAnimating = true;
}

void UFishingComponent::PlayBobberRealBite()
{
	BobberState = EBobberState::RealBite;
	BobberAnimTimer = 0.f;
	bBobberAnimating = true;
	UE_LOG(LogFishingComponent, Log, TEXT("Bobber sinking for RealBite"));
}

void UFishingComponent::OnFishBite(AFish* Fish)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	if (!Fish)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("OnFishBite: GetWorld() returned null!"));
		return;
	}

	if (CurrentBitingFish)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Already have a biting fish - ignoring new bite"));
		return;
	}

	if (!bIsFishing || FishState != EFishingState::Idle)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Not in fishing idle state - ignoring bite"));
		return;
	}

	CurrentBitingFish = Fish;
	bInBiteWindow = true;
	bWaitingForRealBiteAnimComplete = true;

	PlayBobberRealBite();
	Server_SetState(EFishingState::BiteWindow);

	if (UFishData* FishData = Fish->GetFishData())
	{
		UE_LOG(LogFishingComponent, Log, TEXT("Fish %s is REALLY biting! Will start window after bobber sinks"),
		       *FishData->FishID.ToString());
	}
}

void UFishingComponent::OnFishFakeBite(AFish* Fish, float FakeBiteTime)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	if (!Fish)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("OnFakeFishBite: GetWorld() returned null!"));
		return;
	}

	if (!bIsFishing || FishState != EFishingState::Idle)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Not in fishing idle state - ignoring fake bite"));
		return;
	}

	PlayBobberFakeBite();
}

void UFishingComponent::Server_SetState_Implementation(EFishingState NewState)
{
	const EFishingState OldState = FishState;
	FishState = NewState;

	OnStateChanged.Broadcast(OldState, NewState);

	switch (NewState)
	{
	case EFishingState::Casting:
		Multicast_PlayFishingAnimation(Sec_Cast, true);
		break;

	case EFishingState::Idle:
		if (bBobberActive)
		{
			BobberState = EBobberState::Idle;
			BobberAnimTimer = 0.f;
			bBobberAnimating = false;
		}
		Multicast_PlayFishingAnimation(Sec_Idle, true);
		break;

	case EFishingState::BiteWindow:
		Multicast_PlayFishingAnimation(Sec_Bite, true);
		break;

	case EFishingState::BiteFight:
		Multicast_PlayFishingAnimation(Sec_FishFight, true);
		InitBiteFight();
		break;

	case EFishingState::PullOut:
		Multicast_PlayFishingAnimation(Sec_PullOut, true);
		break;

	case EFishingState::ShowOff:
		if (CurrentDisplayFishItem)
		{
			CurrentDisplayFishItem->SetHidden(false);
		}
		Multicast_PlayFishingAnimation(Sec_ShowOff, false);
		break;

	case EFishingState::Losing:
		Multicast_PlayFishingAnimation(Sec_PullOut, false);
		break;

	case EFishingState::Exit:
		Multicast_PlayFishingAnimation(Sec_TakingOff, true);
		break;

	case EFishingState::None:
		UpdateFishingRodSocket(IdleFishingSocket);
		if (CharacterMesh)
		{
			if (UAnimInstance* Anim = CharacterMesh->GetAnimInstance())
			{
				if (FishingMontage)
				{
					Anim->Montage_Stop(0.2f, FishingMontage);
				}
			}
		}
		break;
	}

	OnRep_FishState();
}

void UFishingComponent::AttachAndRevealShowOffItem()
{
	if (!CurrentDisplayFishItem)
	{
		return;
	}

	if (CharacterMesh && ShowOffSocket != NAME_None)
	{
		CurrentDisplayFishItem->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			ShowOffSocket);

		if (USceneComponent* Root = CurrentDisplayFishItem->GetRootComponent())
		{
			Root->SetRelativeLocation(FVector::ZeroVector);
			Root->SetRelativeRotation(FRotator::ZeroRotator);
		}
	}

	CurrentDisplayFishItem->SetHidden(false);
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CurrentDisplayFishItem->GetRootComponent()))
	{
		Prim->SetVisibility(true, true);
		Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
