# STM32F103C8T6 学习工程（野火 HAL 库教程）

跟随野火《7 天入门 STM32》HAL 版教程的逐章实验工程，硬件为 **STM32F103C8T6 核心板**。

## 实验列表

| 目录 | 内容 |
| --- | --- |
| `[8]cube32新建工程` | 用 CubeMX 新建 MDK 工程、配置时钟（HSE 8 MHz × PLL9 → 72 MHz）、编译下载验证 |
| `[9]GPIO_output点灯` | GPIO 推挽输出：点亮板载 LED（PC13，低电平点亮） |
| `[10]delay流水灯` | 软件延时 `HAL_Delay` + RGB 三色流水灯（PA1 / PA2 / PA3） |
| `[11]GPIO_intput控制LED翻转` | GPIO 输入：按键 PA4（内部上拉，接 GND）→ 20 ms 消抖 + 等松手 → 翻转 PC13 |
| `[12]按键触发蜂鸣器` | 按键 PA4 检测 → 低电平触发蜂鸣器 PA6，鸣响 1 s |
| `[13]光敏电阻控制外界LED` | 光敏电阻数字量输入 PA11 → 控制外接 LED（PB5）通断 |
| `[17]系统计数器和中断点灯` | SysTick 系统时基中断中翻转 RGB，验证中断驱动 |
| `[19]EXTI外部中断点灯` | 外部中断：PA5 上升沿触发 **EXTI line 5** → `HAL_GPIO_EXTI_Callback` 中三灯同时翻转 |

## 硬件平台

- **MCU**：STM32F103C8T6（48 脚核心板，LQFP48）
- **板载 LED**：PC13（**低电平点亮**）
- **外接 RGB LED**：R → PA1、G → PA2、B → PA3（高电平点亮）
- **按键**：PA4（内部上拉，另一端接 GND）、PA5（内部下拉，另一端接 **3.3V**）
- **蜂鸣器**：PA6（三脚模块，**低电平触发**）
- **光敏电阻**：PA11（数字量输出）
- 原理图见根目录 `STM32F103C8T6核心板原理图.pdf`

> 接线注意：`[11]`/`[12]` 是「上拉 + 按键接 GND」，`[19]` 是「下拉 + 按键接 3.3V」，两者**极性相反**，照搬会没有反应。

## 开发环境

- 配置工具：STM32CubeMX 6.18
- 编译 / 下载：Keil MDK-ARM V5.36（ARMCC V5.06）
- 固件库：STM32 HAL（STM32F1xx HAL Driver，FW_F1 V1.8.7）

## 如何编译

用 Keil 打开各工程 `MDK-ARM/` 目录下的 `*.uvprojx`，编译并下载到板子即可。
（工程内文件名沿用 CubeMX 生成时的名字，如 `[19]` 目录下是 `19.uvprojx`，与目录名不必一致。）

## 目录说明

- `Core/`：应用代码（CubeMX 生成骨架 + 用户代码，用户代码写在 `USER CODE BEGIN/END` 之间）
- `Drivers/`：HAL 库与 CMSIS，随仓库提交以保证 clone 下来可直接编译
- `MDK-ARM/`：Keil 工程文件（`*.uvprojx` / `*.uvoptx` 提交）
- `EWARM/`：CubeMX 一并生成的 IAR 工程文件（本仓库不用，保留备查）
- `doc/`：个人实验笔记

> 构建产物（`MDK-ARM/<工程名>/` 输出目录、`*.o`、`*.axf`、`*.hex`、`*.lst`、`*.uvguix.*`、`DebugConfig/` 等）已由 `.gitignore` 排除。

## 编码约定

C 源文件的中文注释统一使用 **UTF-8 with BOM**：Keil MDK 默认识别 GBK，加 BOM 后可正确识别 UTF-8，同时 GitHub 网页端不会乱码。
