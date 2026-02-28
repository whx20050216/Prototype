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
#include "ActorComponent/AttributeComponent.h"
#include "HUD/HealthWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/OverlapResult.h"
#include "Items/WeaponActor.h"
#include "BrainComponent.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SightRadius = 3000.f;		//3000.f为视野半径
	PawnSensing->SetPeripheralVisionAngle(60.f);	// 100度总视野

	// 指定AIController类
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	AttributeComp = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComp"));

	HealthBarComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComp"));
	HealthBarComp->SetupAttachment(GetRootComponent());
    HealthBarComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));  // 头顶120cm
    HealthBarComp->SetWidgetSpace(EWidgetSpace::World);
	HealthBarComp->SetDrawSize(FVector2D(100.f, 10.f));            // UI大小（像素）
    HealthBarComp->SetPivot(FVector2D(0.5f, 0.5f));
	HealthBarComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (BlackboardComp)
    {
        BlackboardComp->SetValueAsFloat("DistanceToPlayer", GetDistanceToPlayer());
    }

	if (CurrentAIState != EAIState::Dead)
	{
		UpdateSuspicion(DeltaTime);
	}

	if (AttackConfigs.IsValidIndex(CurrentAttackIndex) && 
        AttackConfigs[CurrentAttackIndex].Type != EAttackType::Melee)
    {
        if (CurrentAIState == EAIState::Alert && 
            GetDistanceToPlayer() <= GetAttackRange())
        {
            // 在范围内：进入/保持 Fire 动画
            bIsAiming = true;
            UpdateAimingData();
        }
        else
        {
            // 超出范围（追击中）：退出 Fire 动画，回到 WalkRun
            bIsAiming = false;
            CurrentAimPitch = FMath::FInterpTo(CurrentAimPitch, 0.f, DeltaTime, 5.f);
        }
    }
}

void AEnemy::ExecuteAttack()
{
	if (AttackConfigs.Num() == 0) return;
	if (bIsAttacking) return;

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
		bIsAiming = false;
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

	if (HealthWidgetClass)  // 检查蓝图是否指定了类
	{
		HealthBarComp->SetWidgetClass(HealthWidgetClass);
        HealthBarComp->InitWidget();

		HealthWidget = Cast<UHealthWidget>(HealthBarComp->GetWidget());
	    if (HealthWidget)
	    {   
	        if (AttributeComp)
	        {
				AttributeComp->OnHealthChanged.AddDynamic(this, &AEnemy::HandleHealthChanged);
				AttributeComp->OnDeath.AddDynamic(this, &AEnemy::OnHealthDepleted);

	            HealthWidget->BindToAttributeComponent(AttributeComp);
	        }
	    }
	}

	HideHealthBar();

	// 生成武器
	if (AttackConfigs.IsValidIndex(CurrentAttackIndex))
	{
		if (!AttackConfigs[CurrentAttackIndex].WeaponClass) return;

		FActorSpawnParameters Params;
		Params.Owner = this;
		Weapon = GetWorld()->SpawnActor<AWeaponActor>(AttackConfigs[CurrentAttackIndex].WeaponClass, Params);
		if (Weapon)
		{
			FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
			Weapon->AttachToComponent(GetMesh(), AttachRules, FName("hand_r"));
			Weapon->SetInstigator(this);
		}
	}
}

void AEnemy::UpdateSuspicion(float DeltaTime)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
	if (!AIController) return;

	// 如果是"狂暴/感染体"类型，看到玩家直接 Alert，不累积黄条
    if (bSkipSuspicion && bCanSeePlayer && CurrentAIState != EAIState::Alert)
    {
        EnterAlertState();  // 直接满条进入战斗
        return;  // 跳过后面的累积逻辑
    }

	if (bCanSeePlayer && CurrentAIState != EAIState::Alert)
	{
		// 关键：注册自己
        if (AAlexCharacter* Player = Cast<AAlexCharacter>(GetPlayerActor()))
        {
            Player->RegisterSuspicionSource(this);
        }

		// 看到玩家，累积警觉条
		CurrentSuspicion += DeltaTime * SuspicionIncreaseRate;

		// 进入Suspicious状态
		if (CurrentAIState == EAIState::Idle)
		{
			CurrentAIState = EAIState::Suspicious;

			// 同步到Blackboard（BT用这个判断行为）
			AIController->SetAIState(EAIState::Suspicious);
			// 面向玩家但不移动
            if (AActor* Player = GetPlayerActor())
            {
                AIController->SetTargetPlayer(Player); // 设置目标用于面向，但BT不追击
            }
		}

		// 警觉条满，进入Alert
		if (CurrentSuspicion >= SuspicionThreshold)
		{
			EnterAlertState();
		}
	}
	else if (!bCanSeePlayer && CurrentAIState == EAIState::Suspicious)
	{
		// 丢失视野，衰减
		CurrentSuspicion -= DeltaTime * SuspicionDecayRate;
		if (CurrentSuspicion <= 0.f)
		{
			bIsAiming = false;
			CurrentSuspicion = 0.f;
			CurrentAIState = EAIState::Idle;
            if (AAlexCharacter* Player = Cast<AAlexCharacter>(GetPlayerActor()))
            {
                Player->UnregisterSuspicionSource(this);
            }
			AIController->SetAIState(EAIState::Idle);
			AIController->ClearTargetPlayer();
		}
	}

	// 同步 Suspicion 值到 Blackboard（用于UI显示）
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsFloat("SuspicionLevel", CurrentSuspicion);
    }
}

void AEnemy::EnterAlertState()
{
	CurrentAIState = EAIState::Alert;
	CurrentSuspicion = SuspicionThreshold;	// 保持满值

	AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
    if (!AIController) return;

	// 1. 同步到Blackboard（BT切换到Attack分支）
	AIController->SetAIState(EAIState::Alert);
	// 2. 设置TargetPlayer（允许BT追击）
	if (AActor* Player = GetPlayerActor())
    {
        AIController->SetTargetPlayer(Player);
    }

	// 3. 广播给附近敌人（让他们进入Suspicious）
	FVector Location = GetActorLocation();
    float AlertRadius = 2000.f; // 20米
	TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(AlertRadius);
	if (GetWorld()->OverlapMultiByChannel(Overlaps, Location, FQuat::Identity, ECC_Pawn, Sphere))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AEnemy* NearbyEnemy = Cast<AEnemy>(Overlap.GetActor());
			// 排除自己、排除已 Alert、排除 Dead
            if (NearbyEnemy && 
                NearbyEnemy != this && 
                NearbyEnemy->CurrentAIState != EAIState::Alert && 
                NearbyEnemy->CurrentAIState != EAIState::Dead)
            {
                // 新增：只有同类型才互相唤醒（感染体唤醒感染体，军队唤醒军队）
				if (NearbyEnemy->bSkipSuspicion == this->bSkipSuspicion)
				{
				    NearbyEnemy->ForceEnterAlert();
				}
            }
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, 
        FString::Printf(TEXT("%s Enter Alert State And Broadcast！"), *GetName()));
	const FAttackConfig& Config = AttackConfigs[CurrentAttackIndex];
	if (Config.Type != EAttackType::Melee)
	{
		//bIsAiming = true;
		PlayAnimation(Config.Animation.Montage, Config.Animation.SectionName, Config.Animation.PlayRate);
	}
}

void AEnemy::ForceEnterAlert()
{
	// 如果已经在 Alert 或 Dead，不处理
    if (CurrentAIState == EAIState::Alert || CurrentAIState == EAIState::Dead)
        return;

	CurrentAIState = EAIState::Alert;
	CurrentSuspicion = SuspicionThreshold;  // 满条

	AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
    if (!AIController) return;

	// 同步 Blackboard
	AIController->SetAIState(EAIState::Alert);

    // 设置 TargetPlayer（面向并准备追击）
    if (AActor* Player = GetPlayerActor())
    {
        AIController->SetTargetPlayer(Player);
    }
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AttributeComp && ActualDamage > 0.f)
	{
		AttributeComp->ApplyHealthChange(-ActualDamage);
		if (!AttributeComp->IsDead())
		{
			// 1. 打断当前攻击
			CancelAttack();

			// 2. 播放受击动画（随机选一个，或根据伤害方向选）
			if (HitReactMontages.Num() > 0 && GetMesh()->GetAnimInstance())
			{
				// 检查是否已经在播放受击（防止被连击时鬼畜）
				bool bAlreadyHit = false;
				for (auto* Montage : HitReactMontages)
				{
					if (GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage))
                    {
                        bAlreadyHit = true;
                        break;
                    }
				}

				if (!bAlreadyHit)
				{
					int32 RandIndex = FMath::RandRange(0, HitReactMontages.Num() - 1);
                    UAnimMontage* HitMontage = HitReactMontages[RandIndex];
					if (HitMontage)
					{
						PlayAnimMontage(HitMontage);
						// 停止行为树
						if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
                        {
                            AIController->StopMovement();
                            if (AIController->GetBrainComponent())
                            {
                                AIController->GetBrainComponent()->PauseLogic("HitReact");
                            }
                        }

						// 动画播完恢复行为树
						FTimerHandle ResumeHandle;
                        GetWorldTimerManager().SetTimer(
                            ResumeHandle,
                            [this]()
                            {
								if (!IsValid(this)) return;
								if (CurrentAIState == EAIState::Dead) return;

                                if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
                                {
                                    if (AIController->GetBrainComponent())
                                    {
                                        AIController->GetBrainComponent()->ResumeLogic("HitReact");
                                    }
                                }
                            },
                            HitStunDuration,
                            false
                        );
					}
				}
			}
		}
		if (CurrentAIState != EAIState::Alert && CurrentAIState != EAIState::Dead)
        {
            // 立即转向攻击者方向（给玩家明显的反馈）
            if (DamageCauser)
            {
                FVector ToAttacker = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                FRotator LookAtRot = ToAttacker.Rotation();
                LookAtRot.Pitch = 0.f; // 保持水平，不仰头低头
                SetActorRotation(LookAtRot);
            }
            
            // 进入 Alert（这会自动设置 TargetPlayer、广播给附近敌人、切换行为树）
            EnterAlertState();
        }
	}
	return ActualDamage;
}

void AEnemy::ShowHealthBar()
{
	if (HealthBarComp)
	{
		HealthBarComp->SetVisibility(true);
	}
}

void AEnemy::HideHealthBar()
{
	if (HealthBarComp)
	{
		HealthBarComp->SetVisibility(false);
	}
}

void AEnemy::HandleHealthChanged(float CurrentHealth, float MaxHealth, float Delta)
{
	// 只有受伤（Delta < 0）且未死亡时才显示
	if (Delta < 0 && CurrentHealth>0)
	{
		ShowHealthBar();

		// 重置脱战计时：清除旧的，启动新的5秒定时器
        GetWorldTimerManager().ClearTimer(HideHealthBarTimer);
        GetWorldTimerManager().SetTimer(
            HideHealthBarTimer,
            this,
            &AEnemy::HideHealthBar,
            HealthBarHideDelay,
            false  // 单次执行，不循环
        );
	}
}

void AEnemy::OnHealthDepleted()
{
	// 死亡时注销（防止尸体还在列表里）
    if (AAlexCharacter* Player = Cast<AAlexCharacter>(GetPlayerActor()))
    {
        Player->UnregisterSuspicionSource(this);
		if (Player->GetLockedTarget() == this)
        {
            Player->CancelLockOn();
        }
    }

	if (Weapon)
	{
		Weapon->Drop(GetActorLocation() + GetActorForwardVector() * 100.f);
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);
	// 标记死亡状态
    CurrentAIState = EAIState::Dead;
	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		AIController->SetAIState(EAIState::Dead);
	}

	GetWorldTimerManager().ClearTimer(HideHealthBarTimer);
	HideHealthBar();

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
		SetLifeSpan(3.0f);
	}
	else
	{
		SetLifeSpan(0.1f);
	}
}

void AEnemy::OnSeePlayer(APawn* Pawn)
{
	if (AAlexCharacter* Player = Cast<AAlexCharacter>(Pawn))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Find Player")));
		// 只是标记看到玩家，具体行为由 UpdateSuspicion 驱动
		bCanSeePlayer = true;

		// 如果已经在Alert状态，确保TargetPlayer已设置（防止丢失后重新看到）
		if (CurrentAIState == EAIState::Alert)
		{
			if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
			{
				if (!AIController->HasTargetPlayer())
				{
					AIController->SetTargetPlayer(Player);
				}
			}
		}

		// 启动持续检测（原有逻辑，用于检测丢失）
		if (!GetWorld()->GetTimerManager().IsTimerActive(PlayerVisibilityTimer))
		{
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
	UObject* Target = BlackboardComp ? BlackboardComp->GetValueAsObject("TargetPlayer") : nullptr;
	if (!Target)
	{
		// 没有目标，停止检测
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
		return;
	}

	APawn* PlayerPawn = Cast<APawn>(Target);
	bool bStillVisible = PawnSensing->CouldSeePawn(PlayerPawn);

	if (!bStillVisible)
	{
		bCanSeePlayer = false;
		// 如果在Alert状态丢失，保持Alert一段时间（或立即降回Suspicious，这里选择立即降回）
		if (CurrentAIState == EAIState::Alert)
		{
			CurrentAIState = EAIState::Suspicious;
            CurrentSuspicion = SuspicionThreshold * 0.8f; // 保留80%开始衰减
			if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
			{
				AIController->SetAIState(EAIState::Suspicious);
			}
		}
		GetWorld()->GetTimerManager().ClearTimer(PlayerVisibilityTimer);
	}
	else
	{
		bCanSeePlayer = true;
	}
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

float AEnemy::GetAcceptanceRadius() const
{
	if (!AttackConfigs.IsValidIndex(CurrentAttackIndex))
		return 150.0f;  // 返回默认值
	
	return AttackConfigs[CurrentAttackIndex].AcceptanceRadius;
}

float AEnemy::GetAttackRange() const
{
	if (!AttackConfigs.IsValidIndex(CurrentAttackIndex))
		return 150.0f;  // 返回默认值

	if (AttackConfigs[CurrentAttackIndex].Type == EAttackType::Melee)
	{
		return AttackConfigs[CurrentAttackIndex].MeleeRange;
	}
	else
	{
		return AttackConfigs[CurrentAttackIndex].MaxRange;
	}
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
	if (!Weapon) return;

	UpdateAimingData();

	FVector MuzzleLocation = Weapon->GetMuzzlePoint()->GetComponentLocation();
	Weapon->Fire(MuzzleLocation, CachedMuzzleRotation, AttackConfig.Damage);

	//PlayAnimation(AttackConfig.Animation.Montage, AttackConfig.Animation.SectionName, AttackConfig.Animation.PlayRate);
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
	bIsAttacking = false;
	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		AIController->SetIsAttacking(false);
	}
}
