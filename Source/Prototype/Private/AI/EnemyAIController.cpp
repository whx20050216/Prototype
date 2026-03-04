// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemyAIController.h"
#include "Enemy/Enemy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

AEnemyAIController::AEnemyAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 获取控制的敌人  
    AEnemy* ControlledEnemy = Cast<AEnemy>(InPawn);
	if (ControlledEnemy)
    {
        // 获取敌人的BlackboardComp引用
        if (UBlackboardComponent* BB = GetBlackboardComponent())
        {
            ControlledEnemy->BlackboardComp = BB;
            
            // 如果敌人有行为树，立即运行
            if (ControlledEnemy->BehaviorTree)
            {
                // 确保黑板与行为树匹配
                BB->InitializeBlackboard(*ControlledEnemy->BehaviorTree->BlackboardAsset);
                
                // 运行行为树
                RunBehaviorTree(ControlledEnemy->BehaviorTree);
                
                // 设置初始值（可选，但推荐）
                BB->SetValueAsObject("SelfActor", ControlledEnemy);
            }
        }
    }
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AEnemyAIController::MoveToTargetPlayer(float AcceptanceRadius)
{
    if (!BlackboardComp) return;

    UObject* Target = BlackboardComp->GetValueAsObject("TargetPlayer");
    if (!Target) return;

	// Controller执行决策：调用UE内置导航
    AActor* TargetActor = Cast<AActor>(Target);
    if (TargetActor)
    {
        MoveToActor(TargetActor, AcceptanceRadius);
    }
}

void AEnemyAIController::StopMovingToPlayer()
{
	// Controller决策：停止所有移动
    StopMovement();
    ClearTargetPlayer();
}

void AEnemyAIController::OnTargetLost()
{
    if (AEnemy* Enemy = Cast<AEnemy>(GetPawn()))
    {
        Enemy->CancelAttack();
    }

    StopMovingToPlayer();
    SetIsAttacking(false);
}

void AEnemyAIController::SetTargetPlayer(AActor* Player)
{
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsObject("TargetPlayer", Player);
    }
}

void AEnemyAIController::ClearTargetPlayer()
{
    if (BlackboardComp)
    {
        BlackboardComp->ClearValue("TargetPlayer");
    }
}

void AEnemyAIController::SetNoiseLocation(FVector Location)
{
    if (BlackboardComp)
    {
		BlackboardComp->SetValueAsVector("NoiseLocation", Location);
    }
}

void AEnemyAIController::ClearNoiseLocation()
{
    if (BlackboardComp)
    {
        BlackboardComp->ClearValue("NoiseLocation");
    }
}

void AEnemyAIController::SetIsAttacking(bool bAttacking)
{
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsBool("IsAttacking", bAttacking);
    }
}

bool AEnemyAIController::GetIsAttacking() const
{
    if (BlackboardComp)
    {
        return BlackboardComp->GetValueAsBool("IsAttacking");
    }
    return false;
}

void AEnemyAIController::SetAIState(EAIState NewState)
{
    if (!BlackboardComp) return;

    EAIState CurrentState = static_cast<EAIState>(BlackboardComp->GetValueAsEnum("AIState"));
    if (CurrentState == NewState) return;

    BlackboardComp->SetValueAsEnum("AIState", (uint8)NewState);
    if (NewState == EAIState::Alert)
    {
        StopMovement();
    }
}

bool AEnemyAIController::HasTargetPlayer() const
{
    if (!BlackboardComp) return false;
    return BlackboardComp->GetValueAsObject("TargetPlayer") != nullptr;
}

AActor* AEnemyAIController::GetTargetPlayer() const
{
    if (!BlackboardComp) return nullptr;
    return Cast<AActor>(BlackboardComp->GetValueAsObject("TargetPlayer"));
}

void AEnemyAIController::PerformAttackOnPlayer()
{
    if (!BlackboardComp) return;

    AEnemy* Enemy = Cast<AEnemy>(GetPawn());
    if (!Enemy) return;

    // 决策1：是否有目标
    AActor* Target = GetTargetPlayer();
    if (!Target) return;

    // 决策2：检查距离
    float Distance = Enemy->GetDistanceToPlayer();
    const FAttackConfig& CurrentAttack = Enemy->AttackConfigs[Enemy->CurrentAttackIndex];
	float AttackRange = (CurrentAttack.Type == EAttackType::Melee) 
        ? CurrentAttack.MeleeRange 
        : CurrentAttack.MaxRange;
    if (Distance > AttackRange) return;

    // 决策3：检查冷却
    if (!Enemy->CanPerformAttack()) return;

    // 设置焦点：让AI面朝目标
    SetFocus(Target);

    // 决策4：所有条件满足，攻击
    Enemy->ExecuteAttack();
}