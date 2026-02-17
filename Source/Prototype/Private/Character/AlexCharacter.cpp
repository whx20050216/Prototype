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
#include "HUD/HealthWidget.h"
#include "HUD/FormWheelWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/Enemy.h"
#include "AbilitySystemComponent.h"
#include "GAS/AlexAttributeSet.h"
#include "GAS/Input/InputConfig.h"
#include "GAS/AbilitySet.h"
#include "Items/WeaponActor.h"

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

	LockPriority = 0.f;

	// GAS
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);	//开启网络复制（即使单机也建议开，因为GAS很多功能依赖这个开关）
	AttributeSet = CreateDefaultSubobject<UAlexAttributeSet>(TEXT("AttributeSet"));
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
	if (bRunActive) TryAutoVault();

	//奔跑或滑翔下自动检测并启动墙跑
	bool bShouldCheckWallRun = (bRunActive || ActionState == EActionState::EAS_Gliding);
	if (!bIsWallRunning && !bIsVaulting && WallDetection->bCanWallRun && bShouldCheckWallRun && WallRunJumpCooldown <= 0.f)
	{
		StartWallRun();
	}

	//处理攀爬逻辑
	if (bIsClimbing) UpdateClimb(DeltaTime);

	//处理墙跑逻辑
	if (bIsWallRunning) UpdateWallRun(DeltaTime);

	//处理打断急停惯性逻辑
	InterruptSkid();

	//锁定系统更新
	if (bIsSelectingTarget)
	{
		UpdateTargetSelection();
	}
	else if (LockedTarget)
	{
		UpdateLockOnCamera(DeltaTime);
	}

	//更新锁定UI
	UpdateLockOnUI();

	//轮盘高亮
	if (FormWheelWidget && FormWheelWidget->IsInViewport())
    {
        UpdateMorphWheelSelection();
    }
}

void AAlexCharacter::ResetRun()
{
	bool bHasNow = HasMovementInput();
	bool bJustPressed = bHasNow && !bHadInputLastFrame;
	bool bJustReleased = !bHasNow && bHadInputLastFrame;

	if (bRunActive && (bJustPressed || bJustReleased))
	{
		// 检查速度是否够快
		float Speed = GetCharacterMovement()->Velocity.Size2D();
		if (Speed > 600.f && !bIsWallRunning && !bIsClimbing && !bIsVaulting)
		{
			PlaySkidMontage();
		}

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
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &AAlexCharacter::CLIMB);
		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AAlexCharacter::TAB_Pressed);
		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Completed, this, &AAlexCharacter::TAB_Released);
		EnhancedInputComponent->BindAction(MorphWheelAction, ETriggerEvent::Started, this, &AAlexCharacter::ShowMorphWheel);
		EnhancedInputComponent->BindAction(MorphWheelAction, ETriggerEvent::Completed, this, &AAlexCharacter::HideMorphWheelAndConfirm);

		// === GAS Ability 输入改为通过 InputConfig 绑定 ===
		if (InputConfig)
		{
			for (const FAbilityInputBinding& Binding : InputConfig->AbilityInputBindings)
			{
				if (Binding.InputAction && Binding.InputTag.IsValid())
				{
					// Pressed
					EnhancedInputComponent->BindAction(
						Binding.InputAction,
						ETriggerEvent::Started,
						this,
						&AAlexCharacter::OnAbilityInputPressed,
						Binding.InputTag
					);
					// Released（用于需要持续按住的技能，如蓄力）
					EnhancedInputComponent->BindAction(
						Binding.InputAction,
						ETriggerEvent::Completed,
						this,
						&AAlexCharacter::OnAbilityInputReleased,
						Binding.InputTag
					);
				}
			}
		}
	}
}

void AAlexCharacter::SetOverlappingItem(AItem* Item)
{
	OverlappingItem = Item;
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

	if (HealthWidgetClass)  // 检查蓝图是否指定了类
	{
	    HealthWidget = CreateWidget<UHealthWidget>(GetWorld(), HealthWidgetClass);
	    if (HealthWidget)
	    {
	        HealthWidget->AddToViewport();
	        
	        if (AttributeSet)
	        {
	            HealthWidget->BindToAttributeSet(AttributeSet);
	        }
	    }
	}

	if (LockOnReticleClass)
	{
		HoverWidget = CreateWidget<UUserWidget>(GetWorld(), LockOnReticleClass);
		if (HoverWidget)
		{
			HoverWidget->AddToViewport();
			HoverWidget->SetVisibility(ESlateVisibility::Hidden);  // 初始隐藏，等检测到敌人再显示
		}
	}

	if (LockOnLockedClass)
	{
		LockedWidget = CreateWidget<UUserWidget>(GetWorld(), LockOnLockedClass);
		if (LockedWidget)
		{
			LockedWidget->AddToViewport();
			LockedWidget->SetVisibility(ESlateVisibility::Hidden);  // 初始隐藏
		}
	}

	UnlockForm(FGameplayTag::RequestGameplayTag(FName("Form.Claw")));
    UnlockForm(FGameplayTag::RequestGameplayTag(FName("Form.Whip")));
    UnlockForm(FGameplayTag::RequestGameplayTag(FName("Form.Hammer")));
    UnlockForm(FGameplayTag::RequestGameplayTag(FName("Form.Blade")));
    UnlockForm(FGameplayTag::RequestGameplayTag(FName("Form.Default")));
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
	if (bIsRoofFlipping) return;

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
	if (bIsRoofFlipping) return;

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
	if (!AbilitySystemComponent) return;

	if (AttributeSet)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> DASH ATTEMPT | State: %s | Charges: %.0f"), 
            *UEnum::GetValueAsString(ActionState),
            AttributeSet->GetDashCharges());  // 如果还没加这个变量，先删掉这个参数
    }

	bool bWasGliding = (ActionState == EActionState::EAS_Gliding);

	FGameplayTagContainer TagContainer;
    TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Dash")));
    
    // 复数形式 Abilities，参数是 Container
    bool bSuccess = AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);

	UE_LOG(LogTemp, Warning, TEXT(">>> DASH RESULT: %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

	if (bSuccess && bWasGliding)
	{
		StopGlide();
	}
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

void AAlexCharacter::ATTACK()
{
    if (!AbilitySystemComponent) return;
    
    // 检查是否已有激活的 Attack Ability（用于连击）
    for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack"))))
        {
            if (Spec.IsActive())
            {
                // 已经激活，发送输入用于连击
                AbilitySystemComponent->AbilitySpecInputPressed(Spec);
                return;
            }
            // 找到了但未激活，不要return，跳出去激活它
            break;
        }
    }
    
    // 没有激活的 Attack Ability，尝试激活
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack")));
    AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
}

void AAlexCharacter::TAB_Pressed()
{
	if (LockedTarget)
	{
		// 已经锁定，再按 Tab 取消锁定
		CancelLockOn();
	}
	else
	{
		// 进入选择模式
		bIsSelectingTarget = true;
		HoveredTarget = nullptr;
	}
}

void AAlexCharacter::TAB_Released()
{
	if (bIsSelectingTarget)
	{
		bIsSelectingTarget = false;
		ConfirmLockOn();
	}
}

void AAlexCharacter::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (!AbilitySystemComponent || !InputTag.IsValid()) return;

	// 1. 先检查是否已有对应Tag的Ability正在运行（连击关键！）
	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		// 检查DynamicAbilityTags（我们在GiveAbility时绑定的InputTag）
		if (Spec.DynamicAbilityTags.HasTag(InputTag) && Spec.IsActive())
		{
			// 通知已激活的Ability：输入又按下了（GA_AlexAttack会用它触发下一段）
			AbilitySystemComponent->AbilitySpecInputPressed(Spec);
			return;
		}
	}

	// 2. 尝试激活对应 Tag 的 Ability
	FGameplayTagContainer TagContainer(InputTag);
	AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
}

void AAlexCharacter::OnAbilityInputReleased(FGameplayTag InputTag)
{	// 暂时没有蓄力技能，有则开放
	//if (!AbilitySystemComponent || !InputTag.IsValid()) return;

	// 遍历所有可激活的 Ability，找到匹配 InputTag 且正在运行的，通知它输入释放了
    /*for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
        if (Spec.DynamicAbilityTags.HasTag(InputTag) && Spec.IsActive())
		{
            AbilitySystemComponent->AbilitySpecInputReleased(Spec);
        }
    }*/
}

void AAlexCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// 授予默认形态的技能组（替代硬编码Give Dash/Attack）
	FGameplayTag DefaultTag = FGameplayTag::RequestGameplayTag(FName("Form.Default"));
	SwitchToForm(DefaultTag);
}

void AAlexCharacter::ShowMorphWheel()
{
	if (!FormWheelWidgetClass) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ShowMorphWheel"));

	// 创建并显示轮盘
	FormWheelWidget = CreateWidget<UFormWheelWidget>(GetWorld(), FormWheelWidgetClass);
	if (FormWheelWidget)
	{
		// 传入已解锁的形态
		TArray<FGameplayTag> AvailableTags;
		for (const FGameplayTag& Tag : UnlockedFormTags)
		{
			AvailableTags.Add(Tag);
		}
		FormWheelWidget->SetupWheel(AvailableTags);
		FormWheelWidget->AddToViewport();

		// 立即更新一次鼠标位置（显示高亮）
		if(APlayerController* PC=Cast<APlayerController>(GetController()))
		{
			FVector2D MousePos, ViewportSize;
			PC->GetMousePosition(MousePos.X, MousePos.Y);
			GEngine->GameViewport->GetViewportSize(ViewportSize);
			FormWheelWidget->UpdateMousePosition(MousePos, ViewportSize * 0.5f);

			// ========== 切换输入模式 ==========
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(FormWheelWidget->TakeWidget());  // 焦点给UI
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
		}

		// 子弹时间（大时缓）
		UGameplayStatics::SetGlobalTimeDilation(this, 0.1f);

		// 显示鼠标光标
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->bShowMouseCursor = true;
		}

	}
}

void AAlexCharacter::HideMorphWheelAndConfirm()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("HideMorphWheelAndConfirm"));

	// 恢复正常时间
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);

	// 隐藏鼠标光标
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		// ========== 切回游戏模式 ==========
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	if (FormWheelWidget)
	{
		// 获取选择的形态
		FGameplayTag SelectedTag = FormWheelWidget->GetSelectedFormTag();
		
		// 移除UI
		FormWheelWidget->RemoveFromParent();
		FormWheelWidget = nullptr;
		
		// 执行切换（如果不是当前形态）
		if (SelectedTag.IsValid() && SelectedTag != CurrentFormTag)
		{
			SwitchToForm(SelectedTag);  // 你之前写的带特效的切换
		}
	}
}


void AAlexCharacter::UpdateMorphWheelSelection()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        FVector2D MousePos, ViewportSize;
        PC->GetMousePosition(MousePos.X, MousePos.Y);
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        FormWheelWidget->UpdateMousePosition(MousePos, ViewportSize * 0.5f);

		UE_LOG(LogTemp, Warning, TEXT("Angle Updated, CurrentForm: %s"), 
            *FormWheelWidget->GetSelectedFormTag().ToString());
    }
}

void AAlexCharacter::UnlockForm(FGameplayTag FormTag)
{
	UnlockedFormTags.Add(FormTag);
}

void AAlexCharacter::AttachWeaponForForm(FGameplayTag FormTag)
{
	// 加这行！
	UE_LOG(LogTemp, Error, TEXT(">>> AttachWeaponForForm ENTRY! FormTag: %s"), *FormTag.ToString());
	// 1. 先卸下旧武器
	DetachCurrentWeapon();

	// 2. 获取新形态配置
	const FMorphConfig& Config = GetCurrentMorphConfig();
	UE_LOG(LogTemp, Error, TEXT(">>> Config ptr: %p"), &Config);

	// 3. 如果该形态没有配置武器（如拳形态），直接返回（空手）
	if (!Config.WeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("No WeaponClass configured for %s"), *FormTag.ToString());
		return;
	}

	// 4. 生成新武器
	FActorSpawnParameters Params;
	Params.Owner = this;
	CurrentWeapon = GetWorld()->SpawnActor<AWeaponActor>(Config.WeaponClass, Params);

	if (CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon Spawned: %s"), *CurrentWeapon->GetName());
		// 5. 附加到右手骨骼插槽（默认骨骼名是 "hand_r"，如果你的不同请改）
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		CurrentWeapon->AttachToComponent(GetMesh(), AttachRules, FName("hand_r"));

		// 关键：打印附加后的位置
		UE_LOG(LogTemp, Warning, TEXT("Attached to %s, Location: %s"), 
			*GetMesh()->GetName(), 
			*CurrentWeapon->GetActorLocation().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn weapon!"));
	}
}

void AAlexCharacter::DetachCurrentWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
}

void AAlexCharacter::SwitchToForm(FGameplayTag NewFormTag)
{
	UE_LOG(LogTemp, Error, TEXT(">>> SwitchToForm Called! Target: %s"), *NewFormTag.ToString());
	if (!AbilitySystemComponent || !FormAbilitySets.Contains(NewFormTag)) return;
	if (NewFormTag == CurrentFormTag) return;

	// 1. 如果正在攻击，先停止
	if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Attacking"))))
	{
		StopAllMontages();
	}
	ResetAttackCombo();

	// 2. 移除当前形态的所有技能
	if (UAbilitySet* CurrentSet = FormAbilitySets.FindRef(CurrentFormTag))
	{
		CurrentSet->RemoveFromAbilitySystem(AbilitySystemComponent, CurrentFormAbilityHandles);
	}
	CurrentFormAbilityHandles.Empty();

	// 3. 授予新形态的技能组
	if (UAbilitySet* NewSet = FormAbilitySets.FindRef(NewFormTag))
	{
		NewSet->GiveToAbilitySystem(AbilitySystemComponent, CurrentFormAbilityHandles);
	}

	// 4. 更新形态Tag（用于GA_AlexAttack查询当前攻击蒙太奇）
	if (CurrentFormTag.IsValid())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(CurrentFormTag, 0);
	}
	AbilitySystemComponent->SetLooseGameplayTagCount(NewFormTag, 1);
	CurrentFormTag = NewFormTag;

	AttachWeaponForForm(NewFormTag);
}

const FMorphConfig& AAlexCharacter::GetCurrentMorphConfig() const
{
	if (const FMorphConfig* Config = FormMorphConfigs.Find(CurrentFormTag))
    {
        return *Config;
    }

	// 保险：返回空配置避免崩溃
	static FMorphConfig EmptyConfig;
	return EmptyConfig;
}

void AAlexCharacter::PlayFootstepSound()
{
	FVector Start = GetActorLocation();
    FVector End = Start - FVector(0, 0, 100);

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1, 0, 3.f);

	FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			// 遍历配置的Tag-Sound映射，找到匹配就播放
			for (const auto& Pair : FootstepSounds)
			{
				if (HitActor->ActorHasTag(Pair.Key))
				{
					if (Pair.Value)
					{
						UGameplayStatics::PlaySoundAtLocation(this, Pair.Value, Hit.ImpactPoint);
					}
					return;
				}
			}

			// 没有匹配到特定Tag，播放默认
			if (DefaultFootstepSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, DefaultFootstepSound, Hit.ImpactPoint);
			}
		}
	}
}

void AAlexCharacter::UpdateLockOnUI()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 更新悬停框
	UpdateTargetWidget(HoveredTarget, HoverWidget, PC);

	// 更新锁定框
	UpdateTargetWidget(LockedTarget, LockedWidget, PC);
}

void AAlexCharacter::UpdateTargetWidget(AActor* Target, UUserWidget* Widget, APlayerController* PC)
{
	if (!Target || !Widget)
	{
		if (Widget) Widget->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// 获取目标在屏幕上的包围盒
	FVector2D PixelSize, ScreenCenter;
	if (GetTargetScreenPixelSize(Target, PC, PixelSize, ScreenCenter))
	{
		// 加边距
        FVector2D TargetSize(PixelSize.X + 20.f, PixelSize.Y + 20.f);

		// 钳制最大最小（防止极端情况）
        TargetSize.X = FMath::Clamp(TargetSize.X, 40.f, 400.f);
        TargetSize.Y = FMath::Clamp(TargetSize.Y, 60.f, 500.f);

		// 关键：强制 Pivot 为左上角 (0,0)，确保 SetPositionInViewport 设置的就是左上角位置
		Widget->SetRenderTransformPivot(FVector2D(0.f, 0.f));
		
		// 关键：手动计算左上角位置（中心点减去半尺寸）
		FVector2D TopLeftPosition(
			ScreenCenter.X - (TargetSize.X / 2.f),
			ScreenCenter.Y - (TargetSize.Y / 2.f)
		);
		
		Widget->SetPositionInViewport(TopLeftPosition, true);

		// 缩放
		FVector2D OriginalSize(100.f, 100.f);
		Widget->SetRenderScale(FVector2D(TargetSize.X / OriginalSize.X, TargetSize.Y / OriginalSize.Y));

		Widget->SetVisibility(ESlateVisibility::Visible);

	}
	else
	{
		Widget->SetVisibility(ESlateVisibility::Hidden);
	}
}

bool AAlexCharacter::GetTargetScreenPixelSize(AActor* Target, APlayerController* PC, FVector2D& OutSize, FVector2D& OutCenter)
{
	if (!Target || !PC) return false;

	// 获取胶囊体组件（人形敌人的标准碰撞）
    UCapsuleComponent* Capsule = Target->FindComponentByClass<UCapsuleComponent>();
    if (!Capsule)
    {
        // 回退到包围盒逻辑
		FVector2D Min, Max;
		if (GetTargetScreenBounds(Target, PC, Min, Max))
		{
			OutSize = Max - Min;
			OutCenter = (Min + Max) / 2.f;  // 包围盒中心
			return true;
		}
		return false;
    }

	// 计算头顶和脚底的世界坐标
	FVector ActorLocation = Target->GetActorLocation();
    float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    float Radius = Capsule->GetScaledCapsuleRadius();

	// 定义5个关键点：中心、顶、底、左、右
	FVector Center = ActorLocation;
	FVector Top = ActorLocation + FVector(0, 0, HalfHeight);
	FVector Bottom = ActorLocation - FVector(0, 0, HalfHeight);
	FVector Left = ActorLocation - FVector(0, Radius, 0);
	FVector Right = ActorLocation + FVector(0, Radius, 0);


	// 投影到屏幕
	FVector2D ScreenCenter, ScreenTop, ScreenBottom, ScreenLeft, ScreenRight;

    // 投影所有点（中心必须成功）
	if (!PC->ProjectWorldLocationToScreen(Center, ScreenCenter)) return false;
	if (!PC->ProjectWorldLocationToScreen(Top, ScreenTop)) return false;
	if (!PC->ProjectWorldLocationToScreen(Bottom, ScreenBottom)) return false;

	// 1. 计算高度（直接用Top/Bottom的Y差值，这个相对准确）
	float HeightPixels = FMath::Abs(ScreenTop.Y - ScreenBottom.Y);

	// 2. 计算宽度和水平中心
	float WidthPixels = 0.f;
	if (PC->ProjectWorldLocationToScreen(Left, ScreenLeft) && PC->ProjectWorldLocationToScreen(Right, ScreenRight))
	{
		// 分别投影左右点，取中点作为真正的水平中心
		WidthPixels = FMath::Abs(ScreenRight.X - ScreenLeft.X);
		ScreenCenter.X = (ScreenLeft.X + ScreenRight.X) / 2.f;  // 修正水平偏移
	}
	else
	{
		// 回退：基于中心点估算（远距离 fallback）
		FVector2D ScreenRightOnly;
		if (PC->ProjectWorldLocationToScreen(Right, ScreenRightOnly))
		{
			WidthPixels = FMath::Abs(ScreenRightOnly.X - ScreenCenter.X) * 2.f;
		}
		else
		{
			WidthPixels = HeightPixels * 0.5f;
		}
	}

	// 3. 距离补偿：太近时手动修正（透视畸变修正）
    float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
    if (Distance < 300.f) // 小于3米
    {
        // 近距离时，投影计算会偏大，需要压缩
        float CompressRatio = FMath::Lerp(0.6f, 1.0f, Distance / 300.f);
        WidthPixels *= CompressRatio;
        HeightPixels *= CompressRatio;
    }
    
    OutSize = FVector2D(WidthPixels, HeightPixels);
    OutCenter = ScreenCenter;
    return true;
}

// 计算目标在屏幕上的包围盒
bool AAlexCharacter::GetTargetScreenBounds(AActor* Target, APlayerController* PC, FVector2D& OutMin, FVector2D& OutMax)
{
	if (!Target || !PC) return false;

	// 获取目标的世界空间包围盒
	FVector Origin, BoxExtent;
	Target->GetActorBounds(true, Origin, BoxExtent);

	// 8个角点
	FVector Corners[8];
    Corners[0] = Origin + FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    Corners[1] = Origin + FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z);
    Corners[2] = Origin + FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z);
    Corners[3] = Origin + FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);
    Corners[4] = Origin + FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    Corners[5] = Origin + FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z);
    Corners[6] = Origin + FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z);
    Corners[7] = Origin + FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);

	// 投影到屏幕，找最小/最大
	bool bFirstValid = true;
    FVector2D MinPoint, MaxPoint;

	for (int32 i = 0; i < 8; i++)
	{
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(Corners[i], ScreenPos))
		{
			if (bFirstValid)
			{
				MinPoint = MaxPoint = ScreenPos;
				bFirstValid = false;
			}
			else
			{
				MinPoint.X = FMath::Min(MinPoint.X, ScreenPos.X);
                MinPoint.Y = FMath::Min(MinPoint.Y, ScreenPos.Y);
                MaxPoint.X = FMath::Max(MaxPoint.X, ScreenPos.X);
                MaxPoint.Y = FMath::Max(MaxPoint.Y, ScreenPos.Y);
			}
		}
	}

	if (bFirstValid) return false; // 所有点都在屏幕外

	OutMin = MinPoint;
    OutMax = MaxPoint;
    return true;
}

void AAlexCharacter::UpdateTargetSelection()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 获取屏幕中心位置（Viewport 中心）
	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	FVector2D ScreenCenter(ViewportSizeX / 2, ViewportSizeY / 2);

	// 屏幕中心转世界射线
	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection))
	{
		return;
	}
	
	FRotator DirectionRot = WorldDirection.Rotation();
    DirectionRot.Pitch += 15.f;  // 负数是向上（UE坐标系：Pitch正=抬头）
    FVector AdjustedDirection = DirectionRot.Vector();

	FVector TraceStart = WorldLocation;
	FVector TraceEnd = WorldLocation + (AdjustedDirection  * LockOnRange);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 只检测实现了 ITargetableInterface 的 Actor（敌人等）
    // 用 ECC_Visibility 或自定义 ECC_GameTraceChannel
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(LockOnSphereRadius),
		Params
	);

	AActor* NewHoveredTarget = nullptr;
	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (Hit.GetActor() && Hit.GetActor()->Implements<UTargetableInterface>())
			{
				float Priority = ITargetableInterface::Execute_GetLockPriority(Hit.GetActor());
				if (Priority > 0.f)
				{
					NewHoveredTarget = Hit.GetActor();
					break;
				}
			}
		}
	}

	// 悬停目标变化
	if (NewHoveredTarget != HoveredTarget)
	{
		HoveredTarget = NewHoveredTarget;
	}
}

void AAlexCharacter::ConfirmLockOn()
{
	if (HoveredTarget)
	{
		LockedTarget = HoveredTarget;
		HoveredTarget = nullptr;

		// 通知目标被锁定（让敌人显示标记等）
		ITargetableInterface::Execute_OnLocked(LockedTarget, GetController());
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
            FString::Printf(TEXT("LockedTarget: %s"), *LockedTarget->GetName()));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::Printf(TEXT("No Target")));
	}
}

void AAlexCharacter::CancelLockOn()
{
	if (LockedTarget)
	{
		ITargetableInterface::Execute_OnUnlocked(LockedTarget, GetController());
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, 
            FString::Printf(TEXT("UnLocked: %s"), *LockedTarget->GetName()));
		LockedTarget = nullptr;
	}
}

void AAlexCharacter::UpdateLockOnCamera(float DeltaTime)
{
	if (!LockedTarget || !GetController()) return;

	// 获取目标位置（接口获取，带高度偏移）
	FVector TargetLocation = ITargetableInterface::Execute_GetTargetLocation(LockedTarget);
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.f;	// 保持水平，不仰头

	if (!ToTarget.IsNearlyZero())
	{
		// 计算目标旋转（Yaw  only）
		FRotator TargetRotation = ToTarget.GetSafeNormal().Rotation();
		TargetRotation.Pitch = 0.f;
		TargetRotation.Roll = 0.f;


		FRotator CurrentRotation = SpringArm->GetComponentRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, CameraRotationSpeed);
		SpringArm->SetWorldRotation(NewRotation);
	}

	// 检查目标是否死亡/超出距离，自动解除锁定
    float Distance = FVector::Distance(GetActorLocation(), LockedTarget->GetActorLocation());
    if (Distance > LockOnRange * 1.5f)  // 超出 1.5 倍距离自动解除
    {
        CancelLockOn();
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

bool AAlexCharacter::CheckApproachingRoof()
{
	if (bIsRoofFlipping) return false;

	if (!WallDetection) return false;

	WallDetection->UpperTrace();

	// 必须是向上移动（玩家按住W/上）
    // 如果 LastMovementInput.Y < 0.1f（阈值可调），说明没想往上，不触发
	if (LastMovementInput.Y < 0.1f) return false;

	// 获取三个检测点状态
	const FHitResult& TopHit = WallDetection->GetTopHit();
	const FHitResult& MiddleHit = WallDetection->GetMiddleHit();
	const FHitResult& BottomHit = WallDetection->GetBottomHit();
	const FHitResult& UpperHit = WallDetection->GetUpperHit();

	// 核心判断：顶射线和中射线检测不到了，但下还能检测到
    // 说明角色头部已经超出楼顶，但身体还在墙面上（正在翻越的瞬间）
	return !TopHit.bBlockingHit
		&& MiddleHit.bBlockingHit
		&& BottomHit.bBlockingHit
		&& !UpperHit.bBlockingHit;
}

void AAlexCharacter::PerformRoofFlip()
{
	if (!RoofFlipMontage || bIsRoofFlipping) return;

	bIsRoofFlipping = true;
	bIsWallRunning = false;
	ActionState = EActionState::EAS_Unoccupied;

	FVector Forward = GetActorForwardVector();
	FVector LaunchVel = Forward * 600.f + FVector::UpVector * 400.f;

	LaunchCharacter(LaunchVel, true, true);

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	PlayAnimation(RoofFlipMontage);

	FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AAlexCharacter::OnRoofFlipEnded);
    GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, RoofFlipMontage);
}

void AAlexCharacter::OnRoofFlipEnded(UAnimMontage* Montage, bool bInterrupted)
{
	GetCharacterMovement()->GravityScale = PreWallRunGravityScale;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	if (GetCharacterMovement()->IsMovingOnGround())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}

	LastMovementInput = FVector2D::ZeroVector;
    bHasMovementInput = false;

	if (AbilitySystemComponent)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(
            FGameplayTag::RequestGameplayTag(FName("State.WallRunning")));
    }

	UE_LOG(LogTemp, Warning, TEXT(">>> ROOFFLIP ENDED | bInterrupted: %d | State: %s"), 
        bInterrupted,
        *UEnum::GetValueAsString(ActionState));
	if (AttributeSet) 
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> Resetting Dash from RoofFlipEnd"));
        AttributeSet->SetDashCharges(AttributeSet->GetMaxDashCharges());
    }
	bIsRoofFlipping = false;
}

void AAlexCharacter::PerformWallRunBackFlip()
{
	if (ActionState == EActionState::EAS_WallRunFlipping) return;

	if (!WallRunBackFlipMontage)
	{
		StopWallRun();  // 没有动画就普通停止墙跑
		return;
	}

	bIsWallRunning = false;
	ActionState = EActionState::EAS_WallRunFlipping;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);// 根运动需要 Flying 模式

	PlayAnimation(WallRunBackFlipMontage);

	FVector LaunchDir = (WallDetection->WallNormal).GetSafeNormal();
	FVector LaunchVel = LaunchDir * 300.f;
	LaunchCharacter(LaunchVel, true, true);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AAlexCharacter::OnWallRunBackFlipEnded);
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, WallRunBackFlipMontage);

	if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInst->bIsWallRunning = false;
	}

	// 设置冷却，防止立即重新贴墙
	WallRunJumpCooldown = WallRunJumpCooldownDuration;
}

void AAlexCharacter::OnWallRunBackFlipEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ActionState = EActionState::EAS_Unoccupied;

	// 恢复重力设置
	GetCharacterMovement()->GravityScale = PreWallRunGravityScale;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	if (GetCharacterMovement()->IsMovingOnGround())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}

	LastMovementInput = FVector2D::ZeroVector;
	bHasMovementInput = false;
}

void AAlexCharacter::StartWallRun()
{
	if (bIsRoofFlipping) return;

	bool bIsAutoFromGlide = (ActionState == EActionState::EAS_Gliding);

	if (LastMovementInput.Y < -0.5f) return;

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

	// 如果从攀爬切换，先保存输入再停止攀爬
	if (bIsClimbing)
	{
		FVector2D SavedInput = LastMovementInput;
		StopClimb();
		// 恢复输入，否则 WallRunDir 计算会失败
		LastMovementInput = SavedInput;
		bHasMovementInput = !SavedInput.IsNearlyZero();
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.WallRunning")));
	}
	//重置Dash次数
	if (AttributeSet) AttributeSet->SetDashCharges(AttributeSet->GetMaxDashCharges());

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

	if (AbilitySystemComponent)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.WallRunning")));
    }

	ResetRun();
}

void AAlexCharacter::UpdateWallRun(float DeltaTime)
{
	if (!bIsWallRunning || !WallDetection) return;

	if (LastMovementInput.Y < -0.5f)
	{
		PerformWallRunBackFlip();
		return;
	}

	// 预判楼顶
	if (CheckApproachingRoof())
	{
		PerformRoofFlip();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Approaching Roof")));
		return;
	}

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
		if (!WallRunDirection.IsNearlyZero())
		{
			GetCharacterMovement()->Velocity = WallRunDirection * WallRunSpeed;
		}
	}
	else
	{
		StopWallRun();
		return;
	}
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

	if (AbilitySystemComponent)
    {
        FGameplayTag ClimbTag = FGameplayTag::RequestGameplayTag(FName("State.Climbing"));
        if (ClimbTag.IsValid())
        {
            AbilitySystemComponent->AddLooseGameplayTag(ClimbTag);
            UE_LOG(LogTemp, Warning, TEXT("Tag added successfully: %s"), *ClimbTag.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("State.Climbing Tag is INVALID! Check DT_GameplayTags"));
        }
    }

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

	if (AbilitySystemComponent)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Climbing")));
    }

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
	if (ClimbMantleMontage && GetMesh()->GetAnimInstance()->Montage_IsPlaying(ClimbMantleMontage))
    {
        return;
    }
	if (CheckClimbMantle())
	{
		PerformClimbMantle();
		return;
	}
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
		GetCharacterMovement()->Velocity = ClimbDirection * ClimbSpeed;
		if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInst->ClimbHorizontal = LastMovementInput.X;
			AnimInst->ClimbVertical = LastMovementInput.Y;
		}
	}
	else
	{
		ClimbDirection = FVector::ZeroVector;
		GetCharacterMovement()->Velocity = ClimbDirection;
		if (UAlexAnimInstance* AnimInst = Cast<UAlexAnimInstance>(GetMesh()->GetAnimInstance()))
		{
		    AnimInst->ClimbHorizontal = 0.0f;
		    AnimInst->ClimbVertical = 0.0f;
		}
	}

	//BP_OnUpdateClimb(DeltaTime);
}

bool AAlexCharacter::CheckClimbMantle()
{
	if (!WallDetection || !bIsClimbing) return false;

	if (LastMovementInput.Y < 0.1f) return false;

	WallDetection->UpperTrace();

	const FHitResult& UpperHit = WallDetection->GetUpperHit();
	const FHitResult& TopHit = WallDetection->GetTopHit();
	const FHitResult& MiddleHit = WallDetection->GetMiddleHit();

	return !UpperHit.bBlockingHit && !TopHit.bBlockingHit && MiddleHit.bBlockingHit;
}

void AAlexCharacter::PerformClimbMantle()
{
	if (!ClimbMantleMontage) return;

	LastMovementInput = FVector2D::ZeroVector;
    bHasMovementInput = false;

	PlayAnimation(ClimbMantleMontage);

	FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AAlexCharacter::OnClimbMantleEnded);
    GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, ClimbMantleMontage);
}

void AAlexCharacter::OnClimbMantleEnded(UAnimMontage* Montage, bool bInterrupted)
{
	StopClimb();
	if (AbilitySystemComponent)
	{
	    AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Climbing")));
	}
}

void AAlexCharacter::StartGlide()
{
	if (bIsClimbing) return;
	if (!CanGlide()) return;
	if (bIsRoofFlipping) return;

	ActionState = EActionState::EAS_Gliding;
	bLockMove = false;

	if (AbilitySystemComponent)
    {
        AbilitySystemComponent->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Gliding")));
    }

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

	if (AbilitySystemComponent)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Gliding")));
    }

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
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 8.0f));
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

void AAlexCharacter::PlaySkidMontage()
{
	if (!SkidMontage) return;
	if (!bRunActive) return;
	if (GetCharacterMovement()->IsFalling()) return;  // 跳跃/下落中
	if (GetCharacterMovement()->MovementMode != MOVE_Walking) return;// 不是行走模式
	if (bIsVaulting) return;  // 翻越中
	if (ActionState == EActionState::EAS_Gliding) return;  // 滑翔中
	if (bIsWallRunning) return;  // 墙跑中（如果你用这个标记）
	if (bIsClimbing) return;  // 攀爬中

	float Speed = GetCharacterMovement()->Velocity.Size2D();
	if (Speed < 600.f) return; 

	bIsSkidding = true;
	PlayAnimation(SkidMontage);
	// 设置减速参数
    GetCharacterMovement()->BrakingDecelerationWalking = 100.f; // 增大刹车力度（默认是0或很小）
    GetCharacterMovement()->GroundFriction = 0.8f; // 增大地面摩擦（默认8，可以调到2-3让滑得更远）
}

void AAlexCharacter::InterruptSkid()
{
	if (!bIsSkidding) return;

	float CurrentSpeed = GetCharacterMovement()->Velocity.Size2D();
	if (CurrentSpeed < 300.f) // 阈值可调，200 约等于走路速度
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Interrupt Skid")));
		if (GetMesh()->GetAnimInstance()->Montage_IsPlaying(SkidMontage))
		{
			GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, SkidMontage); // 0.2秒融合，不生硬
		}
		
		bIsSkidding = false;
		GetCharacterMovement()->GroundFriction = 8.f;
		GetCharacterMovement()->BrakingDecelerationWalking = 0.f;
		return; // 提前返回，不再检查其他条件
	}

	// 如果玩家又按了移动键（且有一定输入量）
    if (bHasMovementInput && LastMovementInput.Size() > 0.1f)
    {
        // 立即停止急停蒙太奇（BlendOut时间0.2秒，不要太长）
        if (GetMesh()->GetAnimInstance()->Montage_IsPlaying(SkidMontage))
        {
            GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, SkidMontage);
        }
        
        bIsSkidding = false;
        
        // 恢复正常摩擦（如果之前改了）
        GetCharacterMovement()->GroundFriction = 8.f; // 默认值
        GetCharacterMovement()->BrakingDecelerationWalking = 0.f; // 默认值
    }

	// 如果动画播完了（自然停下）
    if (!GetMesh()->GetAnimInstance()->Montage_IsPlaying(SkidMontage))
    {
        bIsSkidding = false;
        // 恢复默认摩擦
        GetCharacterMovement()->GroundFriction = 8.f;
		GetCharacterMovement()->BrakingDecelerationWalking = 0.f;
    }
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

float AAlexCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.f || !AbilitySystemComponent || !DamageEffectClass) return 0.f;

	// 1. 创建 Effect Context（记录伤害来源，用于后续判定）
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddInstigator(EventInstigator, DamageCauser); // 记录凶手和武器

	// 2. 创建 Spec（Level 这里传 1，我们用 SetByCaller 传具体值）
	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
	if (Spec.IsValid())
	{
		// 3. 设置伤害值（负数，因为 GE 是 "Add" 到 Health）
        // DataTag 必须和 GE 蓝图里配的一致
		FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
		Spec.Data->SetSetByCallerMagnitude(DamageTag, -ActualDamage);

		// 4. 应用（Apply）
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data);

		// 5. 立即检查死亡（因为 Instant GE 已经执行完毕）
		if (AttributeSet && AttributeSet->GetHealth() <= 0.f)
		{
			OnCharacterDeath.Broadcast(this);
		}
	}

	return ActualDamage;
}

void AAlexCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	UE_LOG(LogTemp, Warning, TEXT(">>> LANDED TRIGGERED | State: %s"), 
        *UEnum::GetValueAsString(ActionState));

	if (AttributeSet) AttributeSet->SetDashCharges(AttributeSet->GetMaxDashCharges());

	// 确保停止滑翔
    StopGlide();
	StopClimb();
	OnVaultAnimEnd();

	WallRunJumpCooldown = 0.f;// 重置墙跳冷却时间
}