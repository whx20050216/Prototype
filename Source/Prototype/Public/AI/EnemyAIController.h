// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

enum class EAIState : uint8;

UCLASS()
class PROTOTYPE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AEnemyAIController();

	// 移动决策函数
	UFUNCTION(BlueprintCallable, Category="AI")
	void MoveToTargetPlayer(float AcceptanceRadius = 150.0f);

	// 停止移动
	UFUNCTION(BlueprintCallable, Category="AI")
	void StopMovingToPlayer();

	// 目标丢失
	UFUNCTION(BlueprintCallable, Category="AI")
	void OnTargetLost();

	// 黑板管理函数
	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	void SetTargetPlayer(AActor* Player);

	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	void ClearTargetPlayer();

	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	void SetNoiseLocation(FVector Location);

	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	void ClearNoiseLocation();

	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	void SetIsAttacking(bool bAttacking);

	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	bool GetIsAttacking() const;

	UFUNCTION(BlueprintCallable, Category="AI")
	void SetAIState(EAIState NewState);

	// 黑板查询函数
	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	bool HasTargetPlayer() const;

	UFUNCTION(BlueprintCallable, Category="AI|Blackboard")
	AActor* GetTargetPlayer() const;

	// 攻击决策函数
	UFUNCTION(BlueprintCallable, Category="AI")
	void PerformAttackOnPlayer();


protected:
	virtual void OnPossess(APawn* InPawn) override;
    virtual void BeginPlay() override;

	// 黑板组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UBlackboardComponent* BlackboardComp;
};
