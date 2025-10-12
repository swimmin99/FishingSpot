// InteractableCharacter.h
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

	// 서버 권위 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Gold, Category="Player")
	int32 Gold = 10000;

	// 변경 알림
	UPROPERTY(BlueprintAssignable, Category="Player")
	FOnGoldChanged OnGoldChanged;

	// 골드 증감(클라에서 호출해도 됨 → 서버로 RPC)
	UFUNCTION(BlueprintCallable, Category="Player")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Player")
	int32 GetGold() const { return Gold; }

	// 직접 세팅이 필요하면 이것도 서버로
	UFUNCTION(BlueprintCallable, Category="Player")
	void SetGold(int32 NewGold);

protected:
	// RepNotify
	UFUNCTION()
	void OnRep_Gold();

	// 내부 적용(서버에서만 호출)
	void ApplyGoldDelta(int32 Delta);

	// RPC
	UFUNCTION(Server, Reliable)
	void Server_AddGold(int32 Amount);

	UFUNCTION(Server, Reliable)
	void Server_SetGold(int32 NewGold);

	// replication 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
