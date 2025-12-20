// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LockOnManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, AActor*, NewTarget);

UCLASS(BlueprintType)
class PROTOTYPE_API ULockOnManager : public UObject
{
	GENERATED_BODY()
	
public:
	ULockOnManager();

	UFUNCTION(BlueprintCallable, Category="LockOn")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category="LockOn")
	void SwitchTarget(float Direction);  // -1=左, 1=右

	UFUNCTION(BlueprintCallable, Category="LockOn")
	AActor* GetCurrentTarget() const { return CurrentLockedActor; }

	UPROPERTY(BlueprintAssignable, Category="LockOn")
	FOnLockOnTargetChanged OnLockOnTargetChanged;

	UPROPERTY(BlueprintReadOnly)
    AActor* CurrentLockedActor = nullptr;  // 当前锁定的目标

    void SetLockedTarget(AActor* NewTarget, AController* InstigatorController);
	void ClearLockedTarget(AController* InstigatorController);

private:
	APlayerController* GetPlayerController() const
    {
         return Cast<APlayerController>(GetOuter());
    }

	UFUNCTION()
	void ScanTargets(float ScanRadius);

	UFUNCTION()
	AActor* FindBestTarget();

	UPROPERTY()
	TArray<AActor*> TargetCandidates;
};
