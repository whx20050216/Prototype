// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/LockOnManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/TargetableInterface.h"

ULockOnManager::ULockOnManager()
{
	CurrentTarget = nullptr;
}

void ULockOnManager::ToggleLockOn()
{
	if (CurrentTarget)
	{
		// 解锁时：调用接口通知目标"你自由了"
		// 这触发敌人的OnUnlocked()回调，让它隐藏锁定标记
		ITargetableInterface::Execute_OnUnlocked(CurrentTarget);

		CurrentTarget = nullptr;
		OnLockOnTargetChanged.Broadcast(nullptr);
	}
	else
	{
		// 锁定前：扫描周围20米
		ScanTargets(2000.f);

		//找到最佳目标
		CurrentTarget = FindBestTarget();

		APlayerController* PC = GetPlayerController();

		if (CurrentTarget && PC)
		{
			// 锁定时：调用接口通知目标"你被锁定了"
			// 这触发敌人的OnLocked()回调，让它显示头顶标记
			ITargetableInterface::Execute_OnLocked(CurrentTarget, PC);

			OnLockOnTargetChanged.Broadcast(CurrentTarget);
		}
	}
}

void ULockOnManager::SwitchTarget(float Direction)
{
}

void ULockOnManager::ScanTargets(float ScanRadius)
{
	TargetCandidates.Empty();

	APlayerController* PC = GetPlayerController();
	if (!PC) return;

	APawn* OwnerPawn = Cast<APawn>(PC->GetPawn());
	if (!OwnerPawn) return;

	FVector Location = OwnerPawn->GetActorLocation();
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Location,
		ScanRadius,
		ObjectTypes,
		nullptr,  // 不限制类，扫描所有Actor
		{OwnerPawn},  // 忽略数组（自己）
		OverlappedActors
	);

	for (AActor* Actor : OverlappedActors)
	{
		if (Actor && Actor->Implements<UTargetableInterface>())
		{
			TargetCandidates.Add(Actor);
		}
	}
}

AActor* ULockOnManager::FindBestTarget()
{
	if (TargetCandidates.Num() == 0) return nullptr;

	APlayerController* PC = GetPlayerController();
	if (!PC) return nullptr;

	APawn* OwnerPawn = PC->GetPawn();
	if (!OwnerPawn) return nullptr;

	// 找最近的目标
	AActor* BestTarget = nullptr;
	float MinDistance = FLT_MAX;
	FVector MyLocation = OwnerPawn->GetActorLocation();

	for (AActor* Candidate : TargetCandidates)
	{
		float Dist = FVector::Dist(MyLocation, Candidate->GetActorLocation());
		if (Dist < MinDistance)
		{
			MinDistance = Dist;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}
