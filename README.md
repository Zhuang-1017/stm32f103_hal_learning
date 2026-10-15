# STM32F103C8T6 学习工程（野火 HAL 库教程）

跟随野火《7 天入门 STM32》HAL 版教程的逐章实验工程，硬件为 **STM32F103C8T6 核心板**。

## 实验列表

| 目录 | 对应章节 | 内容 |
| --- | --- | --- |
| `[8]cube32新建工程` | 第 8 章 | 用 CubeMX 新建 MDK 工程、配置时钟（HSE×9 → 72 MHz）、编译下载验证 |
| `[9]GPIO_output点灯` | 第 9 章 | GPIO 推挽输出配置：PC13（板载灯）+ PA1/PA2/PA3（外接 RGB） |
| `[10]delay流水灯` | 第 10 章 | 软件延时 `HAL_Delay` + RGB 三色流水灯 |

## 硬件平台

- **MCU**：STM32F103C8T6（48 脚核心板）
- **板载 LED**：PC13（低电平点亮）
- **外接 RGB LED**：R → PA1、G → PA2、B → PA3
- 原理图见根目录 `STM32F103C8T6核心板原理图.pdf`

## 开发环境

- 配置工具：STM32CubeMX
- 编译 / 下载：Keil MDK-ARM
- 固件库：STM32 HAL（STM32F1xx）

## 如何编译

用 Keil 打开各工程 `MDK-ARM/` 目录下的 `*.uvprojx`，编译并下载到板子即可。

## 目录说明

- `Core/`：应用代码（CubeMX 生成骨架 + 用户代码）
- `Drivers/`：HAL 库与 CMSIS，随仓库提交以保证 clone 下来可直接编译
- `MDK-ARM/`：Keil 工程文件（`*.uvprojx` / `*.uvoptx` 提交）

> 构建产物（Objects 输出目录、`*.lst`、`*.uvguix`、`DebugConfig/` 等）已由 `.gitignore` 排除。
