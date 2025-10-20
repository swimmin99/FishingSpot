


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FishingPlayerState.generated.h"

class UDatabaseManager;

UCLASS()
class FISHING_API AFishingPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    AFishingPlayerState();

    
    
    
    
    
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Database")
    int32 DatabasePlayerID = -1;

    
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Database")
    int32 HostDatabasePlayerID = -1;

    
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Database")
    bool bIsHost = false;

    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Database")
    void InitializeAsHost(int32 InDatabasePlayerID);

    
    UFUNCTION(BlueprintCallable, Category = "Database")
    void InitializeAsGuest(int32 InDatabasePlayerID, int32 InHostDatabasePlayerID);

    
    
    
    
    UFUNCTION(BlueprintPure, Category = "Database")
    int32 GetDatabasePlayerID() const { return DatabasePlayerID; }

    UFUNCTION(BlueprintPure, Category = "Database")
    int32 GetHostDatabasePlayerID() const { return HostDatabasePlayerID; }

    UFUNCTION(BlueprintPure, Category = "Database")
    bool IsHostPlayer() const { return bIsHost; }

    
    
    
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
};


