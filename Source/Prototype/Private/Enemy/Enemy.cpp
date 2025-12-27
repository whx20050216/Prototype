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

	// 默认配置：近战攻击
	FAttackData DefaultAttack;
	DefaultAttack.Type = EAttackType::Melee;
	DefaultAttack.Damage = 10.0f;
	DefaultAttack.MaxRange = 150.0f;
	AttackConfigs.Add(DefaultAttack);

	// 指定AIController类
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemy::MoveTowardPlayer(float AcceptanceRadius)
{
	if (!BlackboardComp) return;

	UObject* Target = BlackboardComp->GetValueAsObject("TargetPlayer");
	if (!Target) return;
	// 1.获取玩家Actor引用
	if (AActor* Player = Cast<AActor>(Target))
	{
		// 2. 获取敌人的 AI 控制器
		if (AAIController* AIController = Cast<AAIController>(GetController()))
        {
			// 3. 发送移动指令
            AIController->MoveToActor(Player, AcceptanceRadius);
        }
	}
}

void AEnemy::PerformAttack()
{
	if (!CanPerformAttack()) return;

	if (AttackConfigs.Num() == 0) return;

	FAttackData CurrentAttack = AttackConfigs[CurrentAttackIndex];
	float Distance = GetDistanceToPlayer();

	if (Distance > CurrentAttack.MaxRange) return;

	bIsAttacking = true;

	if (CurrentAttack.AttackAnim)
	{
		PlayMontageSection(CurrentAttack.AttackAnim, "Attack");
	}

	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsBool("IsAttacking", true);
	}

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
}

void AEnemy::OnSeePlayer(APawn* Pawn)
{
	if (AAlexCharacter* Player = Cast<AAlexCharacter>(Pawn))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Find Player")));

		if (BlackboardComp)
		{
			BlackboardComp->SetValueAsObject("TargetPlayer", Player);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Set TargetPlayer")));

			// 启动持续检测计时器
			GetWorld()->GetTimerManager().SetTimer(
				PlayerVisibilityTimer,
				this,
				&AEnemy::CheckPlayerVisibility,
				PlayerVisibilityCheckInterval,
				true  // 循环执行
			);
		}
	}
}

AActor* AEnemy::GetPlayerActor() const
{
	return UGameplayStatics::GetPlayerCharacter(this, 0);
}

void AEnemy::CheckPlayerVisibility()
{
	if (!BlackboardComp || !PawnSensing)
	{
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
		return;
	}
	// 从黑板获取当前目标
	UObject* Target = BlackboardComp->GetValueAsObject("TargetPlayer");
	if (!Target)
	{
		// 没有目标，停止检测
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
		return;
	}

	APawn* PlayerPawn = Cast<APawn>(Target);
	if (!PlayerPawn)
	{
		BlackboardComp->ClearValue("TargetPlayer");
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
		return;
	}

	// 关键：检查是否仍然能看到玩家
	if (!PawnSensing->CouldSeePawn(PlayerPawn))
	{
		// 看不到了，清除目标
		BlackboardComp->ClearValue("TargetPlayer");
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Lost Player"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Clear TargetPlayer"));

		// 停止移动
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("停止移动"));
		}

		// 清除计时器
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

void AEnemy::ExecuteMeleeAttack(const FAttackData& Data)
{
	UE_LOG(LogTemp, Log, TEXT("近战攻击：等待 AnimNotify 触发伤害"));
}

void AEnemy::ExecuteProjectileAttack(const FAttackData& Data)
{
	if (!Data.ProjectileClass) return;

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
			Data.ProjectileClass,
			MuzzleLocation,
			MuzzleRotation,
			SpawnParams
		);

		if (Projectile)
		{
			Projectile->Damage = Data.Damage;
			UE_LOG(LogTemp, Log, TEXT("生成子弹：伤害=%.0f"), Data.Damage);
		}
	}
}

void AEnemy::ExecuteRaycastAttack(const FAttackData& Data)
{
	if (AActor* Player = GetPlayerActor())
	{
		FVector Start = GetActorLocation() + FVector(0, 0, 50);
		FVector End = Start + GetActorForwardVector() * Data.MaxRange;

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
				Data.Damage,
				GetController(),
				this,
				UDamageType::StaticClass()
			);
			UE_LOG(LogTemp, Log, TEXT("射线命中玩家，伤害=%.0f"), Data.Damage);
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
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsBool("IsAttacking", false);
		BlackboardComp->ClearValue("AttackHit");
	}
}
