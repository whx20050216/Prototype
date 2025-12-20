// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/LockOnManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/TargetableInterface.h"

ULockOnManager::ULockOnManager()
{
	CurrentLockedActor  = nullptr;
}

void ULockOnManager::ToggleLockOn()
{
	APlayerController* PC = GetPlayerController();
	if (!PC) return;

	if (CurrentLockedActor )
	{
		ClearLockedTarget(PC);
	}
	else
	{
		// 锁定前：扫描周围20米
		ScanTargets(2000.f);

		AActor* BestTarget = FindBestTarget();
		if (BestTarget)
		{
			SetLockedTarget(BestTarget, PC);
		}
	}
}

void ULockOnManager::SwitchTarget(float Direction)
{
}

void ULockOnManager::SetLockedTarget(AActor* NewTarget, AController* InstigatorController)
{
	if (CurrentLockedActor == NewTarget) return;

	// 通知旧目标解锁
	if (CurrentLockedActor)
	{
		ITargetableInterface::Execute_OnUnlocked(CurrentLockedActor, InstigatorController);
	}

	CurrentLockedActor = NewTarget;

	// 通知新目标锁定
	if (NewTarget)
	{
		ITargetableInterface::Execute_OnLocked(NewTarget, InstigatorController);
	}

	// 广播委托
	OnLockOnTargetChanged.Broadcast(CurrentLockedActor);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
        FString::Printf(TEXT("Locked: %s"), *GetNameSafe(NewTarget)));
}

void ULockOnManager::ClearLockedTarget(AController* InstigatorController)
{
	// 通知旧目标解锁
	if (CurrentLockedActor)
	{
		ITargetableInterface::Execute_OnUnlocked(CurrentLockedActor, InstigatorController);
	}

	CurrentLockedActor = nullptr;

	// 广播委托
	OnLockOnTargetChanged.Broadcast(nullptr);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unlocked"));
}

void ULockOnManager::ScanTargets(float ScanRadius)
{
    TargetCandidates.Empty();

    APlayerController* PC = GetPlayerController();
    if (!PC) return;

    APawn* OwnerPawn = Cast<APawn>(PC->GetPawn());
    if (!OwnerPawn) return;

    FVector MyLocation = OwnerPawn->GetActorLocation();
    FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
    FVector CameraForward = PC->PlayerCameraManager->GetCameraRotation().Vector();

    // 直接获取所有实现了接口的Actor
    TArray<AActor*> AllTargetableActors;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UTargetableInterface::StaticClass(), AllTargetableActors);

    // 手动筛选：距离 + 屏幕可见 + 摄像机前方
    for (AActor* Actor : AllTargetableActors)
    {
        if (!Actor || Actor == OwnerPawn) continue;

        float Dist = FVector::Dist(MyLocation, Actor->GetActorLocation());
        if (Dist > ScanRadius) continue;  // 距离太远

        // 检查是否在摄像机前方（角度 < 90度）
        FVector ToTarget = (Actor->GetActorLocation() - CameraLocation).GetSafeNormal();
        float Dot = FVector::DotProduct(CameraForward, ToTarget);
        if (Dot < 0.0f) continue;  // 在摄像机后方，跳过

        // 检查是否在屏幕内
        FVector2D ScreenPos;
        if (PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos))
        {
            int32 ScreenWidth, ScreenHeight;
            PC->GetViewportSize(ScreenWidth, ScreenHeight);

            // 允许轻微超出屏幕边缘（10%余量）
            float Margin = 0.1f;
            if (ScreenPos.X >= -ScreenWidth * Margin && 
                ScreenPos.X <= ScreenWidth * (1.0f + Margin) &&
                ScreenPos.Y >= -ScreenHeight * Margin && 
                ScreenPos.Y <= ScreenHeight * (1.0f + Margin))
            {
                TargetCandidates.Add(Actor);
            }
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, 
        FString::Printf(TEXT("Scan Completed，Visible Targets Num: %d"), TargetCandidates.Num()));
}

AActor* ULockOnManager::FindBestTarget()
{
    if (TargetCandidates.Num() == 0) 
    {
        return nullptr;
    }

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