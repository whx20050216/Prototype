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
	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SightRadius = 2000.f;		//2000.f为视野半径
	PawnSensing->SetPeripheralVisionAngle(50.f);	// 100度总视野

	// 使用新的攻击配置结构初始化
	FAttackConfig DefaultAttack;
	DefaultAttack.Animation.Montage = nullptr;  // 在蓝图中配置
	DefaultAttack.Animation.SectionName = "Attack";
	DefaultAttack.Animation.PlayRate = 1.0f;
	DefaultAttack.Damage = 10.0f;
	DefaultAttack.MaxRange = 150.0f;
	DefaultAttack.Type = EAttackType::Melee;
	AttackConfigs.Add(DefaultAttack);

	// 指定AIController类
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemy::ExecuteAttack()
{
	if (AttackConfigs.Num() == 0) return;

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

	GetWorld()->GetTimerManager().SetTimer(
		AttackCooldownTimer,
		this,
		&AEnemy::OnAttackCooldownEnd,
		AttackCooldown,
		false
	);
}

bool AEnemy::CanPerformAttack() const
{
	return !bIsAttacking && !GetWorld()->GetTimerManager().IsTimerActive(AttackCooldownTimer);
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
			AIController->SetTargetPlayer(Player);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Set TargetPlayer")));

			// 启动持续检测计时器
			GetWorld()->GetTimerManager().SetTimer(
				PlayerVisibilityTimer,
				this,
				&AEnemy::CheckPlayerVisibility,
				PlayerVisibilityCheckInterval,
				true  // 循环执行
			);

			// 通知Controller做决策（移动）
			AIController->MoveToTargetPlayer(150.f);
		}
	}
}

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
			AIController->ClearTargetPlayer();// Controller清除记忆
			AIController->StopMovingToPlayer();
		}
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
	}
	// 如果还能看见，计时器会继续运行，下次再检查
}

float AEnemy::GetDistanceToPlayer() const
{
	if (AActor* Player = GetPlayerActor())
	{
		return FVector::Dist(GetActorLocation(), Player->GetActorLocation());
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

void AEnemy::ExecuteMeleeAttack(const FAttackConfig& AttackConfig)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Execute Melee Attack")));
}

void AEnemy::ExecuteProjectileAttack(const FAttackConfig& AttackConfig)
{	
	if (!AttackConfig.ProjectileClass) return;

	if (AActor* Player = GetPlayerActor())
	{
		//发射点偏移，避免从地面射出
		FVector MuzzleLocation = GetActorLocation() + FVector(0, 0, 50);
		//计算从枪口到玩家的旋转角，让子弹"瞄准"玩家
		FRotator MuzzleRotation = (Player->GetActorLocation() - MuzzleLocation).Rotation();

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
			MuzzleLocation,
			MuzzleRotation,
			SpawnParams
		);

		if (Projectile)
		{
			Projectile->Damage = AttackConfig.Damage;
			UE_LOG(LogTemp, Log, TEXT("生成子弹：伤害=%.0f"), AttackConfig.Damage);
		}
	}
}

void AEnemy::ExecuteRaycastAttack(const FAttackConfig& AttackConfig)
{
	if (AActor* Player = GetPlayerActor())
	{
		FVector Start = GetActorLocation() + FVector(0, 0, 50);
		FVector End = Start + GetActorForwardVector() * AttackConfig.MaxRange;

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
			UE_LOG(LogTemp, Log, TEXT("射线命中玩家，伤害=%.0f"), AttackConfig.Damage);
			DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Green, false, 2.0f);
		}
	}
}

void AEnemy::OnAttackHit()
{
	if (AActor* Player = GetPlayerActor())
	{
		float Distance = GetDistanceToPlayer();
		if (Distance <= AttackConfigs[CurrentAttackIndex].MaxRange)
		{
			UGameplayStatics::ApplyDamage(
				Player,
				AttackConfigs[CurrentAttackIndex].Damage,
				GetController(),
				this,
				UDamageType::StaticClass()
			);
			UE_LOG(LogTemp, Log, TEXT("近战命中，伤害=%.0f"), AttackConfigs[CurrentAttackIndex].Damage);

			if (BlackboardComp)
			{
				BlackboardComp->SetValueAsBool("AttackHit", true);
			}
		}
	}
}

void AEnemy::OnAttackCooldownEnd()
{
	bIsAttacking = false;
	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		AIController->SetIsAttacking(false);
	}
}
