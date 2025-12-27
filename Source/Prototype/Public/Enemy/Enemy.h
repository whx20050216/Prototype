// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "Enemy.generated.h"

class UPawnSensingComponent;
class UAnimMontage;
class AProjectile;
class UBlackboardComponent;
class UBehaviorTree;

// 攻击类型枚举
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee		UMETA(DisplayName = "近战攻击"),
	Projectile	UMETA(DisplayName = "弹道攻击"),
	Raycast		UMETA(DisplayName = "射线攻击")
};

// 攻击数据配置结构体
USTRUCT(BlueprintType)
struct FAttackData
{
	GENERATED_BODY()

	// 基础伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float Damage = 10.0f;

	// 攻击类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	EAttackType Type = EAttackType::Melee;

	// 最大射程（远程攻击用）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float MaxRange = 150.0f;

	// 攻击动画
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	UAnimMontage* AttackAnim = nullptr;

	// 子弹类（仅弹道攻击用）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(EditCondition="Type == EAttackType::Projectile"))
	TSubclassOf<AProjectile> ProjectileClass = nullptr;

	// 标签（扩展用，可留空）
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	FGameplayTagContainer AttackTags;*/
};

UCLASS()
class PROTOTYPE_API AEnemy : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AEnemy();

	// 暴露给行为树的工具函数
	UFUNCTION(BlueprintCallable, Category="AI")
    void MoveTowardPlayer(float AcceptanceRadius = 150.f);

	UFUNCTION(BlueprintCallable, Category="AI")
    void PerformAttack();

	UFUNCTION(BlueprintCallable, Category="AI")
    bool CanPerformAttack() const;

	// 攻击配置（可以在蓝图中设置多组，行为树切换）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	TArray<FAttackData> AttackConfigs;  // 支持多套攻击配置

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	int32 CurrentAttackIndex = 0;  // 当前使用的攻击配置索引

	// 基础属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float AttackCooldown = 2.0f;

	// AI感知组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    UPawnSensingComponent* PawnSensing;

	// 行为树黑板（用于通知状态）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UBlackboardComponent* BlackboardComp;

	// 行为树
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    UBehaviorTree* BehaviorTree;

protected:
	virtual void BeginPlay() override;

	// 感知回调（看到玩家时触发）
	UFUNCTION()
    void OnSeePlayer(APawn* Pawn);

	// 工具函数
	UFUNCTION(BlueprintCallable, Category="AI")
	AActor* GetPlayerActor() const;

private:
	FTimerHandle PlayerVisibilityTimer;  // 检测玩家可见性的计时器
	float PlayerVisibilityCheckInterval = 0.5f;  // 每0.5秒检查一次

	void CheckPlayerVisibility();  // 检查是否还能看见玩家

	FTimerHandle AttackCooldownTimer;  // 冷却计时器
    bool bIsAttacking = false;         // 是否在攻击中

	// 距离玩家的距离
	float GetDistanceToPlayer() const;

	// 三种攻击的执行函数
	void ExecuteMeleeAttack(const FAttackData& Data);
	void ExecuteProjectileAttack(const FAttackData& Data);
	void ExecuteRaycastAttack(const FAttackData& Data);

	// 动画通知回调（需在动画中绑定）
	UFUNCTION(BlueprintCallable, Category="Combat")
	void OnAttackHit();

	// 冷却结束回调
	void OnAttackCooldownEnd();
};
