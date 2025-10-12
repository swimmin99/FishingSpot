// InteractableCharacter.cpp
#include "InteractableCharacter.h"
#include "Net/UnrealNetwork.h"

AInteractableCharacter::AInteractableCharacter()
{
	bReplicates = true;
}

void AInteractableCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AInteractableCharacter, Gold);
}

void AInteractableCharacter::OnRep_Gold()
{
	// 클라에서 HUD 갱신 트리거
	OnGoldChanged.Broadcast(Gold);
}

void AInteractableCharacter::ApplyGoldDelta(int32 Delta)
{
	if (!HasAuthority()) return;
	const int32 NewGold = FMath::Max(0, Gold + Delta);
	if (NewGold == Gold) return;
	Gold = NewGold;

	// 서버에서도 로컬 HUD(리슨 서버 등) 갱신 신호
	OnGoldChanged.Broadcast(Gold);
}

void AInteractableCharacter::AddGold(int32 Amount)
{
	if (HasAuthority())
	{
		ApplyGoldDelta(Amount);
	}
	else
	{
		Server_AddGold(Amount);
	}
}

void AInteractableCharacter::SetGold(int32 NewGold)
{
	if (HasAuthority())
	{
		const int32 Clamped = FMath::Max(0, NewGold);
		if (Clamped == Gold) return;
		Gold = Clamped;
		OnGoldChanged.Broadcast(Gold);
	}
	else
	{
		Server_SetGold(NewGold);
	}
}

void AInteractableCharacter::Server_AddGold_Implementation(int32 Amount)
{
	ApplyGoldDelta(Amount);
}

void AInteractableCharacter::Server_SetGold_Implementation(int32 NewGold)
{
	SetGold(NewGold); // 위에서 권위 체크 포함
}
