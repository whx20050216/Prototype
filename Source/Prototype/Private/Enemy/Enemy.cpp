// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "Character/AlexCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "Items/Projectile.h"
#include "AI/EnemyAIController.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SightRadius = 2000.f;		//2000.f为视野半径
	PawnSensing->SetPeripheralVisionAngle(60.f);	// 100度总视野

	// 指定AIController类
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 只在远程攻击状态下更新Pitch（优化）
    if (bIsAttacking && AttackConfigs[CurrentAttackIndex].Type != EAttackType::Melee)
    {
        UpdateAimingData();
    }
	else
	{
		CurrentAimPitch = FMath::FInterpTo(CurrentAimPitch, 0.f, DeltaTime, 5.f);
	}
}

void AEnemy::ExecuteAttack()
{
	if (AttackConfigs.Num() == 0) return;

	// 立即锁定攻击状态，防止重复触发
	bIsAttacking = true;
    if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
    {
        AIController->SetIsAttacking(true);
    }

	const FAttackConfig& CurrentAttack = AttackConfigs[CurrentAttackIndex];

	switch (CurrentAttack.Type)
	{
		case EAttackType::Melee:
			ExecuteMeleeAttack(CurrentAttack);
			break;
		case EAttackType::Projectile:
			ExecuteProjectileAttack(CurrentAttack);
			break;
		case EAttackType::Raycast:
			ExecuteRaycastAttack(CurrentAttack);
			break;
	}

	if (CurrentAttack.AttackCooldown > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
		AttackCooldownTimer,
		this,
		&AEnemy::OnAttackCooldownEnd,
		CurrentAttack.AttackCooldown,
		false
		);
	}
}

bool AEnemy::CanPerformAttack() const
{
	return !GetWorld()->GetTimerManager().IsTimerActive(AttackCooldownTimer);
}

void AEnemy::CancelAttack()
{
	StopAllMontages();
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Cancel Attack")));
	if (bIsAttacking)
	{
		bIsAttacking = false;
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimer);
		if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
        {
            AIController->SetIsAttacking(false);
            AIController->ClearFocus(EAIFocusPriority::Gameplay);
        }
	}
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 绑定感知回调
	if (PawnSensing)
	{
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::OnSeePlayer);
	}

	// 绑定动画结束事件
	OnMontageEnded.AddDynamic(this, &AEnemy::OnAnimMontageEnded);
}

void AEnemy::OnSeePlayer(APawn* Pawn)
{
	if (AAlexCharacter* Player = Cast<AAlexCharacter>(Pawn))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Find Player")));

		if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
		{
			// 调用Controller的Set函数
			if (!AIController->HasTargetPlayer())
			{
				AIController->SetTargetPlayer(Player);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Set TargetPlayer")));
			}
			
			// 启动持续检测计时器
			GetWorld()->GetTimerManager().SetTimer(
				PlayerVisibilityTimer,
				this,
				&AEnemy::CheckPlayerVisibility,
				PlayerVisibilityCheckInterval,
				true  // 循环执行
			);

			const FAttackConfig& Config = AttackConfigs[CurrentAttackIndex];
			float AttackRange = (Config.Type == EAttackType::Melee) ? Config.MeleeRange : Config.MaxRange;

			// 通知Controller做决策（移动）
			if (GetDistanceToPlayer() > AttackRange)
			{
				if (IsPlayingMontage(Config.Animation.Montage, Config.Animation.SectionName))
				{
					if (Config.Type == EAttackType::Melee)
					{
						return;
					}
					else
					{
						StopAllMontages();
						AIController->MoveToTargetPlayer(Config.AcceptanceRadius);
					}
				}
				else
				{
					StopAllMontages();
					AIController->MoveToTargetPlayer(Config.AcceptanceRadius);
				}
			}
		}
	}
}

/*
* UObject 生命周期函数：对象从磁盘加载完成后由引擎自动调用
* 强制使用 AEnemyAIController，防止蓝图配置错误（会覆盖蓝图设置）
*/
void AEnemy::PostLoad()
{
	Super::PostLoad();

	if (AIControllerClass != AEnemyAIController::StaticClass())
	{
		AIControllerClass = AEnemyAIController::StaticClass();
	}
}

void AEnemy::OnAnimMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsAttacking = false;
	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		AIController->SetIsAttacking(false);
		AIController->ClearFocus(EAIFocusPriority::Gameplay);// 清除焦点
	}
}

void AEnemy::UpdateAimingData()
{
	if (AActor* Player = GetPlayerActor())
	{
		const FVector BaseOffset(0, 0, 50.f);
		FVector ShoulderLocation = GetActorLocation() + BaseOffset;		//子弹生成的高度位置
		FVector TargetLocation = Player->GetActorLocation() + BaseOffset;	//瞄准胸口

		FVector ToTarget = (TargetLocation - ShoulderLocation).GetSafeNormal();
		float Dist2D = FVector::Dist2D(ToTarget, FVector::ZeroVector);

		float IdealPitch = FMath::Atan2(ToTarget.Z, Dist2D) * (180.f / PI);

		CurrentAimPitch = FMath::Clamp(IdealPitch, -50.f, 50.f);

		FRotator AimRot(CurrentAimPitch, GetActorRotation().Yaw, 0.f);
		FVector AimDir = AimRot.Vector();

		CachedMuzzleLocation = ShoulderLocation + (AimDir * 150.f);
		CachedMuzzleRotation = AimRot;
	}
}

AActor* AEnemy::GetPlayerActor() const
{
	return UGameplayStatics::GetPlayerCharacter(this, 0);
}

void AEnemy::CheckPlayerVisibility()
{
	if (!PawnSensing) return;

	// 从黑板获取当前目标
	UObject* Target = BlackboardComp->GetValueAsObject("TargetPlayer");
	if (!Target)
	{
		// 没有目标，停止检测
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
		return;
	}

	APawn* PlayerPawn = Cast<APawn>(Target);
	if (!PawnSensing->CouldSeePawn(PlayerPawn))
	{
		if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
		{
			AIController->OnTargetLost();
		}
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
	}
	// 如果还能看见，计时器会继续运行，下次再检查
}

float AEnemy::GetDistanceToPlayer() const
{
	if (AActor* Player = GetPlayerActor())
	{
		return FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	}
	return FLT_MAX;
}

void AEnemy::SetAttackType(EAttackType Type)
{
	for (int32 i = 0; i < AttackConfigs.Num(); i++)
    {
        if (AttackConfigs[i].Type == Type)
        {
            CurrentAttackIndex = i;
            return;
        }
    }
    // 如果没找到对应配置，默认使用第一个
    CurrentAttackIndex = 0;
}

float AEnemy::GetCurrentAimPitch() const
{
	return CurrentAimPitch;
}

void AEnemy::ExecuteMeleeAttack(const FAttackConfig& AttackConfig)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Execute Melee Attack")));
	AnimationConfig = AttackConfig.Animation;
	PlayAnimationWithSections(AttackConfig.Animation.Montage, AttackConfig.Animation.SectionSequence, AttackConfig.Animation.PlayRate);
}

void AEnemy::ExecuteProjectileAttack(const FAttackConfig& AttackConfig)
{	
	if (!AttackConfig.ProjectileClass) return;

	if (AActor* Player = GetPlayerActor())
	{
		////发射点偏移，避免从地面射出
		//FVector MuzzleLocation = GetActorLocation() + GetActorForwardVector() * 150.f + FVector(0, 0, 50);
		////计算从枪口到玩家的旋转角，让子弹"瞄准"玩家
		//FVector TargetLocation = Player->GetActorLocation() + FVector(0, 0, 50.f);
		//FRotator MuzzleRotation = (TargetLocation - MuzzleLocation).Rotation();

		/*
		* AdjustIfPossibleButAlwaysSpawn	尝试微调位置避开碰撞，但必定生成（可能卡在墙里）
		* DontSpawnIfColliding	如果位置被占，直接不生成
		* SpawnEvenIfColliding	不管有没有碰撞，硬生生成（会瞬移或卡住）
		* SpawnIfPossible	能避开就生成，避不开就不生成
		*/
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Instigator = this;
		SpawnParams.Owner = this;

		AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
			AttackConfig.ProjectileClass,
			CachedMuzzleLocation,
			CachedMuzzleRotation,
			SpawnParams
		);

		PlayAnimation(AttackConfig.Animation.Montage, AttackConfig.Animation.SectionName, AttackConfig.Animation.PlayRate);

		if (Projectile)
		{
			Projectile->Damage = AttackConfig.Damage;
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Spawn Projectile,Damage=%.0f"), AttackConfig.Damage));
		}
	}
}

void AEnemy::ExecuteRaycastAttack(const FAttackConfig& AttackConfig)
{
	if (AActor* Player = GetPlayerActor())
	{
		FVector Start = GetActorLocation() + FVector(0, 0, 50);
		FVector ToPlayer = (Player->GetActorLocation() - Start).GetSafeNormal();
		FVector End = Start + ToPlayer * AttackConfig.MaxRange;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FHitResult HitResult;
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility,
			QueryParams
		);

		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f, 0, 2.0f);

		if (bHit && HitResult.GetActor() == Player)
		{
			UGameplayStatics::ApplyDamage(
				Player,
				AttackConfig.Damage,
				GetController(),
				this,
				UDamageType::StaticClass()
			);
			DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Green, false, 2.0f);
		}
	}
}

void AEnemy::OnAttackHit()
{
	if (!bIsAttacking || AttackConfigs[CurrentAttackIndex].Type != EAttackType::Melee) return;

	const FAttackConfig& Config = AttackConfigs[CurrentAttackIndex];
	// 检测起点：敌人位置 + 高度偏移
	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	// 球形检测（起点=终点=Start，形成原地球形）
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Config.MeleeRadius);
	TArray<FHitResult> HitResults;
	// 检测玩家（Pawn）通道
	bool bHit = GetWorld()->SweepMultiByChannel(HitResults, Start, Start, FQuat::Identity, ECC_Pawn, Sphere);
	// 调试可视化（开发时开启）
    DrawDebugSphere(GetWorld(), Start, Config.MeleeRadius, 12, FColor::Red, false, 1.f);

	if (bHit)
	{
		for (auto& Hit : HitResults)
		{
			if (AAlexCharacter* Player = Cast<AAlexCharacter>(Hit.GetActor()))
			{
				// 扇形角度检测
				FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
				FVector Forward = GetActorForwardVector();
				float AngleDegrees = FMath::Acos(FVector::DotProduct(Forward, ToPlayer)) * (180.f / PI);

				// 在攻击角度内则造成伤害
				if (AngleDegrees <= Config.AttackAngle / 2.f)
				{
					UGameplayStatics::ApplyDamage(Player, Config.Damage, GetController(), this, UDamageType::StaticClass());
                    break; // 命中一个即退出
				}
			}
		}
	}
}

void AEnemy::OnAttackCooldownEnd()
{
}
