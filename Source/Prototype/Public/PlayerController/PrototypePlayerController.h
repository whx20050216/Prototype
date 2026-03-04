// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PrototypePlayerController.generated.h"

class ULockOnManager;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UAudioComponent;
class USoundBase;

UCLASS()
class PROTOTYPE_API APrototypePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	//锁定管理器
	UPROPERTY(VisibleAnywhere, Category="LockOn")
    ULockOnManager* LockOnManager;

	//输入函数
	UFUNCTION(BlueprintCallable, Category="LockOn")
	void Input_ToggleLockOn();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputMappingContext* DefaultMappingContext;

	//输入Action变量
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LockOnAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PauseAction;

	//死亡与重生
	UFUNCTION(BlueprintCallable, Category="Death")
	void ShowDeathWidget();

	UFUNCTION(BlueprintCallable, Category="Death")
	void HideDeathWidget();

	UFUNCTION(BlueprintCallable, Category="Death")
	void OnRespawnButtonClicked();

	// 菜单控制
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable)
	void QuitGame();

	UFUNCTION(BlueprintCallable)
	void TogglePauseMenu();	// 暂停菜单

	UPROPERTY(BlueprintReadOnly)
	bool bIsPaused = false;

	UPROPERTY(BlueprintReadOnly)
	bool bInMainMenu = false;	// true=主菜单, false=游戏中

	UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> MenuWidgetClass;

	// 菜单背景音乐
	UPROPERTY()
	UAudioComponent* MenuMusic;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* MainMenuMusic;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<UUserWidget> DeathWidgetClass;

	UPROPERTY()
	UUserWidget* DeathWidgetInstance;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> MenuWidget;
};
