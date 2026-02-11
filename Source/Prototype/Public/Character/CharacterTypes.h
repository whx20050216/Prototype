#pragma once
UENUM(BlueprintType)//角色动作状态枚举
enum class EActionState : uint8
{
	EAS_Walk UMETA(DisplayName = "Walk"),
	EAS_Run UMETA(DisplayName = "Run"),
	EAS_Vaulting UMETA(DisplayName = "Vaulting"),
	EAS_WallRunFlipping UMETA(DisplayName = "WallRunFlipping"),
	EAS_WallRunning UMETA(DisplayName = "WallRunning"),
	EAS_Climbing UMETA(DisplayName = "Climbing"),
	EAS_Gliding UMETA(DisplayName = "Gliding"),
	EAS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	EAS_HitReaction UMETA(DisplayName = "HitReaction"),
	EAS_Attacking UMETA(DisplayName = "Attacking"),
	EAS_EquippingWeapon UMETA(DisplayName = "Equipping Weapon"),
	EAS_Dodge UMETA(DisplayName = "Dodge"),
	EAS_Dead UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class EDetectionResult : uint8
{
	Clear UMETA(DisplayName = "畅通"),                    // 无障碍
	WallRunPossible UMETA(DisplayName = "可墙跑"),        // 高墙
	VaultPossible UMETA(DisplayName = "可翻越"),          // 矮障碍
	Invalid UMETA(DisplayName = "无效")                    // 不满足条件
};

UENUM(BlueprintType)
enum class EMorphType : uint8
{
	EMT_Fist        UMETA(DisplayName = "Fist"),        // 拳
    EMT_Claw        UMETA(DisplayName = "Claw"),        // 爪
    EMT_Blade       UMETA(DisplayName = "Blade"),       // 刀
    EMT_Whip        UMETA(DisplayName = "Whip"),        // 鞭
    EMT_Hammer      UMETA(DisplayName = "Hammer")       // 锤
};