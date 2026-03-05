// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/PickupInterface.h"
#include "Character/CharacterTypes.h"
#include "GameplayTagContainer.h"
#include "AlexCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UAnimMontage;
class UWallDetectionComponent;
class UHealthWidget;
class AItem;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UAlexAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UInputConfig;
struct FGameplayTag;
class UAbilitySet;
struct FGameplayAbilitySpecHandle;
class AWeaponActor;
class UFormWheelWidget;
class UProgressBar;
class AEnemy;
class UAmmoWidget;

USTRUCT(BlueprintType)
struct FMorphConfig
{
	GENERATED_BODY()

	// 形态类型标识
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morph")
	EMorphType Type = EMorphType::EMT_Fist;

	// 武器
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	TSubclassOf<AWeaponActor> WeaponClass; // 该形态使用的武器（拳形态可留空）

	// 检测参数（每种形态不同）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MeleeRadius = 100.f;		// 拳80，爪100，刀120，鞭200，锤150

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float AttackAngle = 90.f;       // 刀90°，鞭360°（全周），锤120°

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    float Damage = 25.f;

	// 连段配置（每种形态独立）
    // 拳：["Jab1", "Jab2", "Uppercut"]
    // 鞭：["Swipe_Start", "Swipe_Mid", "Swipe_End"]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combo")
    TArray<FName> LightComboSections;

	// 该形态的蒙太奇（如果每种形态用不同蒙太奇文件）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
    UAnimMontage* ComboMontage = nullptr;

	// 特殊能力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special")
    bool bCanCharge = false;        // 比如锤可以蓄力

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special")
    bool bIsMultiHit = false;       // 鞭子是否多次判定（true的话动画里加多个通知）

	// 切换特效（身体变形时的视觉反馈）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
    UNiagaraSystem* SwitchEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SFX")
    USoundBase* SwitchSound = nullptr;
};

USTRUCT(BlueprintType)
struct FSurfaceSoundSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
    TMap<FName, USoundBase*> SurfaceSounds;  // Tag -> Sound

	UPROPERTY(EditAnywhere)
    USoundBase* DefaultSound;  // 默认
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReserveAmmoChanged, int32, NewReserve, int32, MaxReserve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeldWeaponChanged, AWeaponActor*, NewWeapon, AWeaponActor*, OldWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AActor*, Victim);

UCLASS()
class PROTOTYPE_API AAlexCharacter : public ABaseCharacter, public IPickupInterface, public IAbilitySystemInterface  
{
	GENERATED_BODY()
	
public:
	AAlexCharacter();
	virtual void Tick(float DeltaTime) override;
	void ResetRun();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void SetOverlappingItem(AItem* Item) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

	UAlexAttributeSet* GetAttributeSet() const { return AttributeSet; }

	EActionState GetActionState() const { return ActionState; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WallDetection")
	UWallDetectionComponent* WallDetection;

	/*蓝图攀爬方法*/
	UFUNCTION(BlueprintImplementableEvent, Category="Climb")
	void BP_OnStartClimb();

	/* 蓝图墙跑方法 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WallRun")
	void BP_OnStartWallRun();

	// 获取墙跑状态（用于AnimInstance）
	UFUNCTION(BlueprintCallable)
	bool GetbIsWallRunning()const { return bIsWallRunning; }

	// 获取玩家锁定目标（用于敌人死亡解锁）
	UFUNCTION(BlueprintCallable, Category="LockOn")
	AActor* GetLockedTarget() const { return LockedTarget; }

	// 获取当前形态配置
	const FMorphConfig& GetCurrentMorphConfig() const;

	// 连击管理（给GA）
    UFUNCTION(BlueprintCallable, Category="Combat")
	void ResetAttackCombo() {
		CurrentComboStep = 0;
		SetComboWindowOpen(false); 
		UE_LOG(LogTemp, Warning, TEXT(">>> Combo Reset"));
	}
    
    UFUNCTION(BlueprintCallable, Category="Combat")
	void AdvanceAttackCombo() {
		CurrentComboStep++;
		UE_LOG(LogTemp, Warning, TEXT("CurrentComboStep: %d"), CurrentComboStep);
	}
    
    UFUNCTION(BlueprintCallable, Category="Combat")
    int32 GetAttackComboStep() const { return CurrentComboStep; }


	// 敌人注册/注销（只有看到玩家的敌人才会注册）
    UFUNCTION()
    void RegisterSuspicionSource(AEnemy* Enemy);
    
    UFUNCTION()
    void UnregisterSuspicionSource(AEnemy* Enemy);

	// 瞄准相关
	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	float CurrentAimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Aiming")  
	float CurrentAimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	FRotator CachedMuzzleRotation;

	void UpdateAimingData();  // 计算瞄准数据

	UFUNCTION()
	float GetAimPitch() const { return CurrentAimPitch; }

	UFUNCTION()
	float GetAimYaw() const { return CurrentAimYaw; }

	UFUNCTION()
	AWeaponActor* GetHeldWeapon() const { return HeldWeapon; }

    UPROPERTY(BlueprintAssignable, Category="Weapon|UI")
    FOnReserveAmmoChanged OnReserveAmmoChanged;

    UFUNCTION(BlueprintCallable, Category="Weapon|UI")
    void GetAmmoDisplay(int32& OutCurrent, int32& OutReserve) const;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
    FOnHeldWeaponChanged OnHeldWeaponChanged;

    // 设置当前武器（内部调用，会广播事件）
    UFUNCTION(BlueprintCallable, Category="Weapon")
    void SetHeldWeapon(AWeaponActor* NewWeapon);

	UFUNCTION()
	FRotator GetCachedMuzzleRotation() const { return CachedMuzzleRotation; }

	UFUNCTION(BlueprintCallable, Category="LockOn")
	void ConfirmLockOn();              // 松开 Tab 确认锁定

	UFUNCTION(BlueprintCallable, Category="LockOn")
	void CancelLockOn();               // 取消锁定

	// 连击窗口
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
	bool bComboWindowOpen = false;

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsComboWindowOpen() const { return bComboWindowOpen; }
	
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetComboWindowOpen(bool bOpen) { bComboWindowOpen = bOpen; }

	// 禁止移动（供攻击GA调用）
	UFUNCTION(BlueprintCallable, Category="Movement")
    void SetMovementLock(bool bLocked) { bLockMove = bLocked; }

	/*
	*	死亡逻辑
	*/
	// 死亡委托（给 UI 或 GameMode 监听）
	UPROPERTY(BlueprintAssignable)
	FOnCharacterDeath OnCharacterDeath;

	UFUNCTION(BlueprintCallable, Category="Death")
    void OnDeath();  // 死亡入口

	UFUNCTION(BlueprintPure, Category="Death")
    bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category="Death")
	void ClearAllSuspicionOnDeath();

	// 滑翔时Dash退出滑翔逻辑（供GA_Dash使用）
	UFUNCTION()
	void DASH();

	// 杀死敌人加血
	UFUNCTION(BlueprintCallable, Category = "Health")
	void OnKillEnemy(float HealAmount = 300.f);

protected:
	virtual void BeginPlay() override;

	bool HasMovementInput();

	void MOVE(const FInputActionValue& Value);
	void MOVE_Completed(const FInputActionValue& Value);
	void LOOK(const FInputActionValue& Value);
	void ZOOM(const FInputActionValue& Value);
	void JumpPressed();
	void JumpReleased();
	void WALK();
	void RUN();
	void CLIMB();
	void ATTACK();
	void ATTACK_Released();
	void TAB_Pressed();
	void TAB_Released();
	void PICKUP();
	void Crouch();
	void OnAssassinatePressed();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Landed(const FHitResult& Hit) override;	// 着陆触发事件
	/*
	*	Montages
	*/
	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* ChargeMontage;		// 蓄力动作

	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* LaunchMontage;		// 跳动作

	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* GlideMontage;			//滑翔动作

	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* DashMontage;			//冲刺动作

	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* RunningVaultMontage;	// 奔跑翻越动作

	UPROPERTY(EditAnywhere, Category = "AnimMontage")
	UAnimMontage* RoofFlipMontage;		// 屋顶翻滚动作

	UPROPERTY(EditAnywhere, Category = "AnimMontage")
	UAnimMontage* SkidMontage;			// 急刹滑行动作

	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* ClimbMantleMontage;	// 爬上楼顶动作

	UPROPERTY(EditAnywhere, Category="AnimMontage")
	UAnimMontage* WallRunBackFlipMontage;	// 墙跑急停后空翻动作

	UPROPERTY(EditDefaultsOnly, Category="Death")
    UAnimMontage* DeathMontage;			// 死亡动作

	UPROPERTY(EditDefaultsOnly, Category="Assassination")
	UAnimMontage* AssassinationMontage;  // 玩家暗杀动作

	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	UAnimMontage* GrabMontage;	// 抓取敌人动作

	/* 相机距离限制 */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinCameraDistance = 150.0f;  // 最近（第三人称过肩视角）
	
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxCameraDistance = 600.0f;  // 最远（全景视角）
	
	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomSpeed = 50.0f;  // 缩放灵敏度

	// HealthWidgetClass
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    TSubclassOf<UHealthWidget> HealthWidgetClass;

	// HealthWidget实例
    UPROPERTY()
    TObjectPtr<UHealthWidget> HealthWidget;

	// FormWheelWidgetClass
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UFormWheelWidget> FormWheelWidgetClass;

	// FormWheelWidget实例
	UPROPERTY()
	TObjectPtr<UFormWheelWidget> FormWheelWidget;

	// AmmoWidgetClass
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UAmmoWidget> AmmoWidgetClass;

	// AmmoWidget实例
	UPROPERTY()
	TObjectPtr<UAmmoWidget> AmmoWidget;

	// 警觉UI相关配置
	// 当前正在观察玩家的敌人列表（自动去重）
    UPROPERTY()
    TArray<AEnemy*> WatchingEnemies;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> SuspicionWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> SuspicionWidget;

	UPROPERTY()
	TObjectPtr<UProgressBar> SuspicionBar;

	// 更新警觉UI
    void UpdateSuspicionUI();
    
    // 缓存所有敌人（优化：不需要每帧FindAll）
    TArray<AEnemy*> GetVisibleEnemies();

	// GAS
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAlexAttributeSet> AttributeSet;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TSubclassOf<UGameplayAbility> AttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input")
	TObjectPtr<UInputConfig> InputConfig;

	// 新的统一输入回调（替代原来的 DASH(), ATTACK() 等分散函数）
	UFUNCTION()
	void OnAbilityInputPressed(FGameplayTag InputTag);
	
	UFUNCTION()
	void OnAbilityInputReleased(FGameplayTag InputTag);

	// GAS初始化（必须）
    virtual void PossessedBy(AController* NewController) override;

private:
	UPROPERTY(BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	bool bHasMovementInput = false;			// 是否有移动输入

	// 死亡相关
	bool bIsDead = false;
    FTimerHandle DeathTimerHandle;

	/*
	* 存档
	*/
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadGame();

	/*
	* crouch暗杀相关
	*/
	UPROPERTY(EditAnywhere, Category="Crouch")
	float CrouchWalkSpeed = 200.f;
	
	// 站立时缓存的速度（用于恢复）
	float PreCrouchWalkSpeed = 400.f;

	UPROPERTY(BlueprintReadOnly, Category="Crouch", meta=(AllowPrivateAccess="true"))
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category="Assassination", meta=(AllowPrivateAccess="true"))
	bool bIsAssassinating = false;

	UPROPERTY(BlueprintReadOnly, Category="Assassination", meta=(AllowPrivateAccess="true"))
	AEnemy* AssassinationTarget = nullptr;

	bool CanEnterCrouch() const;
	void ExitCrouch();

	/* 暗杀配置参数 */
	UPROPERTY(EditAnywhere, Category="Assassination")
	float AssassinationRange = 150.f;  // 1.5米（150cm）
	
	UPROPERTY(EditAnywhere, Category="Assassination")
	float AssassinationAngleThreshold = 0.5f;  // DotProduct阈值
	
	/* 暗杀函数 */
	UFUNCTION()
	AEnemy* FindNearestAssassinationTarget();

	UFUNCTION()
	bool CanAssassinate(AEnemy* Target) const;
	
	UFUNCTION()
	void StartAssassinate(AEnemy* Target);
	
	UFUNCTION()
	void EndAssassinate();

	UFUNCTION()
	void OnAssassinationMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	// 动画通知回调（在动画中段调用，实际造成伤害）
	UFUNCTION(BlueprintCallable, Category="Assassination")
	void ExecuteAssassinationDamage();

	/*
	* 形态轮盘相关
	*/
	// 已解锁的形态（存档用）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Morph", SaveGame, meta = (AllowPrivateAccess = "true"))
	TSet<FGameplayTag> UnlockedFormTags;
	
	void ShowMorphWheel();
	void HideMorphWheelAndConfirm();
	void UpdateMorphWheelSelection();  // Tick中调用或绑定鼠标移动
	void UnlockForm(FGameplayTag FormTag);  // 剧情事件调用

	/*
	* 战斗相关
	*/
	UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
	TMap<FGameplayTag, FMorphConfig> FormMorphConfigs;

	// 当前武器形态
	UPROPERTY()
	TObjectPtr<AWeaponActor> CurrentWeapon;

	// 手持武器（如枪、RPG）
	UPROPERTY()
	TObjectPtr<AWeaponActor> HeldWeapon;

	// 武器形态切换函数
	void AttachWeaponForForm(FGameplayTag FormTag);
	void DetachCurrentWeapon();

	// 当前连段进度（相对于当前形态）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat", meta = (AllowPrivateAccess = "true"))
	int32 CurrentComboStep = 0;

	// 形态切换（供输入调用）
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SwitchToForm(FGameplayTag NewFormTag);

	// 各形态对应的技能组配置
	UPROPERTY(EditDefaultsOnly, Category="GAS|Forms")
	TMap<FGameplayTag, TObjectPtr<UAbilitySet>> FormAbilitySets;

	// 当前激活的技能Handles（用于切换时移除）
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> CurrentFormAbilityHandles;

	// 当前形态Tag（替代之前的int32 CurrentMorphIndex作为唯一标识）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
	FGameplayTag CurrentFormTag;
	
	/*
	* 动态音效
	*/
	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlaySurfaceSound(ECharacterSoundType SoundType, FName SurfaceTagOverride);

	UPROPERTY(EditAnywhere, Category="Audio")
	TMap<ECharacterSoundType, FSurfaceSoundSet> CharacterSounds;

	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayFootstepSound() { PlaySurfaceSound(ECharacterSoundType::Footstep, NAME_None); }

	UFUNCTION(BlueprintCallable, Category="Audio")  
	void PlayFootstepRun()  { PlaySurfaceSound(ECharacterSoundType::FootstepRun, NAME_None); }

	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayJumpSound() { PlaySurfaceSound(ECharacterSoundType::Jump, NAME_None); }

	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayLandSound() { PlaySurfaceSound(ECharacterSoundType::Land, NAME_None); }

	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayAttackSound() { PlaySurfaceSound(ECharacterSoundType::Attack, NAME_None); }
	
	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayHitSound() { PlaySurfaceSound(ECharacterSoundType::Hit, NAME_None); }

	/*
	* 软锁定
	*/
	UPROPERTY()
	bool bIsSelectingTarget = false;  // 是否按住 Tab 选择中

	UPROPERTY()
	AActor* HoveredTarget = nullptr;   // 当前悬停的目标

	UPROPERTY()
	AActor* LockedTarget = nullptr;    // 确认锁定的目标

	UPROPERTY(EditAnywhere, Category="LockOn")
	float LockOnRange = 5000.f;        // 锁定最大距离（5000cm = 50米）
	
	UPROPERTY(EditAnywhere, Category="LockOn")
	float LockOnSphereRadius = 300.f;  // 屏幕中心检测半径（越大越容易选中）

	UPROPERTY(EditAnywhere, Category="LockOn", meta=(ClampMin=1, ClampMax=20))
	float CameraRotationSpeed = 8.f;

	// 锁定UI
	UPROPERTY(EditDefaultsOnly, Category = "LockOn|UI")
	TSubclassOf<UUserWidget> LockOnReticleClass;	// 悬停框（半透明大框）

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|UI")
	TSubclassOf<UUserWidget> LockOnLockedClass;		// 锁定框（红色实线框）

	UPROPERTY()
	UUserWidget* HoverWidget;

	UPROPERTY()
	UUserWidget* LockedWidget;

	void UpdateLockOnUI();
	void UpdateTargetWidget(AActor* Target, UUserWidget* Widget, APlayerController* PC);
	bool GetTargetScreenPixelSize(AActor* Target, APlayerController* PC, FVector2D& OutSize, FVector2D& OutCenter);
	bool GetTargetScreenBounds(AActor* Target, APlayerController* PC, FVector2D& OutMin, FVector2D& OutMax);

	// 核心逻辑
	void UpdateTargetSelection();      // 按住 Tab 时每帧更新悬停目标
	void UpdateLockOnCamera(float DeltaTime);  // 锁定时更新相机

	/* 
	* 自动翻越参数
	*/
	UPROPERTY(BlueprintReadWrite, Category="Vault", meta=(AllowPrivateAccess="true"))
	bool bIsVaulting = false;
	
	UPROPERTY(BlueprintReadWrite, Category="Vault", meta=(AllowPrivateAccess="true"))
	bool bVaultQueued = false;  //是否有下一跳在排队
	
	UPROPERTY(EditAnywhere, Category="Vault")
	float VaultHeightThreshold = 60.f;  // 最低翻越高度（低于此高度不触发）
	
	UPROPERTY(EditAnywhere, Category="Vault")
	float MaxVaultHeight = 80.f;       // 最大翻越高度（防止跳太高）
	
	UPROPERTY(EditAnywhere, Category="Vault")
	float VaultHorizontalSpeed = 350.f; // 最小水平速度

	UPROPERTY(EditAnywhere, Category="Vault")
	float MaxVaultHorizontalSpeed = 550.f;	// 最大水平速度

	UPROPERTY(EditAnywhere, Category="Vault", meta=(ClampMin=0.5, ClampMax=2.0, UIMin=0.5, UIMax=2.0))
	float VaultVerticalMultiplier = 1.4f; // 默认1.4倍
	
	// 连续翻越方法
	void TryAutoVault();                    // 尝试翻越（可入队）
	void StartVault(const FVector& LaunchVel);  // 立即执行翻越
	UFUNCTION(BlueprintCallable, Category="Vault")
	void OnVaultAnimEnd();					// 动画结束重置状态
	float CalculateVaultHeight() const;     // 计算障碍物高度
	FVector CalculateVaultVelocity();       // 动态计算跳跃速度

	/* 
	* 墙跑参数 
	*/
	UPROPERTY(BlueprintReadWrite, Category="WallRun", meta = (allowPrivateAccess = "true"))
	bool bIsWallRunning = false;           // 是否正在墙跑

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="WallRun",meta=(AllowPrivateAccess = "true"))
	FVector WallRunDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WallRun", meta=(AllowPrivateAccess = "true"))
	float WallRunSpeed = 600.f;          // 墙跑速度

	UPROPERTY(EditAnywhere, Category = "WallRun")
	float WallRunGravityScale = 0.f;    // 墙跑时重力缩放

	UPROPERTY(EditAnywhere, Category = "WallRun")
	float WallRunDetectionDistance = 100.f; // 离墙距离阈值

	UPROPERTY()
	float PreWallRunGravityScale;

	/* 墙跑跳跃冷却（防止跳跃后立即重新进入墙跑） */
	UPROPERTY(BlueprintReadOnly, Category="WallRun", meta=(AllowPrivateAccess="true"))
	float WallRunJumpCooldown = 0.f;
	
	UPROPERTY(EditAnywhere, Category="WallRun")
	float WallRunJumpCooldownDuration = 0.7f;  // 冷却时间（秒），推荐0.3-0.5

	bool bIsRoofFlipping = false;

	//预判楼顶
	bool CheckApproachingRoof();

	// 前空翻函数
	void PerformRoofFlip();
	void OnRoofFlipEnded(UAnimMontage* Montage, bool bInterrupted);

	// 墙跑急停后空翻
	void PerformWallRunBackFlip();
	void OnWallRunBackFlipEnded(UAnimMontage* Montage, bool bInterrupted);

	// 墙跑方法
	void StartWallRun();
	void StopWallRun();
	void UpdateWallRun(float DeltaTime);

	/* 
	* 攀爬参数 
	*/
	UPROPERTY(BlueprintReadWrite, Category="Climb", meta = (allowPrivateAccess = "true"))
	bool bIsClimbing = false;           // 是否正在攀爬

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Climb",meta=(AllowPrivateAccess = "true"))
	FVector ClimbDirection;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb", meta=(AllowPrivateAccess = "true"))
	float ClimbSpeed = 100.f;           // 攀爬速度
	
	UPROPERTY(EditAnywhere, Category="Climb")
	float ClimbGravityScale = 0.0f;     // 攀爬时重力缩放
	
	UPROPERTY(EditAnywhere, Category="Climb")
	float ClimbDetectionDistance = 70.f; // 离墙距离阈值

	UPROPERTY()
	FVector PreClimbVelocity;
	
	UPROPERTY()
	float PreClimbGravityScale;
	
	// 攀爬方法
	void StartClimb();
	void StopClimb();
	void UpdateClimb(float DeltaTime);

	// 检测攀爬到楼顶
	bool CheckClimbMantle();

	// 爬上楼顶
	void PerformClimbMantle();
	void OnClimbMantleEnded(UAnimMontage* Montage, bool bInterrupted);

	/* 
	* 滑翔参数 
	*/
	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float InitialGlideUpwardSpeed = 300.f;  // 进入滑翔时的初始向上速度

	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float GlideDescentSpeed = 500.f;        // 滑翔下降初始速度

	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float CurrentGlideDescentSpeed = 500.f;     // 当前滑翔下降速度

	UPROPERTY(EditAnywhere, Category = "Glide", meta=(ClampMin=0.0))
	float DescentRate = 100.f;

	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float MaxGlideDescentSpeed = 1500.f;     // 最大滑翔下降速度
	
	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float GlideTurnSpeed = 30.f;           // 滑翔转向速度（度/秒）
	
	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float MaxGlideTiltAngle = 30.f;         // 最大倾斜角度
	
	UPROPERTY(EditAnywhere, Category="Glide", meta=(ClampMin=0.0))
	float GlideTiltSpeed = 5.f;             // 倾斜插值速度
	
	UPROPERTY(VisibleAnywhere, Category="Glide")
	float CurrentGlideTilt = 0.f;           // 当前倾斜角度
	
	UPROPERTY(BlueprintReadWrite, Category="Input", meta = (AllowPrivateAccess = "true"))
	FVector2D LastMovementInput;            // 存储原始输入（不依赖摄像头）
	
	// 滑翔方法
	void StartGlide();
	void StopGlide();
	void UpdateGlide(float DeltaTime);
	bool CanGlide() const;

	/* 
	* 奔跑参数 
	*/
	UPROPERTY()
	bool bHadInputLastFrame = false;   // 上一帧是否有移动输入
	
	UPROPERTY()
	bool bRunActive = false;           // true=当前处于奔跑锁定状态

	UPROPERTY()
	bool bIsSkidding = false;			//是否在急停

	// 惯性急停
	void PlaySkidMontage();
	void InterruptSkid();

	/* 
	* 蓄力跳参数 
	*/
	UPROPERTY(EditAnywhere, Category="ChargeJump")
	float MaxChargeTime = 1.2f;          // 满蓄力时长
	
	UPROPERTY(EditAnywhere, Category="ChargeJump")
	float MaxJumpZMult = 2.2f;           // 相对普通跳的最大倍率

	bool bLockMove = false;   // 蓄力锁

	/* 蓄力运行时状态 */
	bool bIsCharging = false;
	float CurrentChargeTime = 0.f;
	FTimerHandle ChargeHandle;

	void StartCharge();
	void ExecuteChargeJump();

	/* 
	 *冲刺参数 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DashAbilityClass;
	
	/*
	*	镜头相关
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SceneComponent", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SceneComponent", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* ViewCamera;

	/*
	* 增强输入相关
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RunAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbAction;	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LockOnAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MorphWheelAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PickupAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AssassinateAction;

	UPROPERTY(VisibleAnywhere)
	AItem* OverlappingItem;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EActionState ActionState = EActionState::EAS_Unoccupied;//初始化动作状态为未占用
};
