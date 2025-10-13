#include "FishingAnimationModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Variant_Fishing/ActorComponent/FishingFeatures/FishingComponent.h"

void UFishingAnimationModule::Initialize(UFishingComponent* InOwner,
                                         USkeletalMeshComponent* InCharacterMesh,
                                         UStaticMeshComponent* InFishingRod)
{
	OwnerComponent = InOwner;
	CharacterMesh = InCharacterMesh;
	FishingRod = InFishingRod;
	
	UE_LOG(LogFishingComponent, Log, TEXT("AnimationModule initialized"));
}

void UFishingAnimationModule::PlayMontageSection(FName SectionName)
{
	UE_LOG(LogFishingComponent, Log, TEXT("Enter Montage Section :  %s!"), *SectionName.ToString());

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

void UFishingAnimationModule::PlayWithHandPosition(FName SectionName, bool bPositionRotHand)
{
	UE_LOG(LogFishingComponent, Log, TEXT("PlayWithHandPosition Called"));

	if (bPositionRotHand)
	{
		UpdateFishingRodSocket(ActiveFishingSocket);
	}
	else
	{
		UpdateFishingRodSocket(IdleFishingSocket);
	}

	PlayMontageSection(SectionName);
}

void UFishingAnimationModule::StopMontage(float BlendOutTime)
{
	if (!CharacterMesh || !FishingMontage)
	{
		return;
	}

	if (UAnimInstance* Anim = CharacterMesh->GetAnimInstance())
	{
		if (Anim->Montage_IsPlaying(FishingMontage))
		{
			Anim->Montage_Stop(BlendOutTime, FishingMontage);
			UE_LOG(LogFishingComponent, Log, TEXT("Stopped fishing montage"));
		}
	}
}

void UFishingAnimationModule::UpdateFishingRodSocket(FName DesiredSocket)
{
	if (!FishingRod || !CharacterMesh)
	{
		return;
	}

	if (FishingRod->GetAttachSocketName() != DesiredSocket)
	{
		CurrentFishingRodSocket = DesiredSocket;
		
		// Update parent component's replicated socket
		if (OwnerComponent && OwnerComponent->GetOwner() && OwnerComponent->GetOwner()->HasAuthority())
		{
			OwnerComponent->CurrentFishingRodSocket = DesiredSocket;
		}

		FishingRod->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			DesiredSocket);

		UE_LOG(LogFishingComponent, Log, TEXT("FishingRod attached to socket: %s"), 
		       *DesiredSocket.ToString());
	}
}

void UFishingAnimationModule::AttachToIdleSocket()
{
	UpdateFishingRodSocket(IdleFishingSocket);
}

void UFishingAnimationModule::AttachToActiveSocket()
{
	UpdateFishingRodSocket(ActiveFishingSocket);
}