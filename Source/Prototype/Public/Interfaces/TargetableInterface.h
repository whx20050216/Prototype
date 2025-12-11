// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

class PROTOTYPE_API ITargetableInterface
{
	GENERATED_BODY()

public:
	// LockOnManager调用：获取目标位置（用于计算距离、显示标记）
	UFUNCTION(BlueprintNativeEvent, Category="Target")
	FVector GetTargetLocation() const;
	virtual FVector GetTargetLocation_Implementation() const { return FVector::ZeroVector; }

	// LockOnManager调用：获取优先级（Boss=100，杂兵=1）
	UFUNCTION(BlueprintNativeEvent, Category="Target")
	float GetLockPriority() const;
	virtual float GetLockPriority_Implementation() const { return 1.f; }

	// LockOnManager调用：当被锁定时，敌人做什么？
	UFUNCTION(BlueprintNativeEvent, Category="Target")
	void OnLocked(AController* Locker);
	virtual void OnLocked_Implementation(AController* Locker) {}

	// LockOnManager调用：当解锁时，敌人做什么？
	UFUNCTION(BlueprintNativeEvent, Category="Target")
	void OnUnlocked();
	virtual void OnUnlocked_Implementation() {}
};
