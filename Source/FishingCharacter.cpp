

#include "FishingCharacter.h"
#include "Variant_Fishing/ActorComponent/FishingComponent.h"
#include "FishingPlayerController.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "NiagaraComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/ActorComponent/InventoryComponent.h"
#include "Variant_Fishing/NPC/ShopCharacter.h"
#include "Variant_Fishing/Interface/Interactable.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AFishingCharacter::AFishingCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement   = true;
	Move->RotationRate                = FRotator(0.f, 500.f, 0.f);
	Move->JumpZVelocity               = 500.f;
	Move->AirControl                  = 0.35f;
	Move->MaxWalkSpeed                = 500.f;
	Move->MinAnalogWalkSpeed          = 20.f;
	Move->BrakingDecelerationWalking  = 2000.f;
	Move->BrakingDecelerationFalling  = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	ShowOffCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ShowOffCameraBoom"));
	ShowOffCameraBoom->SetupAttachment(RootComponent);
	ShowOffCameraBoom->TargetArmLength = 400.f;
	ShowOffCameraBoom->bUsePawnControlRotation = false;
	
	ShowOffCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ShowOffCamera"));
	ShowOffCamera->SetupAttachment(ShowOffCameraBoom, USpringArmComponent::SocketName);
	ShowOffCamera->bUsePawnControlRotation = false;
	ShowOffCamera->SetActive(false); 

	
	FishingRod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FishingRod"));
	FishingRod->SetupAttachment(GetMesh());

	Bobber = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bobber"));
	Bobber->SetupAttachment(RootComponent);
	Bobber->SetVisibility(false);
	Bobber->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplashEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BobberEffectComponent"));
	SplashEffectComponent->SetupAttachment(Bobber);
	SplashEffectComponent->SetAutoActivate(false);
	
	
	InteractionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionCapsule"));
	InteractionCapsule->SetupAttachment(RootComponent);
	InteractionCapsule->InitCapsuleSize(50.f, 50.f);
	
	
	InteractionCapsule->SetRelativeLocation(FVector(80.f, 0.f, 0.f));
	
	
	InteractionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCapsule->SetCollisionResponseToAllChannels(ECR_Overlap);
	
	
	InteractionCapsule->OnComponentBeginOverlap.AddDynamic(this, &AFishingCharacter::OnInteractionBeginOverlap);
	InteractionCapsule->OnComponentEndOverlap.AddDynamic(this, &AFishingCharacter::OnInteractionEndOverlap);

	CoreFishingComponent = CreateDefaultSubobject<UFishingComponent>(TEXT("FishingComponent"));
	CoreInventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));


	
	PrimaryActorTick.bCanEverTick = true;
}

void AFishingCharacter::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemplateCharacter, Log, TEXT("OnInteractionBeginOverlap with %s"), *GetNameSafe(OtherActor));
	if (!OtherActor || OtherActor == this) return;

	IInteractable* InteractableTarget = Cast<IInteractable>(OtherActor);
	if (!InteractableTarget)
	{
		return;
	}
	
	
	if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
	{
		PC->ShowInteractionPrompt(OtherActor);
	}
	
	CurrentOverlapActor = OtherActor;
}

void AFishingCharacter::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == CurrentOverlapActor)
	{
		CurrentOverlapActor = nullptr;
		
		
		if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
		{
			PC->HideInteractionPrompt();
		}
	}
}

void AFishingCharacter::ExecuteInteract()
{
	if (!CurrentOverlapActor) return;
	
	const IInteractable* InteractableTarget = Cast<IInteractable>(CurrentOverlapActor);
	if (!InteractableTarget)
		return;
	
	if (InteractableTarget->GetType_Implementation() == EInteractionType::Shop)
	{
		if (AShopCharacter* ShopActor = Cast<AShopCharacter>(CurrentOverlapActor))
		{
			if (IsLocallyControlled())
			{
				
				if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
				{
					PC->OpenShopUI(ShopActor, CoreInventoryComponent);
				}
				return;
			}
		}
	} 
	else if (InteractableTarget->GetType_Implementation() == EInteractionType::Item)
	{
		Server_TryPickup(CurrentOverlapActor); 
		return;
	}
}

void AFishingCharacter::ExecuteEscape()
{
	if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
	{
		PC->ToggleSettings();
	}
}

void AFishingCharacter::Server_TryPickup_Implementation(AActor* Target)
{
	if (!Target || !CoreInventoryComponent) return;

	if (AItemActor* Item = Cast<AItemActor>(Target))
	{
		const bool bAdded = CoreInventoryComponent->TryAddItemFroActor(Item);
		if (bAdded)
		{
			Item->Destroy();
		}
	}
}

void AFishingCharacter::SwitchToShowOffCamera(bool val)
{
	FollowCamera->SetActive(!val);
	ShowOffCamera->SetActive(val);
}

void AFishingCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	
	CoreInventoryComponent->Initalize(this);
	CoreFishingComponent->Initialize(GetMesh(), FishingRod, Bobber, SplashEffectComponent, this);
	
	
	if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			PC->InitializeWidgets(CoreInventoryComponent);
		}
	}

	AssignMeshIndex();
}

void AFishingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,  this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFishingCharacter::Move);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFishingCharacter::Look);
		}
		if (MouseLookAction)
		{
			EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AFishingCharacter::Look);
		}
		if (PrimaryAction)
		{
			EIC->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AFishingCharacter::OnPrimary);
		}
		if (InventoryAction)
		{
			EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &AFishingCharacter::OnToggleInventory);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AFishingCharacter::ExecuteInteract);
		}
		if (EscapeAction)
		{
			EIC->BindAction(EscapeAction, ETriggerEvent::Started, this, &AFishingCharacter::ExecuteEscape);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' EnhancedInputComponent missing"), *GetNameSafe(this));
	}
}

void AFishingCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GEngine && CoreFishingComponent)
	{
		FString StateStr = TEXT("None");
		if (CoreFishingComponent->IsFishing())
		{
			StateStr = UFishingComponent::FishingStateToStr(CoreFishingComponent->GetFishState());
		}

		FString DebugText = FString::Printf(TEXT("State: %s | IsFishing: %s | InBiteWindow: %s | Speed: %.1f"),
			*StateStr,
			CoreFishingComponent->IsFishing() ? TEXT("YES") : TEXT("NO"),
			CoreFishingComponent->IsInBiteWindow() ? TEXT("YES") : TEXT("NO"),
			GetVelocity().Size2D());
		
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Cyan, DebugText);
	}
}

void AFishingCharacter::AssignMeshIndex()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssignMeshIndex] Skipped: no authority on %s"), *GetName());
		return;
	}

	const int32 NumChoices = MeshOptions.Num();
	if (NumChoices <= 0)
	{
		MeshIndex = -1;
		UE_LOG(LogTemp, Warning, TEXT("[AssignMeshIndex] No MeshOptions for %s"), *GetName());
		return;
	}

	APlayerState* PS = GetPlayerState();
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;

	int32 OrderIndex = 0;

	if (GS && PS)
	{
		TArray<APlayerState*> Arr = GS->PlayerArray;
		Arr.Sort([](const APlayerState& A, const APlayerState& B)
		{
			return A.GetPlayerId() < B.GetPlayerId();
		});

		const int32 Found = Arr.IndexOfByKey(PS);
		if (Found != INDEX_NONE)
			OrderIndex = Found;
		else
			OrderIndex = PS->GetPlayerId(); 

		UE_LOG(LogTemp, Log, TEXT("[AssignMeshIndex] Found PlayerState: %s | OrderIndex=%d | TotalPlayers=%d"),
			*PS->GetPlayerName(), OrderIndex, Arr.Num());
	}
	else if (PS)
	{
		OrderIndex = PS->GetPlayerId();
		UE_LOG(LogTemp, Warning, TEXT("[AssignMeshIndex] GameState missing, fallback to PlayerId=%d for %s"),
			OrderIndex, *PS->GetPlayerName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[AssignMeshIndex] Missing PlayerState for %s"), *GetName());
		return;
	}

	MeshIndex = OrderIndex % NumChoices;

	UE_LOG(LogTemp, Log, TEXT("[AssignMeshIndex] Assigned MeshIndex=%d (mod %d) for %s"),
		MeshIndex, NumChoices, *GetName());

	ApplyMeshIndex();
}

void AFishingCharacter::ApplyMeshIndex()
{
	USkeletalMeshComponent* Skel = GetMesh();
	if (!Skel)
	{
		UE_LOG(LogTemp, Error, TEXT("[ApplyMeshIndex] No MeshComponent found on %s"), *GetName());
		return;
	}

	if (MeshOptions.IsValidIndex(MeshIndex) && MeshOptions[MeshIndex])
	{
		Skel->SetSkeletalMesh(MeshOptions[MeshIndex]);
		UE_LOG(LogTemp, Log, TEXT("[ApplyMeshIndex] Applied Mesh[%d] = %s to %s"),
			MeshIndex,
			*MeshOptions[MeshIndex]->GetName(),
			*GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyMeshIndex] Invalid MeshIndex=%d or null entry on %s"),
			MeshIndex, *GetName());
	}
}
void AFishingCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	DoMove(Axis.X, Axis.Y);
}

void AFishingCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	DoLook(Axis.X, Axis.Y);
}

void AFishingCharacter::DoMove(float Right, float Forward)
{
	if (GetController() == nullptr) return;

	const FRotator Rot = GetController()->GetControlRotation();
	const FRotator YawRot(0.f, Rot.Yaw, 0.f);

	const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Rgt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Fwd, Forward);
	AddMovementInput(Rgt, Right);
}

void AFishingCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() == nullptr) return;
	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AFishingCharacter::OnToggleInventory()
{
	
	if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
	{
		PC->ToggleInventory(CoreInventoryComponent);
	}
}

void AFishingCharacter::OnPrimary()
{
	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}

	if (CoreFishingComponent)
	{
		CoreFishingComponent->OnPrimaryInput();
	}
}

void AFishingCharacter::OnFishingStateChanged(EFishingState OldState, EFishingState NewState)
{
	UE_LOG(LogTemplateCharacter, Log, TEXT("Fishing state changed: %s -> %s"), 
		UFishingComponent::FishingStateToStr(OldState),
		UFishingComponent::FishingStateToStr(NewState));
}

void AFishingCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bTearingDown = true;

    
    if (AFishingPlayerController* PC = Cast<AFishingPlayerController>(GetController()))
    {
        PC->HideInteractionPrompt();
        
        
    }

    
    if (InteractionCapsule)
    {
        InteractionCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &AFishingCharacter::OnInteractionBeginOverlap);
        InteractionCapsule->OnComponentEndOverlap.RemoveDynamic(this, &AFishingCharacter::OnInteractionEndOverlap);
    }

    
    if (CoreFishingComponent)
    {
        
    }

    
    CurrentOverlapActor = nullptr;

    Super::EndPlay(EndPlayReason);
}

void AFishingCharacter::BeginDestroy()
{
    // 방어적 중복 해제 (엔진 파괴 순서 이슈 대비)
    if (InteractionCapsule)
    {
        InteractionCapsule->OnComponentBeginOverlap.RemoveAll(this);
        InteractionCapsule->OnComponentEndOverlap.RemoveAll(this);
    }

    CurrentOverlapActor = nullptr;

    Super::BeginDestroy();
}
