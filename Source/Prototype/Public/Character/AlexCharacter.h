// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "Interfaces/PickupInterface.h"
#include "Character/CharacterTypes.h"
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

UCLASS()
class PROTOTYPE_API AAlexCharacter : public ABaseCharacter, public IPickupInterface
{
	GENERATED_BODY()
	
public:
	AAlexCharacter();
	virtual void Tick(float DeltaTime) override;
	void ResetRun();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void SetOverlappingItem(AItem* Item) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WallDetection")
	UWallDetectionComponent* WallDetection;

	/*蓝图攀爬方法*/
	UFUNCTION(BlueprintImplementableEvent, Category="Climb")
	void BP_OnStartClimb();
	
	UFUNCTION(BlueprintImplementableEvent, Category="Climb")
	void BP_OnUpdateClimb(float DeltaTime);

	/* 蓝图墙跑方法 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WallRun")
	void BP_OnStartWallRun();

	UFUNCTION(BlueprintImplementableEvent, Category = "WallRun")
	void BP_OnUpdateWallRun(float DeltaTime);

protected:
	virtual void BeginPlay() override;

	bool HasMovementInput();

	void MOVE(const FInputActionValue& Value);
	void MOVE_Completed(const FInputActionValue& Value);
	void LOOK(const FInputActionValue& Value);
	void JumpPressed();
	void JumpReleased();
	void WALK();
	void RUN();
	void DASH();
	void CLIMB();

	UFUNCTION()
	void DashEnd();

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

	UPROPERTY(EditAnywhere, Category="Vault")
	UAnimMontage* RunningVaultMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UAttributeComponent* AttributeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    TSubclassOf<UHealthWidget> HealthWidgetClass;

	// Widget实例
    UPROPERTY()
    UHealthWidget* HealthWidget;

private:
	

	UPROPERTY(BlueprintReadOnly, Category="Input", meta = (AllowPrivateAccess = "true"))
	bool bHasMovementInput = false;			// 是否有移动输入

	/* 自动翻越参数*/
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
	float VaultVerticalMultiplier = 1.4f; // 默认1.2倍
	
	// 连续翻越方法
	void TryAutoVault();                    // 尝试翻越（可入队）
	void StartVault(const FVector& LaunchVel);  // 立即执行翻越
	UFUNCTION(BlueprintCallable, Category="Vault")
	void OnVaultAnimEnd();					// 动画结束重置状态
	float CalculateVaultHeight() const;     // 计算障碍物高度
	FVector CalculateVaultVelocity();       // 动态计算跳跃速度

	/* 墙跑参数 */
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


	// 墙跑方法
	void StartWallRun();
	void StopWallRun();
	void UpdateWallRun(float DeltaTime);

	/* 攀爬参数 */
	UPROPERTY(BlueprintReadWrite, Category="Climb", meta = (allowPrivateAccess = "true"))
	bool bIsClimbing = false;           // 是否正在攀爬

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Climb",meta=(AllowPrivateAccess = "true"))
	FVector ClimbDirection;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb", meta=(AllowPrivateAccess = "true"))
	float ClimbSpeed = 1.f;           // 攀爬速度
	
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

	/* 滑翔参数 */
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

	/* 奔跑参数 */
	UPROPERTY()
	bool bHadInputLastFrame = false;   // 上一帧是否有移动输入
	
	UPROPERTY()
	bool bRunActive = false;           // true=当前处于奔跑锁定状态


	/* 蓄力跳参数 */
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

	/* 冲刺参数 */
	UPROPERTY(EditAnywhere, Category="Dash")
	float AirDashSpeed = 10000.f;		// 空中冲刺速度

	UPROPERTY()
	float DashDuration = 0.2f;		// 冲刺持续时间

	bool ConsumeDashIfAvailable();				// 是否正在冲刺

	UPROPERTY(EditAnywhere, Category="Dash")
	int DashCount = 2;				// 冲刺次数

	FTimerHandle DashHandle;		// 冲刺计时器

	
	/*
	*	镜头及输入
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SceneComponent", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SceneComponent", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* ViewCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

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

	UPROPERTY(VisibleAnywhere)
	AItem* OverlappingItem;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EActionState ActionState = EActionState::EAS_Unoccupied;//初始化动作状态为未占用
};
