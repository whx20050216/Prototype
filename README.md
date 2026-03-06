```markdown
# Prototype - UE5 第三人称动作游戏原型

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.x-blue.svg)](https://www.unrealengine.com/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Gameplay Ability System](https://img.shields.io/badge/GAS-Gameplay%20Ability%20System-orange.svg)](https://docs.unrealengine.com/5.0/en-US/gameplay-ability-system-for-unreal-engine/)
```

> 基于 **虚幻引擎5** 与 **Gameplay Ability System (GAS)** 构建的高机动性第三人称动作游戏技术原型，融合《虐杀原形》式跑酷与多形态战斗系统。

## 项目规模
- 语言：UE5 C++ (不含蓝图)
- 代码量：约9,000行（Source目录，49个代码文件，约6,700行有效代码）
- 核心模块：GAS战斗系统、Parkour移动、AI行为树、UI交互

## 🎮 核心特性

### 1. 高机动性跑酷系统 (Parkour System)
- **自动翻越 (Vault)**：基于检测组件的障碍物高度计算，支持连续翻越与动态起跳速度
- **墙跑 (Wall Run)**：多射线检测墙面法线与距离，支持左右跑、垂直攀爬与屋顶前空翻
- **滑翔 (Glide)**：物理-based 滑翔系统，带倾斜角度计算与转向速度控制
- **惯性急停 (Skid)**：高速奔跑时输入反向的物理滑行动画

### 2. GAS 驱动的战斗系统
- **多形态武器系统**：5种近战形态（拳/爪/刀/鞭/锤），每种拥有独立的攻击范围、角度与连招段数
- **动态连击 (Combo System)**：基于动画通知的输入窗口期，支持预输入缓冲与连击中断恢复
- **远程武器**：M4（全自动）与 RPG（单发/范围伤害）双武器系统，含换弹逻辑与弹药管理
- **暗杀机制 (Assassination)**：蹲下状态下从背后触发处决动画，同步播放双方蒙太奇

### 3. 智能 AI 警觉系统
- **三层状态机**：Idle（待机）→ Suspicious（警觉累积黄条）→ Alert（红条战斗状态）
- **视野检测**：基于 PawnSensing 与射线检测的视野遮挡判断
- **群体警觉广播**：Alert 状态敌人自动唤醒 30m 内同类型敌人
- **受击反应**：受击时暂停行为树播放受击动画，恢复后自动进入 Alert

### 4. 软锁定与 UI 系统
- **软锁定 (Soft Lock-on)**：Tab 键屏幕中心锥形检测，支持动态 UI 框体随目标大小缩放
- **形态轮盘 (Morph Wheel)**：按住 R 触发子弹时间，鼠标方位选择 5 种形态
- **自适应 HUD**：血条/耐力条、弹药显示（当前/后备）、警觉进度条

## 🏗️ 技术架构

```### 代码结构
Source/Prototype/
├── ActorComponent/
│   ├── AttributeComponent.h      # 传统属性组件（血量/蓝条）
│   └── WallDetectionComponent.h  # 跑酷检测：上/中/下三线射线检测
├── Character/
│   ├── AlexCharacter.h           # 主角：输入处理、状态机、GAS集成
│   ├── AlexAnimInstance.h        # 动画蓝图：速度/倾斜角/攀爬参数
│   ├── BaseCharacter.h           # 基础角色：蒙太奇播放接口
│   └── CharacterTypes.h          # 枚举：动作状态/检测类型/形态类型
├── Enemy/
│   ├── Enemy.h                   # AI：警觉系统、攻击配置、受击反应
│   ├── EnemyAIController.h       # AI控制器：黑板管理、行为树驱动
│   └── PatrolPath.h              # 巡逻路径：样条线同步编辑
├── GAS/
│   ├── Abilities/
│   │   ├── GA_AlexAttack.h       # 近战攻击：连击窗口、伤害检测
│   │   ├── GA_RangeAttack.h      # 远程攻击：全自动/单发逻辑
│   │   └── GA_Dash.h             # 冲刺：空中Dash与标签管理
│   ├── Input/
│   │   ├── InputConfig.h         # 输入配置数据资产
│   │   └── AbilitySet.h          # 形态技能组（用于切换形态时授予/移除技能）
│   └── AlexAttributeSet.h        # GAS属性：生命/耐力/Dash次数/弹药
├── HUD/
│   ├── HealthWidget.h            # 血条/耐力条绑定
│   ├── AmmoWidget.h              # 弹药显示与换弹提示
│   └── FormWheelWidget.h         # 形态轮盘：角度计算与扇形区域判定
├── Items/
│   ├── WeaponActor.h             # 武器基类：拾取/装备/丢弃/发射
│   └── Projectile.h              # 投射物：子弹与RPG爆炸伤害
└── PlayerController/
├── PrototypePlayerController.h # 菜单控制、暂停、死亡重生
└── LockOnManager.h           # 锁定管理器：目标扫描与切换
```
### 关键技术实现

#### 1. GAS 架构设计
- **AttributeSet**：使用 `UAlexAttributeSet` 管理生命、耐力、Dash 次数、弹药（子弹/火箭弹）
- **AbilitySet**：每种形态（Form）配置独立的 `UAbilitySet` 数据资产，切换形态时动态授予/移除技能
- **Input Binding**：通过 `InputConfig` 数据资产将增强输入动作与 GameplayTag 绑定，支持运行时输入映射

#### 2. 连击系统实现 (GA_AlexAttack)
```cpp
// 核心机制：动画通知触发窗口期
void OpenComboWindow();    // AnimNotify_OpenComboWindow 触发
void TryAdvanceCombo();    // 检查输入缓冲与下一段存在性
void PerformDamageCheck(); // 扇形/球形检测 + 角度判定
```

#### 3. 跑酷检测算法 (WallDetectionComponent)
- **三线检测**：Bottom（脚）、Middle（胸）、Top（头）分别射线检测
- **法线评估**：计算墙面法线与角色前向量的夹角，确保墙面垂直度与方向一致性
- **高度计算**：UpToDown 垂直射线计算障碍物顶部高度，用于 Vault 起跳速度公式：
  ```cpp
  VerticalVel = Sqrt(2 * Gravity * Height) * VaultVerticalMultiplier;
  ```

#### 4. AI 感知与状态同步
- **黑板同步**：`EnemyAIController` 将 `EAIState` 同步到 Blackboard，驱动 BehaviorTree 分支选择
- **警觉条计算**：`UpdateSuspicion()` 中根据视野与距离线性插值增减警觉值，同步到 UI ProgressBar

## 🎯 操作指南

| 按键 | 功能 | 备注 |
|------|------|------|
| **WASD** | 移动 | 支持八向移动与惯性急停 |
| **右键** | 奔跑 | 1500 速度，支持自动翻越障碍物 |
| **Space** | 跳跃/蓄力 | 按住蓄力（1.2s 满），松开跳跃；空中二次按下进入滑翔 |
| **C** | 蹲下 | 降低移速，背后靠近敌人按 **F** 触发暗杀 |
| **Shift** | 冲刺 | 空中前冲 |
| **Tab** | 锁定目标 | 屏幕中心锥形检测，再次按下取消 |
| **R** | 形态轮盘 | 按住进入子弹时间，鼠标选择形态后松开确认 |
| **F** | 拾取/丢弃 | 靠近武器拾取，空手时丢弃当前武器 |
| **左键** | 攻击 | 近战：连击系统；持枪：射击（M4 全自动/RPG 单发） |
| **Esc** | 菜单 | 主菜单/暂停菜单切换 |

## 🚀 快速开始

### 环境要求
- Unreal Engine 5.4+
- Visual Studio 2022 (C++17)
- 插件：**GameplayAbilities**、**Niagara**、**EnhancedInput**

### 构建步骤
1. 克隆仓库
   ```bash
   git clone https://github.com/whx20050216/Prototype.git
   ```
2. 生成 Visual Studio 项目文件（右键 `.uproject` → Generate Project Files）
3. 打开 `Prototype.sln`，编译配置选择 **Development Editor**
4. 启动项目，在编辑器中打开 `TestMap` 关卡进行测试

### 关键蓝图配置（需在编辑器中完成）
- **形态配置**：在 `AlexCharacter` 蓝图中配置 `FormMorphConfigs` 映射，为每种形态指定：
  - 武器类（`WeaponClass`）
  - 连招蒙太奇（`ComboMontage`）
  - 攻击参数（半径、角度、伤害）
- **输入映射**：创建 `IA_Attack`、`IA_Dash` 等增强输入动作，并在 `InputConfig` 数据资产中绑定对应 GameplayTag
- **行为树**：为 `Enemy` 创建行为树，使用黑板键 `AIState`、`TargetPlayer`、`SuspicionLevel`

## 📸 系统截图（建议补充）
- [ ] 形态轮盘 UI 截图
- [ ] 跑酷（墙跑+翻越）连续动作演示
- [ ] AI 警觉条（黄→红）渐变效果
- [ ] 软锁定 UI 框体动态缩放

## 🛠️ 待优化项
- [ ] 网络复制：当前 GAS 配置为单机模式，需完善网络同步（`SetIsReplicated` 与 Server RPC）
- [ ] 存档系统：已实现 `PrototypeSaveGame` 基础结构，需补充完整关卡进度存储
- [ ] 武器 Mesh 插槽：不同形态武器需在 Blender 中统一骨骼原点，避免运行时偏移
- [ ] GA_Dash功能概率失效

## 🙏 致谢
- 动画资源：[Mixamo](https://www.mixamo.com/)（基础角色动画）、Epic Games 官方
- 音效/特效：Epic Games 官方
- 架构参考：Unreal Engine Gameplay Ability System 官方文档

---
**开发者**: [@whx20050216](https://github.com/whx20050216)  
**项目状态**: 活跃开发中（技术原型阶段）
```
