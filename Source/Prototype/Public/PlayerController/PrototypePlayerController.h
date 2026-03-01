// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PrototypePlayerController.generated.h"

class ULockOnManager;
class UInputAction;
class UInputMappingContext;
class UUserWidget;

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

	//死亡与重生
	UFUNCTION(BlueprintCallable, Category="Death")
	void ShowDeathWidget();

	UFUNCTION(BlueprintCallable, Category="Death")
	void HideDeathWidget();

	UFUNCTION(BlueprintCallable, Category="Death")
	void OnRespawnButtonClicked();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<UUserWidget> DeathWidgetClass;

	UPROPERTY()
	UUserWidget* DeathWidgetInstance;
};
