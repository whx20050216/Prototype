// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AlexCharacter.h"
#include "GameFramework/SpringArmComponent.h"//弹簧臂
#include "Camera/CameraComponent.h"//摄像机
#include "InputActionValue.h"//用于增强输入
#include "EnhancedInputComponent.h"//用于增强输入
#include "EnhancedInputSubsystems.h"//用于增强输入
#include "GameFramework/Controller.h"//用于增强输入
#include "GameFramework/CharacterMovementComponent.h"//角色移动组件
#include "Components/CapsuleComponent.h"
#include "Character/AlexAnimInstance.h"
#include "ActorComponent/WallDetectionComponent.h"
#include "Kismet/KismetSystemLibrary.h"

AAlexCharacter::AAlexCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->TargetArmLength = 300.0f;
	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	SpringArm->SetupAttachment(GetRootComponent());
	ViewCamera->SetupAttachment(SpringArm);

	//让控制器的转动不影响角色，只影响摄像机
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	//让角色面朝加速度方向
	GetCharacterMovement()->bOrientRotationToMovement = true;
	//使用Pawn的旋转
	SpringArm->bUsePawnControlRotation = true;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	LastMovementInput = FVector2D::ZeroVector;// 存储原始输入

	WallDetection = CreateDefaultSubobject<UWallDetectionComponent>(TEXT("WallDetection"));
}

void AAlexCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsCharging)
	{
		CurrentChargeTime = FMath::Min(CurrentChargeTime + DeltaTime, MaxChargeTime);
	}

	// 处理墙跑跳跃冷却
	if (WallRunJumpCooldown > 0.f)
	{
		WallRunJumpCooldown -= DeltaTime;
	}
	
	// 更新滑翔逻辑
    UpdateGlide(DeltaTime);

	/* 奔跑状态重置逻辑 */
	ResetRun();

	WallDetection->UpdateDetection();

	//疾跑时尝试自动翻越
	if (bRunActive)
	{
		TryAutoVault();
	}

	//奔跑或滑翔下自动检测并启动墙跑
	bool bShouldCheckWallRun = (bRunActive || ActionState == EActionState::EAS_Gliding);
	if (!bIsWallRunning && !bIsClimbing && !bIsVaulting && WallDetection->bCanWallRun && bShouldCheckWallRun && WallRunJumpCooldown <= 0.f)
    {
        StartWallRun();
    }

	if (bIsClimbing)
    {
        UpdateClimb(DeltaTime);
    }

	if (bIsWallRunning)
	{
		UpdateWallRun(DeltaTime);
	}
}

void AAlexCharacter::ResetRun()
{
	bool bHasNow = HasMovementInput();
	bool bJustPressed = bHasNow && !bHadInputLastFrame;
	bool bJustReleased = !bHasNow && bHadInputLastFrame;

	if (bRunActive && (bJustPressed || bJustReleased))
	{
		bRunActive = false;
		GetCharacterMovement()->MaxWalkSpeed = 400.f;
	}
	bHadInputLastFrame = bHasNow;
}

void AAlexCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAlexCharacter::MOVE);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AAlexCharacter::MOVE_Completed);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAlexCharacter::LOOK);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAlexCharacter::JumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAlexCharacter::JumpReleased);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &AAlexCharacter::WALK);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Triggered, this, &AAlexCharacter::RUN);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AAlexCharacter::DASH);
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &AAlexCharacter::CLIMB);
	}
}

void AAlexCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

bool AAlexCharacter::HasMovementInput()
{
	return bHasMovementInput;
}

void AAlexCharacter::MOVE(const FInputActionValue& Value)
{
	if (bLockMove) return;
	FVector2D MovementVector = Value.Get<FVector2D>();

	bHasMovementInput = !MovementVector.IsNearlyZero();
	LastMovementInput = MovementVector; // 存储原始输入

	//阻止所有其他移动状态
    if (bIsClimbing || bIsWallRunning || ActionState == EActionState::EAS_Gliding)
    {
        return;
    }

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AAlexCharacter::MOVE_Completed(const FInputActionValue& Value)
{
	LastMovementInput = FVector2D::ZeroVector;
    bHasMovementInput = false;
	if (bIsWallRunning)
	{
		StopWallRun();
	}
	if (!bIsClimbing)
    {
        LastMovementInput = FVector2D::ZeroVector;
    }
	bHasMovementInput = false;
}

void AAlexCharacter::LOOK(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AAlexCharacter::JumpPressed()
{
	// 如果正在滑翔，再次按下空格停止滑翔
    if (ActionState == EActionState::EAS_Gliding)
    {
        StopGlide();
        return;
    }

	StartCharge();
}

void AAlexCharacter::JumpReleased()
{
	// 滑翔状态下不处理跳跃释放
    if (ActionState == EActionState::EAS_Gliding) return;

	if (!bIsCharging) return;
	bIsCharging = false;

	bool bWasWallRunning = bIsWallRunning;
	bool bWasClimbing = bIsClimbing;

	if (bWasWallRunning) StopWallRun();
	if (bWasClimbing) StopClimb();

	GetCharacterMovement()->bConstrainToPlane = false;
	bLockMove = false;

	if (bWasWallRunning)
	{
		WallRunJumpCooldown = WallRunJumpCooldownDuration;
	}

	if (bWasWallRunning || bWasClimbing)
	{
		ExecuteChargeJump();
		return; // 直接返回，不检查滑翔
	}

	if (CanGlide())
	{
		StartGlide();
		return;
	}

	if (GetCharacterMovement()->IsFalling()) return;

	ExecuteChargeJump();
}

void AAlexCharacter::WALK()
{
	// 滑翔时禁用行走切换
    if (ActionState == EActionState::EAS_Gliding) return;

	if (ActionState == EActionState::EAS_Walk)
	{
		GetCharacterMovement()->MaxWalkSpeed = 400.f;
		ActionState = EActionState::EAS_Unoccupied;
		bRunActive = false;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 150.f;
		ActionState = EActionState::EAS_Walk;
	}
	
}

void AAlexCharacter::RUN()
{
	// 滑翔时禁用奔跑
    if (ActionState == EActionState::EAS_Gliding) return;

	if (!bRunActive)
	{
		bRunActive = true;
		GetCharacterMovement()->MaxWalkSpeed = 1500.f;
	}
}

void AAlexCharacter::DASH()
{
	if (bIsClimbing) return;
	if (!ConsumeDashIfAvailable()) return;
	FVector OldSpeed = GetCharacterMovement()->Velocity;
	OldSpeed.Z = 0.f;
	GetCharacterMovement()->Velocity = GetActorForwardVector() * AirDashSpeed;
	if (DashMontage)
	{
		PlayMontageSection(DashMontage, FName("Dash"));
		StopGlide();
	}
	GetWorld()->GetTimerManager().SetTimer(DashHandle, this, &AAlexCharacter::DashEnd, DashDuration, false);
}

void AAlexCharacter::CLIMB()
{
	if (bIsClimbing)
	{
		StopClimb();
	}
	else if(WallDetection && WallDetection->bCanWallRun)
	{
		StartClimb();
	}
}

void AAlexCharacter::TryAutoVault()
{
	if (bIsVaulting)
	{
		bVaultQueued = true;
		return;
	}

	// 检测条件：地面移动 + 可翻越 + 不在特殊状态
	bool bIsGrounded = GetCharacterMovement()->MovementMode == MOVE_Walking;
	bool bIsInSpecialMove = bIsWallRunning || bIsClimbing || ActionState == EActionState::EAS_Gliding;
	if (!bIsGrounded || bIsInSpecialMove || !WallDetection || !WallDetection->bCanVault)
    {
        return;
    }

	// 计算并执行
	FVector LaunchVel = CalculateVaultVelocity();
    if (!LaunchVel.IsNearlyZero())
    {
        StartVault(LaunchVel);
    }
}

void AAlexCharacter::StartVault(const FVector& LaunchVel)
{
	bIsVaulting = true;
	ActionState = EActionState::EAS_Vaulting;
	bVaultQueued = false;

	// 禁用角色控制，切换到飞行模式
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->StopMovementImmediately();

	// 应用跳跃速度
    LaunchCharacter(LaunchVel, true, true);

	// 播放翻越动画
    if (RunningVaultMontage)
    {
        PlayMontageSection(RunningVaultMontage, FName("Vault"));
    }
}

void AAlexCharacter::OnVaultAnimEnd()
{
	// 恢复移动状态
    bIsVaulting = false;
    ActionState = EActionState::EAS_Unoccupied;
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement = true;

    // 处理排队：立即尝试下一次翻越
    if (bVaultQueued)
    {
        bVaultQueued = false;
        TryAutoVault(); // 递归调用，处理连续翻越
    }
}

float AAlexCharacter::CalculateVaultHeight() const
{
	if (!WallDetection || !WallDetection->GetUpToDownHit().bBlockingHit) return 0.f;

	// 角色脚部Z坐标
    float CharacterFootZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// 障碍物顶部Z坐标（UpToDownHit的命中点）
    float ObstacleTopZ = WallDetection->GetUpToDownHit().Location.Z;

	// 返回相对高度
	return ObstacleTopZ - CharacterFootZ;
}

FVector AAlexCharacter::CalculateVaultVelocity()
{
	float Height = CalculateVaultHeight();

	// 限制在合理范围
    Height = FMath::Clamp(Height, VaultHeightThreshold, MaxVaultHeight);
	if (Height <= 0) return FVector::ZeroVector;

	// 物理公式：v = √(2gh)，加10%安全余量
    float Gravity = FMath::Abs(GetCharacterMovement()->GetGravityZ());
    float VerticalVel = FMath::Sqrt(2 * Gravity * Height) * VaultVerticalMultiplier;

	// 水平速度：保持当前方向，最低500
    FVector HorizontalVel = GetCharacterMovement()->Velocity;
    HorizontalVel.Z = 0;
	float CurrentHorizSpeed = HorizontalVel.Size();
	float ClampedHorizSpeed = FMath::Clamp(CurrentHorizSpeed, VaultHorizontalSpeed, MaxVaultHorizontalSpeed);
    if (CurrentHorizSpeed < KINDA_SMALL_NUMBER)
    {
        HorizontalVel = GetActorForwardVector() * VaultHorizontalSpeed;
    }
    else
    {
        // 保持方向，但应用限制后的速度大小
        HorizontalVel = HorizontalVel.GetSafeNormal() * ClampedHorizSpeed;
    }

	FVector LaunchVel = HorizontalVel.GetSafeNormal() * HorizontalVel.Size();
    LaunchVel.Z = VerticalVel;

    return LaunchVel;
}

void AAlexCharacter::StartWallRun()
{
	bool bIsAutoFromGlide = (ActionState == EActionState::EAS_Gliding);

	if (!bRunActive && bIsAutoFromGlide) return;  // 不在奔跑状态，直接返回

	if (!WallDetection || !WallDetection->bCanWallRun) return;  // 没有墙

	if (WallDetection->ActorToWallDistance > WallRunDetectionDistance) return;  // 距离不够，不跑

	FVector WallNormal = WallDetection->WallNormal;
	float Angle = FMath::Acos(FVector::DotProduct(WallNormal, FVector::UpVector));
	if (Angle < FMath::DegreesToRadians(30.0f) || Angle > FMath::DegreesToRadians(90.0f))
	{
	    return;  // 墙面太平（<30°）或太陡（>90°），不进入墙跑状态
	}

	if (bIsWallRunning) return;

	if (bIsAutoFromGlide)
	{
		StopGlide();
	}

	// 1. 计算墙跑方向（从UpdateWallRun提取的局部逻辑）
    FVector WallUp = WallDetection->WallNormal;
    if (FMath::Abs(WallUp.Z) > 0.9f)
    {
        WallUp = FVector::CrossProduct(WallUp, GetActorForwardVector()).GetSafeNormal();
    }
    WallUp = FVector::CrossProduct(WallDetection->WallNormal, FVector::CrossProduct(FVector::UpVector, WallUp)).GetSafeNormal();
    FVector WallRight = FVector::CrossProduct(WallUp, -WallDetection->WallNormal).GetSafeNormal();
    FVector WallRunDir = (WallRight * LastMovementInput.X + WallUp * LastMovementInput.Y).GetSafeNormal();

    if (WallRunDir.IsNearlyZero()) return; // 方向无效则不启动

    // 2. 获取当前水平速度（疾跑速度）
    float CurrentSpeed2D = FVector::Dist2D(FVector::ZeroVector, GetCharacterMovement()->Velocity);
    
    // 3. 设置初始速度：至少为WallRunSpeed，如果疾跑更快则保留疾跑速度
    float InitialSpeed = FMath::Max(CurrentSpeed2D, WallRunSpeed);

	bIsWallRunning = true;
	ActionState = EActionState::EAS_WallRunning;
	DashCount = 2;		//重置Dash次数

	PreWallRunGravityScale = GetCharacterMovement()->GravityScale;

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->GravityScale = WallRunGravityScale;

	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInst->bIsWallRunning = true;
	}

	//直接设置初始速度
	GetCharacterMovement()->Velocity = WallRunDir * InitialSpeed;
	GetCharacterMovement()->bConstrainToPlane = false;

	GetCharacterMovement()->MaxFlySpeed = WallRunSpeed * 1.5f;

	BP_OnStartWallRun();
}

void AAlexCharacter::StopWallRun()
{
	if (!bIsWallRunning) return;

	bIsWallRunning = false;
	ActionState = EActionState::EAS_Unoccupied;

	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = PreWallRunGravityScale;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bConstrainToPlane = false;

	LastMovementInput = FVector2D::ZeroVector;
	bHasMovementInput = false;

	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInst->bIsWallRunning = false;
	}

	ResetRun();
}

void AAlexCharacter::UpdateWallRun(float DeltaTime)
{
	if (!bIsWallRunning || !WallDetection) return;

	if (!bHasMovementInput || LastMovementInput.IsNearlyZero())
    {
        StopWallRun();
        return;
    }

	if (!WallDetection->bCanWallRun || 
        WallDetection->ActorToWallDistance > WallRunDetectionDistance * 1.5f ||
        GetCharacterMovement()->MovementMode != MOVE_Flying)
    {
        StopWallRun();
        return;
    }

	FVector WallUp = WallDetection->WallNormal;
	if (FMath::Abs(WallUp.Z) > 0.9f) // 防崩溃：地板/天花板
	{
		WallUp = FVector::CrossProduct(WallUp, GetActorForwardVector()).GetSafeNormal();
	}
	WallUp = FVector::CrossProduct(WallDetection->WallNormal, FVector::CrossProduct(FVector::UpVector, WallUp)).GetSafeNormal();
	FVector WallRight = FVector::CrossProduct(WallUp, -WallDetection->WallNormal).GetSafeNormal();
	if (bHasMovementInput)
	{
		if (LastMovementInput.X > 0.01f)
		{
			WallDetection->RightCapsuleTrace();
			if (WallDetection->GetRightCapsuleHit().bBlockingHit)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Right Wall Hit"));
			}
		}
		else if (LastMovementInput.X < -0.01f)
		{
			WallDetection->LeftCapsuleTrace();
			if (WallDetection->GetLeftCapsuleHit().bBlockingHit)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Left Wall Hit"));
			}
		}

		WallRunDirection = (WallRight * LastMovementInput.X + WallUp * LastMovementInput.Y).GetSafeNormal();
		if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInst->WallRunDirection = WallRunDirection;
		}
	}
	else
	{
		StopWallRun();
		return;
	}

	BP_OnUpdateWallRun(DeltaTime);
}

void AAlexCharacter::StartClimb()
{
	if (!WallDetection || !WallDetection->bCanWallRun)
    {
        return;  // 没有墙，不爬
    }

	if (WallDetection->ActorToWallDistance > ClimbDetectionDistance)
    {
        return;  // 距离不够，不爬
    }

	FVector WallNormal = WallDetection->WallNormal;
    float Angle = FMath::Acos(FVector::DotProduct(WallNormal, FVector::UpVector));
    if (Angle < FMath::DegreesToRadians(30.0f) || Angle > FMath::DegreesToRadians(150.0f))
    {
        return;  // 墙面太平或太陡，不爬
    }

	if (bIsClimbing) return;

	bIsClimbing = true;
	ActionState = EActionState::EAS_Climbing;

	PreClimbGravityScale = GetCharacterMovement()->GravityScale;

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->GravityScale = ClimbGravityScale;

	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInst->bIsClimbing = true;
	}

	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->bConstrainToPlane = false;

	BP_OnStartClimb();
}

void AAlexCharacter::StopClimb()
{
	if (!bIsClimbing) return;

	bIsClimbing = false;
	ActionState = EActionState::EAS_Unoccupied;

	GetCharacterMovement()->GravityScale = PreClimbGravityScale;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->bConstrainToPlane = false;
	LastMovementInput = FVector2D::ZeroVector;
	bHasMovementInput = false;

	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInst->bIsClimbing = false;
	}

	ResetRun();
}

void AAlexCharacter::UpdateClimb(float DeltaTime)
{
	if (!bIsClimbing || !WallDetection) return;
	if (!WallDetection->bCanWallRun)
    {
        StopClimb();  // 自动退出攀爬状态
        return;
    }
	if (WallDetection->ActorToWallDistance > ClimbDetectionDistance * 1.5f) // 允许1.5倍容错
	{
	    StopClimb();
	    return;
	}
	if (GetCharacterMovement()->MovementMode != MOVE_Flying) return;

	FVector WallUp = WallDetection->WallNormal;
	if (FMath::Abs(WallUp.Z) > 0.9f) // 防崩溃：地板/天花板
	{
		WallUp = FVector::CrossProduct(WallUp, GetActorForwardVector()).GetSafeNormal();
	}
	WallUp = FVector::CrossProduct(WallDetection->WallNormal, FVector::CrossProduct(FVector::UpVector, WallUp)).GetSafeNormal();
	FVector WallRight = FVector::CrossProduct(WallUp, -WallDetection->WallNormal).GetSafeNormal();
	if (bHasMovementInput)
	{
		//用于处理左右攀爬时墙壁的碰撞检测（暂时不用）
		/*if (LastMovementInput.X > 0.01f)
		{
			WallDetection->RightCapsuleTrace();
			if (WallDetection->GetRightCapsuleHit().bBlockingHit)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Right Wall Hit"));
			}
		}
		else if (LastMovementInput.X < -0.01f)
		{
			WallDetection->LeftCapsuleTrace();
			if (WallDetection->GetLeftCapsuleHit().bBlockingHit)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Left Wall Hit"));
			}
		}*/

		ClimbDirection = (WallRight * LastMovementInput.X + WallUp * LastMovementInput.Y).GetSafeNormal();
		if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInst->ClimbDirection = ClimbDirection;
		}
	}
	else
	{
		ClimbDirection = FVector::ZeroVector;
	}

	BP_OnUpdateClimb(DeltaTime);
}

void AAlexCharacter::StartGlide()
{
	if (bIsClimbing) return;
	if (!CanGlide()) return;

	ActionState = EActionState::EAS_Gliding;
	bLockMove = false;

	// 播放开始滑翔过渡动画
    if (GlideMontage)
    {
        PlayMontageSection(GlideMontage, FName("Glide"));
    }

	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        AnimInst->bIsGliding = true;
    }

	 // --- 核心：初始化速度方向为当前角色朝向 ---
	FVector CurrentVelocity = GetCharacterMovement()->Velocity;
    float CurrentSpeed = FVector::Dist2D(FVector::ZeroVector, CurrentVelocity);

	// 确保有最低速度
    if (CurrentSpeed < 200.f) CurrentSpeed = 200.f;

	FVector Forward = GetActorForwardVector();
    CurrentVelocity.X = Forward.X * CurrentSpeed;
    CurrentVelocity.Y = Forward.Y * CurrentSpeed;

	GetCharacterMovement()->Velocity = CurrentVelocity;
}

void AAlexCharacter::StopGlide()
{
	if (ActionState != EActionState::EAS_Gliding) return;

	ActionState = EActionState::EAS_Unoccupied;
	CurrentGlideTilt = 0.f;
	LastMovementInput = FVector2D::ZeroVector;

	// 重置动画实例
    if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        AnimInst->bIsGliding = false;
        AnimInst->GlideTilt = 0.f;
    }
	CurrentGlideDescentSpeed = GlideDescentSpeed;// 重置为初始下降速度
}

void AAlexCharacter::UpdateGlide(float DeltaTime)
{
	if (ActionState != EActionState::EAS_Gliding) return;

	// 1. 获取当前水平速度大小
    FVector Velocity = GetCharacterMovement()->Velocity;
    float CurrentHorizontalSpeed = FVector::Dist2D(FVector::ZeroVector, Velocity);

	CurrentGlideDescentSpeed = FMath::Min(CurrentGlideDescentSpeed + (DescentRate * DeltaTime), MaxGlideDescentSpeed);
	// 限制下降速度（缓降）
    Velocity.Z = FMath::Clamp(Velocity.Z, -CurrentGlideDescentSpeed, 0.f);

	// 2. 处理转向输入：直接旋转速度向量，而非Actor
	float RightInput = LastMovementInput.X; // 左右输入，范围[-1, 1]
	if (FMath::Abs(RightInput) > 0.01f)
	{
		// 计算偏航角变化
        float YawDelta = RightInput * GlideTurnSpeed * DeltaTime;

		// 将速度向量绕Z轴旋转（核心：旋转速度而非Actor）
		FVector HorizontalVel = FVector(Velocity.X, Velocity.Y, 0.f);
		if (HorizontalVel.Size() > KINDA_SMALL_NUMBER)
        {
            FRotator YawRot(0.f, YawDelta, 0.f);
            HorizontalVel = YawRot.RotateVector(HorizontalVel);
            
            // 保持水平速度大小不变
            HorizontalVel = HorizontalVel.GetSafeNormal() * CurrentHorizontalSpeed;
        }
		Velocity.X = HorizontalVel.X;
        Velocity.Y = HorizontalVel.Y;

		// 计算平滑的倾斜角度（用于1D混合空间）
		float TargetTilt = RightInput * MaxGlideTiltAngle;
		CurrentGlideTilt = FMath::FInterpTo(CurrentGlideTilt, TargetTilt, DeltaTime, GlideTiltSpeed);
	}
	else
	{
		// 无输入时恢复水平
        CurrentGlideTilt = FMath::FInterpTo(CurrentGlideTilt, 0.f, DeltaTime, GlideTiltSpeed);
	}

	GetCharacterMovement()->Velocity = Velocity;

	// 3. 更新动画参数
	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        AnimInst->GlideTilt = CurrentGlideTilt;
    }

	// 4. 让角色面朝速度方向（被动跟随，非强制旋转）
	// 这确保角色模型看起来在"驾驶"滑翔轨迹
	if (CurrentHorizontalSpeed > KINDA_SMALL_NUMBER)	
	{
		FVector VelocityDir = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
        FRotator TargetRotation = FRotationMatrix::MakeFromX(VelocityDir).Rotator();
        TargetRotation.Pitch = 0.f; // 强制保持水平
        TargetRotation.Roll = 0.f;

		// 平滑插值旋转，避免生硬
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 8.f));
	}

	// 5. 自动退出条件：落地或死亡
	if (!GetCharacterMovement()->IsFalling() || ActionState == EActionState::EAS_Dead)
	{
		StopGlide();
	}
}

bool AAlexCharacter::CanGlide() const
{
	return GetCharacterMovement()->IsFalling() &&
		ActionState != EActionState::EAS_Gliding &&
		ActionState != EActionState::EAS_Dead &&
		!bIsCharging;
}

void AAlexCharacter::StartCharge()
{
	bIsCharging = true;
	CurrentChargeTime = 0.f;

	if (bIsWallRunning || bIsClimbing) return;

	if (HasMovementInput()) return;

	if (GetCharacterMovement()->IsFalling()) return;

	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0, 0, 1));
	bLockMove = true;

	if (ChargeMontage)
	{
		PlayMontageSection(ChargeMontage, FName("Charge"));
	}
}

void AAlexCharacter::ExecuteChargeJump()
{

	float ChargeRatio = CurrentChargeTime / MaxChargeTime;
	float OldJumpZ = GetCharacterMovement()->JumpZVelocity;
	float TargetJumpZ = OldJumpZ * FMath::Lerp(1.f, MaxJumpZMult, ChargeRatio);
	if (GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->Velocity.Z = TargetJumpZ;
	}
	else
	{
		// 否则正常跳跃
		GetCharacterMovement()->JumpZVelocity = TargetJumpZ;
		Super::Jump();
	}
	GetWorld()->GetTimerManager().SetTimerForNextTick([this, OldJumpZ]()
	{
	    GetCharacterMovement()->JumpZVelocity = OldJumpZ;
	});
	if (LaunchMontage)
	    PlayMontageSection(LaunchMontage, FName("Launch"));
	CurrentChargeTime = 0.f;
}

bool AAlexCharacter::ConsumeDashIfAvailable()
{
	if (GetCharacterMovement()->IsFalling())
	{
		if (DashCount > 0)
		{
			DashCount--;
			return true;
		}
	}
	return false;
}

void AAlexCharacter::DashEnd()
{
	GetCharacterMovement()->Velocity *= 0.2f;
}

void AAlexCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	DashCount = 2;

	// 确保停止滑翔
    StopGlide();
	StopClimb();
	OnVaultAnimEnd();

	WallRunJumpCooldown = 0.f;// 重置墙跳冷却时间
}