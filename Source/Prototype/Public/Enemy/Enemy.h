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
	Melee		UMETA(DisplayName = "melee attack"),
	Projectile	UMETA(DisplayName = "Ballistic attack"),
	Raycast		UMETA(DisplayName = "Ray attack")
};

USTRUCT(BlueprintType)
struct FAttackConfig
{
	GENERATED_BODY()

	// 动画配置（使用BaseCharacter的通用结构）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Animation")
	FCharacterAnimation Animation;

	// 基础伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float Damage = 10.0f;

	// 攻击间隔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta = (ToolTip = "Seconds before next attack. <=0 = use animation length instead"))
	float AttackCooldown = 2.0f;

	// 攻击类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	EAttackType Type = EAttackType::Melee;

	// 最大射程（远程攻击用）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta = (ToolTip = "For Projectile or Raycast Enemy", EditCondition = "Type == EAttackType::Projectile || Type == EAttackType::Raycast"))
	float MaxRange = 150.0f;

	// 与目标的可接受距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float AcceptanceRadius = 150.0f;

	// 子弹类（仅弹道攻击用）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(EditCondition="Type == EAttackType::Projectile"))
	TSubclassOf<AProjectile> ProjectileClass = nullptr;

	UPROPERTY(EditAnywhere, Category="Combat|Melee", meta=(EditCondition="Type == EAttackType::Melee"))
    float MeleeRadius = 100.f;  // 检测半径
    
    UPROPERTY(EditAnywhere, Category="Combat|Melee", meta=(EditCondition="Type == EAttackType::Melee"))
    float MeleeRange = 150.f;   // 攻击距离
    
    UPROPERTY(EditAnywhere, Category="Combat|Melee", meta=(EditCondition="Type == EAttackType::Melee"))
    float AttackAngle = 90.f;   // 扇形角度

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
    void ExecuteAttack();

	UFUNCTION(BlueprintCallable, Category="AI")
    bool CanPerformAttack() const;

	// 攻击配置（可以在蓝图中设置多组，行为树切换）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	TArray<FAttackConfig> AttackConfigs;  // 配置攻击

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	int32 CurrentAttackIndex = 0;  // 当前使用的攻击配置索引

	// AI感知组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    UPawnSensingComponent* PawnSensing;

	// 行为树黑板（用于通知状态）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UBlackboardComponent* BlackboardComp;

	// 行为树
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    UBehaviorTree* BehaviorTree;

	// 工具函数
	UFUNCTION(BlueprintCallable, Category="AI")
	AActor* GetPlayerActor() const;

	// 距离玩家的距离
	UFUNCTION(BlueprintCallable, Category="AI")
	float GetDistanceToPlayer() const;

	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetAttackType(EAttackType Type);

protected:
	virtual void BeginPlay() override;

	// 感知回调（看到玩家时触发）
	UFUNCTION()
    void OnSeePlayer(APawn* Pawn);

	// 同步AIController
	virtual void PostLoad() override;

	// 动画结束回调函数
	UFUNCTION()
	void OnAnimMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
private:
	FTimerHandle PlayerVisibilityTimer;  // 检测玩家可见性的计时器
	float PlayerVisibilityCheckInterval = 0.5f;  // 每0.5秒检查一次

	void CheckPlayerVisibility();  // 检查是否还能看见玩家

	FTimerHandle AttackCooldownTimer;  // 冷却计时器

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking = false;         // 是否在攻击中

	// 三种攻击的执行函数
	void ExecuteMeleeAttack(const FAttackConfig& AttackConfig);
	void ExecuteProjectileAttack(const FAttackConfig& AttackConfig);
	void ExecuteRaycastAttack(const FAttackConfig& AttackConfig);

	// 动画通知回调（需在动画中绑定）
	UFUNCTION(BlueprintCallable, Category="Combat")
	void OnAttackHit();

	// 冷却结束回调
	void OnAttackCooldownEnd();
};
