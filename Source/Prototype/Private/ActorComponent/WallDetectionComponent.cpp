// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/WallDetectionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

UWallDetectionComponent::UWallDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UWallDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<ACharacter>(GetOwner());
}

void UWallDetectionComponent::UpdateDetection()
{
	bCanWallRun = false;
	bCanVault = false;
	DetectionResult = EDetectionResult::Clear;

	BottomTrace();
	MiddleTrace();
	TopTrace();

	bool bBottom = BottomHit.bBlockingHit;
	bool bMiddle = MiddleHit.bBlockingHit;
	bool bTop = TopHit.bBlockingHit;

	if (bBottom && bMiddle && bTop)
	{
		DetectionResult = EDetectionResult::WallRunPossible;
		EvaluateResults();
		return;
	}

	if ((bBottom && !bMiddle && !bTop) || (bBottom && bMiddle && !bTop) || (!bBottom && bMiddle && !bTop))
	{
		DetectionResult = EDetectionResult::VaultPossible;
		bCanVault = true;
		UpToDownTrace();
		return;
	}

	if (bBottom || bMiddle || bTop)
	{
		DetectionResult = EDetectionResult::Invalid;
		return;
	}
}

void UWallDetectionComponent::UpToDownTrace()
{
	if (!CharacterOwner || !GetWorld()) return;

	if (!bCanVault) return;

	// 选择参考点：优先用 Bottom，其次用 Middle（两者必有一个命中）
	const FHitResult* ReferenceHit = BottomHit.bBlockingHit ? &BottomHit : &MiddleHit;
	if (!ReferenceHit || !ReferenceHit->bBlockingHit) return;

	FVector TraceStart = ReferenceHit->Location + FVector(10.f, 0.f, 200.f);
	FVector TraceEnd = ReferenceHit->Location;

	GetWorld()->LineTraceSingleByObjectType(
		UpToDownHit,
		TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic), // 检测静态物体
		FCollisionQueryParams(NAME_None, false, CharacterOwner) // 忽略自身
	);

	// Debug绘制：紫色线 + 橙色命中点
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Purple, false, 0.1f, 0, 1.0f);
	if (UpToDownHit.bBlockingHit)
	{
		DrawDebugSphere(GetWorld(), UpToDownHit.Location, 10.0f, 16, FColor::Orange, false, 0.1f);
	}
}

void UWallDetectionComponent::BottomTrace()
{
	if (!CharacterOwner) return;

	FVector Offset = FVector(0.f, 0.f, -65.f);

	FVector Start = CharacterOwner->GetActorLocation() + Offset;
	FVector End = Start + CharacterOwner->GetActorForwardVector() * LineTraceLength;

	if (GetWorld())
	{
		GetWorld()->LineTraceSingleByObjectType(
			BottomHit,
			Start,
			End,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),//检测静态物体
			FCollisionQueryParams(NAME_None, false, CharacterOwner) // 忽略自身
		);
	}
	//Debug
	FColor LineColor = BottomHit.bBlockingHit ? FColor::Red : FColor::Blue;
    DrawDebugLine(
        GetWorld(),
        Start,
        BottomHit.bBlockingHit ? BottomHit.Location : End,  // 命中点画到击中点
        LineColor,
        false,        // 不持久
        0.1f,         // 显示0.1秒
        0,            // DepthPriority
        1.0f          // 线宽
    );
	if (BottomHit.bBlockingHit)
    {
        DrawDebugSphere(
            GetWorld(),
            BottomHit.Location,
            10.0f,        // 半径
            16,           // 分段数
            FColor::Green,
            false,        // 不持久
            0.1f          // 显示0.1秒
        );
    }
}

void UWallDetectionComponent::TopTrace()
{
	if (!CharacterOwner) return;

	FVector Upset = FVector(0.f, 0.f, 100.f);

	FVector Start = CharacterOwner->GetActorLocation() + Upset;
	FVector End = Start + CharacterOwner->GetActorForwardVector() * LineTraceLength;

	if (GetWorld())
	{
		GetWorld()->LineTraceSingleByObjectType(
			TopHit,
			Start,
			End,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),//检测静态物体
			FCollisionQueryParams(NAME_None, false, CharacterOwner) // 忽略自身
		);
	}

	FColor LineColor = TopHit.bBlockingHit ? FColor::Yellow : FColor::Blue;
	DrawDebugLine(
    GetWorld(),
    Start,
    TopHit.bBlockingHit ? TopHit.Location : End,  // 命中点画到击中点
    LineColor,
    false,        // 不持久
    0.1f,         // 显示0.1秒
    0,            // DepthPriority
    1.0f          // 线宽
	);
	if (TopHit.bBlockingHit)
	{
	    DrawDebugSphere(
	        GetWorld(),
	        TopHit.Location,
	        10.0f,        // 半径
	        16,           // 分段数
	        FColor::Green,
	        false,        // 不持久
	        0.1f          // 显示0.1秒
	    );
	}
}

void UWallDetectionComponent::MiddleTrace()
{
	if (!CharacterOwner) return;

	FVector Start = CharacterOwner->GetActorLocation();
	FVector End = Start + CharacterOwner->GetActorForwardVector() * LineTraceLength;

	if (GetWorld())
	{
		GetWorld()->LineTraceSingleByObjectType(
			MiddleHit,
			Start,
			End,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),//检测静态物体
			FCollisionQueryParams(NAME_None, false, CharacterOwner) // 忽略自身
		);
		WallNormal = MiddleHit.ImpactNormal;
		ActorToWallDistance = FVector::Distance(MiddleHit.Location, CharacterOwner->GetActorLocation());
		MiddlePointLocation = MiddleHit.Location;
	}

	FColor LineColor = MiddleHit.bBlockingHit ? FColor::Purple : FColor::Blue;
	DrawDebugLine(
	GetWorld(),
	Start,
	MiddleHit.bBlockingHit ? MiddleHit.Location : End,  // 命中点画到击中点
	LineColor,
	false,        // 不持久
	0.1f,         // 显示0.1秒
	0,            // DepthPriority
	1.0f          // 线宽
	);
	if (MiddleHit.bBlockingHit)
	{
	    DrawDebugSphere(
	        GetWorld(),
	        MiddleHit.Location,
	        10.0f,        // 半径
	        16,           // 分段数
	        FColor::Green,
	        false,        // 不持久
	        0.1f          // 显示0.1秒
	    );
	}
}

void UWallDetectionComponent::EvaluateResults()
{
	if (!CharacterOwner) return;

	FVector ForwardVector = CharacterOwner->GetActorForwardVector();
    FVector BottomNormal = BottomHit.ImpactNormal;
    FVector MiddleNormal = MiddleHit.ImpactNormal;
    FVector TopNormal = TopHit.ImpactNormal;

	float BottomDot = FVector::DotProduct(ForwardVector, BottomNormal);
	float MiddleDot = FVector::DotProduct(ForwardVector, MiddleNormal);
	float TopDot = FVector::DotProduct(ForwardVector, TopNormal);

	if (BottomDot > -0.7f || TopDot > -0.7f || MiddleDot > -0.7f)
    {
        DetectionResult = EDetectionResult::Invalid;
        return;
    }

	if (FVector::DotProduct(BottomNormal, MiddleNormal) < 0.8f || FVector::DotProduct(MiddleNormal, TopNormal) < 0.8f)
	{
		DetectionResult = EDetectionResult::Invalid;
        return;
	}

	float HeightDifference = FMath::Abs(TopHit.Location.Z - BottomHit.Location.Z);
	if (HeightDifference < 150.0f)
    {
        DetectionResult = EDetectionResult::Invalid;
        return;
    }

	DetectionResult = EDetectionResult::WallRunPossible;
	bCanWallRun = true;
}

void UWallDetectionComponent::CapsuleTrace(const FVector& Direction, FHitResult& OutHitResult, float CapsuleRadius, float CapsuleHalf, float TraceLength, FColor DebugColor)
{
	if (!CharacterOwner || !GetWorld()) return;

	FVector Start = CharacterOwner->GetActorLocation();
	FVector End = Start + Direction * TraceLength;

	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalf);
	FCollisionQueryParams QueryParams(NAME_None, false, CharacterOwner);

	// 执行检测
	GetWorld()->SweepSingleByChannel(
		OutHitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_WorldStatic,
		CapsuleShape,
		QueryParams
	);

	// 统一调试绘制
	DrawDebugLine(
		GetWorld(),
		Start,
		OutHitResult.bBlockingHit ? OutHitResult.Location : End,
		DebugColor,
		false,
		0.1f,
		0,
		1.0f
	);

	if (OutHitResult.bBlockingHit)
	{
		// 绘制命中点
		DrawDebugSphere(
			GetWorld(),
			OutHitResult.Location,
			10.0f,
			16,
			FColor::Green,
			false,
			0.1f
		);

		// 绘制胶囊体轮廓
		DrawDebugCapsule(
			GetWorld(),
			OutHitResult.Location,
			CapsuleHalf,
			CapsuleRadius,
			FQuat::Identity,
			DebugColor,
			false,
			0.1f
		);
	}
}

void UWallDetectionComponent::RightCapsuleTrace()
{
	CapsuleTrace(
		CharacterOwner->GetActorRightVector(),
		RightCapsuleHit,
		CapsuleTraceRadius,
		CapsuleHalfHeight,
		CapsuleTraceLength,
		FColor::Emerald  // 右侧用翠绿色
	);
}

void UWallDetectionComponent::LeftCapsuleTrace()
{
	CapsuleTrace(
		-CharacterOwner->GetActorRightVector(),  // 取反即可
		LeftCapsuleHit,
		CapsuleTraceRadius,
		CapsuleHalfHeight,
		CapsuleTraceLength,
		FColor::Cyan  // 左侧用青色区分
	);
}
