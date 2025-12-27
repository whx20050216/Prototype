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

				UObject* InitialTarget = BB->GetValueAsObject("TargetPlayer");
				if (InitialTarget)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("TargetPlayer is not null")));
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TargetPlayer is null")));
				}
                
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
