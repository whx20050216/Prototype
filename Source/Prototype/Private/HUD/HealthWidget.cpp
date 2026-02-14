// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/HealthWidget.h"
#include "GAS/AlexAttributeSet.h"
#include "ActorComponent/AttributeComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHealthWidget::BindToAttributeSet(UAlexAttributeSet* AttributeSet)
{
    if (!AttributeSet) return;

    BoundAttributeSet = AttributeSet;

    // 绑定 GAS 委托（FSimpleDelegate 无参数）
    AttributeSet->OnHealthChanged.BindUObject(this, &UHealthWidget::OnHealthChangedGAS);
    AttributeSet->OnManaChanged.BindUObject(this, &UHealthWidget::OnManaChangedGAS);

	// 立即更新初始值
	UpdateHealthDisplay(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
    UpdateManaDisplay(AttributeSet->GetMana(), AttributeSet->GetMaxMana());
}

void UHealthWidget::BindToAttributeComponent(UAttributeComponent* AttributeComp)
{
	if (!AttributeComp) return;

    BoundAttributeComp = AttributeComp;

    // 绑定动态委托（AddDynamic 需要 UFUNCTION）
    AttributeComp->OnHealthChanged.AddDynamic(this, &UHealthWidget::OnHealthChanged);
    AttributeComp->OnManaChanged.AddDynamic(this, &UHealthWidget::OnManaChanged);
    
    // 立即更新初始值
    UpdateHealthDisplay(AttributeComp->GetHealth(), AttributeComp->GetMaxHealth());
    UpdateManaDisplay(AttributeComp->GetMana(), AttributeComp->GetMaxMana());
}

void UHealthWidget::OnHealthChangedGAS()
{
    if (BoundAttributeSet)
    {
        UpdateHealthDisplay(BoundAttributeSet->GetHealth(), BoundAttributeSet->GetMaxHealth());
    }
}

void UHealthWidget::OnManaChangedGAS()
{
    if (BoundAttributeSet)
    {
        UpdateManaDisplay(BoundAttributeSet->GetMana(), BoundAttributeSet->GetMaxMana());
    }
}

void UHealthWidget::OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta)
{
	UpdateHealthDisplay(CurrentHealth, MaxHealth);
}

void UHealthWidget::OnManaChanged(float CurrentMana, float MaxMana, float Delta)
{
	UpdateManaDisplay(CurrentMana, MaxMana);
}

void UHealthWidget::UpdateHealthDisplay(float CurrentHealth, float MaxHealth)
{
	if (Bar_Health)
	{
		Bar_Health->SetPercent(CurrentHealth / MaxHealth);
	}
	if (Text_Health)
    {
            Text_Health->SetText(FText::Format(
            INVTEXT("{0} / {1}"),
            FMath::CeilToInt(CurrentHealth),
            FMath::CeilToInt(MaxHealth)
        ));
    }
}

void UHealthWidget::UpdateManaDisplay(float CurrentMana, float MaxMana)
{
	if (Bar_Mana)
	{
		Bar_Mana->SetPercent(CurrentMana / MaxMana);
	}
	if (Text_Mana)
    {
            Text_Mana->SetText(FText::Format(
            INVTEXT("{0} / {1}"),
            FMath::CeilToInt(CurrentMana),
            FMath::CeilToInt(MaxMana)
        ));
    }
}

void UHealthWidget::NativeDestruct()
{
	if (BoundAttributeSet)
    {
        BoundAttributeSet->OnHealthChanged.Unbind();
        BoundAttributeSet->OnManaChanged.Unbind();
        BoundAttributeSet = nullptr;
    }

	if (BoundAttributeComp)
    {
        BoundAttributeComp->OnHealthChanged.RemoveDynamic(this, &UHealthWidget::OnHealthChanged);
        BoundAttributeComp->OnManaChanged.RemoveDynamic(this, &UHealthWidget::OnManaChanged);
        BoundAttributeComp = nullptr;
    }

    Super::NativeDestruct();
}
