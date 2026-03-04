// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/PrototypePlayerController.h"
#include "PlayerController/LockOnManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "PrototypeGameMode.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

void APrototypePlayerController::BeginPlay()
{
	Super::BeginPlay();

	LockOnManager = NewObject<ULockOnManager>(this, TEXT("LockOnManager"));

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }

	ShowMainMenu();
}

void APrototypePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &APrototypePlayerController::Input_ToggleLockOn);
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &APrototypePlayerController::TogglePauseMenu);
	}
}

void APrototypePlayerController::Input_ToggleLockOn()
{
	if (LockOnManager)
	{
		LockOnManager->ToggleLockOn();
	}
}

void APrototypePlayerController::ShowDeathWidget()
{
	if (DeathWidgetClass && !DeathWidgetInstance)
	{
		DeathWidgetInstance = CreateWidget<UUserWidget>(this, DeathWidgetClass);
		if (DeathWidgetInstance)
		{
			DeathWidgetInstance->AddToViewport();
			// 显示鼠标
			SetInputMode(FInputModeUIOnly());
			bShowMouseCursor = true;
		}
	}
}

void APrototypePlayerController::HideDeathWidget()
{
	if (DeathWidgetInstance)
	{
		DeathWidgetInstance->RemoveFromParent();
		DeathWidgetInstance = nullptr;

		// 恢复游戏
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void APrototypePlayerController::OnRespawnButtonClicked()
{
	HideDeathWidget();

	if (APrototypeGameMode* GameMode = Cast<APrototypeGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->RespawnPlayer();
	}
}


void APrototypePlayerController::ShowMainMenu()
{
	if (!MenuWidgetClass || MenuWidget) return;

	MenuWidget = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (MenuWidget)
	{
		bInMainMenu = true;
		bIsPaused = true;

		// UI模式
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(MenuWidget->TakeWidget());
		SetInputMode(Mode);
		bShowMouseCursor = true;

		MenuWidget->AddToViewport(100);
		SetPause(true);

		if (MainMenuMusic)
		{
			// 如果已有组件且正在播放，先停止
			if (MenuMusic && MenuMusic->IsPlaying())
			{
				MenuMusic->Stop();
			}
			// 创建新的音频组件（循环播放）
			MenuMusic = UGameplayStatics::SpawnSound2D(
				this,
				MainMenuMusic,
				1.f,
				1.f,
				0.f,
				nullptr,
				true
			);
			// 淡入效果（2秒）
			if (MenuMusic)
			{
				MenuMusic->FadeIn(2.f, 1.f);
			}
		}
	}
}

void APrototypePlayerController::StartGame()
{
	if (!MenuWidget) return;

	MenuWidget->RemoveFromParent();
	MenuWidget = nullptr;

	// 游戏模式
	FInputModeGameOnly InputModeGame;
	SetInputMode(InputModeGame);
	bShowMouseCursor = false;
	SetPause(false);

	bInMainMenu = false;
	bIsPaused = false;

	// 淡出音乐（2秒淡出后自动停止）
	if (MenuMusic && MenuMusic->IsPlaying())
	{
		MenuMusic->FadeOut(2.f, 0.f);
		MenuMusic = nullptr;
	}
}

void APrototypePlayerController::ReturnToMainMenu()
{
	// 从暂停状态返回主菜单
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	bIsPaused = false;
	SetPause(false);
	if (APrototypeGameMode* GameMode = Cast<APrototypeGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->RespawnPlayer();
	}
}

void APrototypePlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void APrototypePlayerController::TogglePauseMenu()
{
	// 主菜单状态下按ESC无效（防止误关）
	if (bInMainMenu) return;
	
	if (!bIsPaused)
	{
		// 打开暂停菜单
		if (!MenuWidget && MenuWidgetClass)
		{
			MenuWidget = CreateWidget<UUserWidget>(this, MenuWidgetClass);
		}
		if (MenuWidget)
		{
			bIsPaused = true;
			bInMainMenu = false;
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(MenuWidget->TakeWidget());
			SetInputMode(Mode);
			bShowMouseCursor = true;
	
			MenuWidget->AddToViewport(100);
			SetPause(true);
		}
	}
	else
	{
		// 关闭暂停菜单
		if (MenuWidget)
		{
			MenuWidget->RemoveFromParent();
			MenuWidget = nullptr;
		}
		FInputModeGameOnly GameMode;
		SetInputMode(GameMode);
		bShowMouseCursor = false;
		SetPause(false);
		bIsPaused = false;
	}
}