// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/FormWheelWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

void UFormWheelWidget::SetupWheel(const TArray<FGameplayTag>& AvailableForms)
{
	AvailableFormSet.Empty();
	for (const FGameplayTag& Tag : AvailableForms)
	{
		AvailableFormSet.Add(Tag);
	}

	// 默认选中第一个可用的
	for (const FGameplayTag& Form : FormOrder)
	{
		if (AvailableFormSet.Contains(Form))
		{
			CurrentSelectedForm = Form;
			break;
		}
	}
}

void UFormWheelWidget::UpdateMousePosition(const FVector2D& MouseScreenPos, const FVector2D& ScreenCenter)
{
	// 计算鼠标相对屏幕中心的角度
	FVector2D Delta = MouseScreenPos - ScreenCenter;

	// ========== 屏幕Y向下，数学Y向上，取反 ==========
	Delta.Y = -Delta.Y;

	// Atan2返回弧度：X右Y上，范围-PI到PI
	// 转成角度，0=右，逆时针增加
	float AngleRad = FMath::Atan2(Delta.Y, Delta.X);
	float AngleDeg = FMath::RadiansToDegrees(AngleRad);

	// 转成0-360度，0=右，顺时针增加（更符合直觉）
	// 或者保持数学角度，看你UI怎么画扇形
	if (AngleDeg < 0) AngleDeg += 360.0f;

	// 注意：这里的角度逻辑要和你的扇形UI对应
	// 简单方案：把360度分成5份，每份72度
	CurrentSelectedForm = GetFormByAngle(AngleDeg);
}

bool UFormWheelWidget::IsFormAvailable(FGameplayTag FormTag) const
{
	return AvailableFormSet.Contains(FormTag);
}

FGameplayTag UFormWheelWidget::GetFormByAngle(float AngleDegrees) const
{
    // 标准化到 0-360
    if (AngleDegrees < 0) AngleDegrees += 360.0f;
    
    FGameplayTag Selected;
    
    // 按你定义的区间判断（注意爪跨越了0°）
    if (AngleDegrees >= 342.0f || AngleDegrees < 54.0f)
    {
        Selected = FormOrder[0]; // 爪
    }
    else if (AngleDegrees >= 54.0f && AngleDegrees < 126.0f)
    {
        Selected = FormOrder[1]; // 拳
    }
    else if (AngleDegrees >= 126.0f && AngleDegrees < 198.0f)
    {
        Selected = FormOrder[2]; // 刃
    }
    else if (AngleDegrees >= 198.0f && AngleDegrees < 270.0f)
    {
        Selected = FormOrder[3]; // 锤
    }
    else // 270°~342°
    {
        Selected = FormOrder[4]; // 鞭
    }
    
    // 检查是否解锁，未解锁则保持当前
    if (AvailableFormSet.Contains(Selected))
    {
        return Selected;
    }
    
    return CurrentSelectedForm;
}
