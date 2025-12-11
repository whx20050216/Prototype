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
	void SwitchTarget(float Direction);  // -1=×ó, 1=ÓÒ

	UFUNCTION(BlueprintCallable, Category="LockOn")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UPROPERTY(BlueprintAssignable, Category="LockOn")
	FOnLockOnTargetChanged OnLockOnTargetChanged;

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
	AActor* CurrentTarget;

	UPROPERTY()
	TArray<AActor*> TargetCandidates;
};
