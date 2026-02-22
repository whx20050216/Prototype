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
class UHealthWidget;
class AWeaponActor;

// 攻击类型枚举
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee		UMETA(DisplayName = "melee attack"),
	Projectile	UMETA(DisplayName = "Ballistic attack"),
	Raycast		UMETA(DisplayName = "Ray attack")
};

// 行为类型枚举
UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle,		// 待机（绿）
	Suspicious,	// 警觉（黄条累积中）
	Alert,		// 通缉/战斗（红）
	Dead
};

USTRUCT(BlueprintType)
struct FAttackConfig
{
	GENERATED_BODY()

	// 动画配置（使用BaseCharacter的通用结构）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Animation")
	FCharacterAnimation Animation;

	// 武器配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TSubclassOf<AWeaponActor> WeaponClass;

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
	virtual void Tick(float DeltaTime) override;

	// 暴露给行为树的工具函数
	UFUNCTION(BlueprintCallable, Category="AI")
    void ExecuteAttack();

	UFUNCTION(BlueprintCallable, Category="AI")
    bool CanPerformAttack() const;

	UFUNCTION(BlueprintCallable, Category="Combat")
    void CancelAttack();

	// 攻击配置（可以在蓝图中设置多组，行为树切换）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	TArray<FAttackConfig> AttackConfigs;  // 配置攻击

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	int32 CurrentAttackIndex = 0;  // 当前使用的攻击配置索引

	// 专为枪类敌人设计
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsAiming = false;  // 是否处于瞄准状态（循环）

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

	UFUNCTION(BlueprintCallable, Category="AI")
	float GetAcceptanceRadius() const;

	UFUNCTION(BlueprintCallable, Category="AI")
	float GetAttackRange() const;

	// 获取当前动画Pitch值
	UFUNCTION(BlueprintCallable, Category="Combat")
    float GetCurrentAimPitch() const;

	// 警觉系统配置
	UPROPERTY(EditAnywhere, Category = "AI|Suspicion")
	float SuspicionIncreaseRate = 30.f;		// 每秒增加30点（约3.3秒充满）

	UPROPERTY(EditAnywhere, Category="AI|Suspicion")
	float SuspicionDecayRate = 20.f;		// 每秒减少20点（5秒清空）

	UPROPERTY(EditAnywhere, Category="AI|Suspicion")
	float SuspicionThreshold = 100.f;		// 满条进入Alert

	UFUNCTION(BlueprintCallable, Category="AI")
	float GetCurrentSuspicion() const { return CurrentSuspicion; }

	UFUNCTION(BlueprintCallable, Category="AI")
	bool IsSeeingPlayer() const { return bCanSeePlayer; }

	UFUNCTION(BlueprintCallable, Category="AI")
	EAIState GetCurrentAIState() const { return CurrentAIState; }
	
	// 当前状态（同步到Blackboard）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	EAIState CurrentAIState = EAIState::Idle;

protected:
	virtual void BeginPlay() override;

	// 武器配置
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<AWeaponActor> Weapon;

	// 警觉系统配置
	float CurrentSuspicion = 0.f;
	bool bCanSeePlayer = false;		// 当前帧是否看到玩家
	// 是否跳过警觉条，直接发现即攻击（如感染体/近战敌人）
	UPROPERTY(EditAnywhere, Category="AI|Suspicion")
	bool bSkipSuspicion = false;

	// 警觉更新
	void UpdateSuspicion(float DeltaTime);

	// 进入Alert状态（广播、设置Target等）
    void EnterAlertState();

	// 附近敌人广播响应
    void ForceEnterAlert();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UAttributeComponent* AttributeComp;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UHealthWidget> HealthWidgetClass;

	UPROPERTY()
	UHealthWidget* HealthWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
    UWidgetComponent* HealthBarComp;

	void ShowHealthBar();
	void HideHealthBar();

	// 脱战隐藏定时器（单次）
    FTimerHandle HideHealthBarTimer;
    
    // 脱战延迟时间（秒）
    UPROPERTY(EditAnywhere, Category="UI")
    float HealthBarHideDelay = 5.0f;

    // 血量变化回调（显示时机）
    UFUNCTION()
    void HandleHealthChanged(float CurrentHealth, float MaxHealth, float Delta);

	// 死亡处理
    UFUNCTION()
    void OnHealthDepleted();

	// 感知回调（看到玩家时触发）
	UFUNCTION()
    void OnSeePlayer(APawn* Pawn);

	// 同步AIController
	virtual void PostLoad() override;

	// 动画结束回调函数
	UFUNCTION()
	void OnAnimMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
private:
	FVector CachedMuzzleLocation;
    FRotator CachedMuzzleRotation;
    
    // 核心计算函数：同时更新角度和枪口位置
    void UpdateAimingData();

	// 缓存当前计算的Pitch，避免每帧算两次
    float CurrentAimPitch = 0.f;

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
