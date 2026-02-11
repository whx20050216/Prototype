// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPE_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttributeComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //血量变化委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth, float, Delta);

    //蓝条变化委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnManaChanged, float, CurrentMana, float, MaxMana, float, Delta);

    //死亡委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

	UPROPERTY(BlueprintAssignable, Category="Attributes")
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category="Attributes")
    FOnManaChanged OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category="Attributes")
    FOnDeath OnDeath;

	// 血量变化接口
    UFUNCTION(BlueprintCallable, Category="Attributes")
    void ApplyHealthChange(float Delta);

    // 蓝条变化接口
    UFUNCTION(BlueprintCallable, Category="Attributes")
    void ApplyManaChange(float Delta);

	// 获取当前血量（供UI读取）
    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetMaxHealth() const { return MaxHealth; }

    // 获取当前蓝条（供UI读取）
    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetMana() const { return Mana; }

    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetMaxMana() const { return MaxMana; }

    UFUNCTION(BlueprintPure, Category="Attributes")
    bool IsDead() const { return Health <= 0.f; }
    
protected:
	virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attributes")
    float Health = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attributes")
    float MaxHealth = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attributes")
    float Mana = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attributes")
    float MaxMana = 100.f;

};
