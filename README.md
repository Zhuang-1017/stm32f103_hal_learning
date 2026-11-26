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
| `[18]EXTI外部中断点灯` | 外部中断：PA5 上升沿触发 **EXTI line 5** → `HAL_GPIO_EXTI_Callback` 中三灯同时翻转 |
| `[19-1]串口发送之发送字节` | USART1 阻塞发送：`HAL_UART_Transmit(&huart1, &ch, 1, HAL_MAX_DELAY)` 每 1 s 发一个字符 `H` |
| `[19-2]串口发送字符串以及printf函数的应用` | 串口发字符串：`HAL_UART_Transmit` + `strlen`；重定向 `fputc` 后 `printf` 直接走 USART1 |
| `[19-3]串口发送之更换脚组` | 换脚验证：改用 **USART2（TX=PA2 / RX=PA3）** 发送，`MX_USART2_UART_Init()` + `fputc` 改 `&huart2`；印证 TX/RX 必须交叉 |
| `[19-4]串口的阻塞接收` | 阻塞接收：`HAL_UART_Receive(&huart1, Recvbuff, 2, HAL_MAX_DELAY)` 等满 2 字节，再用 `HAL_UART_Transmit` 回显；验证阻塞语义（收不满就卡住） |
| `[19-5]串口发送之中断发送方式` | 中断收发：`HAL_UART_Receive_IT` 等满 `LENGTH=6` 字节触发中断 → `HAL_UART_RxCpltCallback` 中用 `HAL_UART_Transmit_IT` 回显并重新使能接收；主循环全空，完全中断驱动 |
| `[19-6]串口发送控制led` | 串口命令控制：逐字节中断接收（每次 1 字节）→ `strncmp` 比对 `"ON"` / `"OFF"` → 点亮 / 熄灭 PA1/PA2/PA3 三路 LED；主循环全空 |
| `[19-7]串口发送之中断接收不定长数据` | **空闲中断接收不定长**：`HAL_UARTEx_ReceiveToIdle_IT(&huart1, RxBuff, 64)` → 一帧结束（IDLE）触发 `HAL_UARTEx_RxEventCallback(huart, Size)`，`Size` 即本帧实际长度；回调中 `HAL_UART_Transmit_IT` 回显并重新使能；与 `[19-5]` 的"等满定长"形成对照 |
| `[19-8]串口之多串口应用` | **双串口互转**：同时开启 USART1（PA9/PA10）与 USART2（PA2/PA3）的空闲中断接收，两路各自独立缓冲区 `RxBuff1` / `RxBuff2`；回调中按 `huart->Instance` 分流，把本路收的帧**转发到另一路**再重开本路接收。要点：转发必须用**阻塞** `HAL_UART_Transmit`——若用 `Transmit_IT`，`gState` 立刻变 `BUSY_TX`，紧随其后的阻塞打印会因首句 `if (gState == READY)` 不成立而整串返回 `HAL_BUSY`，一个字都出不去；且 `_IT` 未发完就重开接收，同一 buffer 会被边发边写覆盖 |
| `[19-10]DMA与串口的应用` | **DMA 收发不定长**：`HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuff, 64)` 启动 DMA 接收，IDLE 触发 `HAL_UARTEx_RxEventCallback`，`Size` 为本帧长度；回调中用 `HAL_UART_Transmit_DMA` 回显并重开接收。USART1_RX 绑 **DMA1_Channel5**、USART1_TX 绑 **DMA1_Channel4**，均为 `DMA_NORMAL`；**两个 DMA 通道中断必须使能**，否则 `UART_DMATransmitCplt` 不执行，`gState` 永远停在 `BUSY_TX`，之后所有 `printf` 都会返回 `HAL_BUSY` |
| `[19-11]自定义通信协议` | **自定义帧协议**：`AA 55` + `LEN` + `CMD` + `DATA[LEN]` + `CRC_L CRC_H`，整帧 = 6 + LEN、LEN ≤ 32；CRC16/MODBUS（`0xA001` / 初值 `0xFFFF` / 低字节先发，`"123456789"` → `0x4B37`）。解析用**逐字节 7 态状态机** `Proto_Parse()`（返回 `0` 未成帧 / `1` 成帧 / `-1` 校验错 / `-2` 长度非法），状态存 `static` → **粘包、半包天然成立，不依赖任何超时**。要点：`ST_HEAD2` 遇到重复 `0xAA` 留在原状态（支持 `AA AA 55` 重叠帧头）、`LEN` 越界立刻丢弃（防越界写）、`ST_CRC_H` 出帧后不论对错都 `Reset`。工程 `doc/` 里附面试讲稿 `自定义协议-面试一页纸.pdf` |
| `[21-1]ADC单通道和电位器的应用` | **ADC 单通道轮询采样**：`MX_ADC1_Init()` 配 ADC1_IN6（**PA6**）单次转换、非扫描、12 位右对齐；上电先 `HAL_ADCEx_Calibration_Start()` 校准一次 → 主循环 `HAL_ADC_Start` + `HAL_ADC_PollForConversion(&hadc1,50)` + `HAL_ADC_GetValue` → `V = ADC × 3.3 / 4095` → printf 每 500 ms 打印。ADCCLK = PCLK2/6 = **12 MHz**（上限 14 MHz）。手边没有电位器时可用两只 10k 电阻分压（取样点 1.65 V → 读数 ≈2048）替代。**采样时间必须从 1.5 cycles 调到 55.5 以上**：1.5 cycles 只允许源阻抗 1.6 kΩ，10k 分压的等效源阻抗 5 kΩ，会让读数系统性偏低 |

## 硬件平台

- **MCU**：STM32F103C8T6（48 脚核心板，LQFP48）
- **板载 LED**：PC13（**低电平点亮**）
- **外接 RGB LED**：R → PA1、G → PA2、B → PA3（高电平点亮）
- **按键**：PA4（内部上拉，另一端接 GND）、PA5（内部下拉，另一端接 **3.3V**）
- **蜂鸣器**：PA6（三脚模块，**低电平触发**）
- **光敏电阻**：PA11（数字量输出）
- **串口 USART1**：TX = PA9、RX = PA10（P4 排针），经 USB-TTL 模块（CH340）连电脑，115200 8N1
- **串口 USART2**：TX = PA2、RX = PA3（P3 排针第 7、8 脚），与 RGB 灯的 G/B 通道**共用引脚**
- 原理图见根目录 `STM32F103C8T6核心板原理图.pdf`

> 接线注意：`[11]`/`[12]` 是「上拉 + 按键接 GND」，`[18]` 是「下拉 + 按键接 3.3V」，两者**极性相反**，照搬会没有反应。
>
> 串口接线注意：USB-TTL 的 **TXD → PA10、RXD → PA9**（必须交叉），**GND 必须共地**，模块 **VCC 不要接到板子**（板子由 ST-Link 供电）。模块电平需为 3.3V。
>
> 用 USART2 时改接 **TXD → PA3（第 8 脚）、RXD → PA2（第 7 脚）**。注意 PA2 是 TX、PA3 是 RX，与模块的 TXD/RXD 顺序相反，**按同名平推会接成 TX-TX，表现为一个字节都收不到**。另需先拔掉 PA2/PA3 上的 RGB 灯线，否则 LED 挂在 UART 线上吃电流。
>
> 跑 `[19-8]` 这类**双串口**实验需要**两个独立的 USB-TTL 模块**（各占电脑一个 USB 口），一路接 PA9/PA10、另一路接 PA2/PA3。只有一个模块时，另一侧没有对端，双向转发无法验证。**切勿**把 PA9/PA10 与 PA2/PA3 互相短接做回环——USART1→USART2→USART1 会形成无限转发。
>
> 跑 `[21-1]` 这类 **ADC** 实验时，**PA6 上不要挂蜂鸣器模块**（`[12]` 用的就是 PA6），否则模块会分走电流、把采集到的电压拉低。`[21-1]` 采集的正是 PA6 = ADC1_IN6。

## 开发环境

- 配置工具：STM32CubeMX 6.18
- 编译 / 下载：Keil MDK-ARM V5.36（ARMCC V5.06）
- 固件库：STM32 HAL（STM32F1xx HAL Driver，FW_F1 V1.8.7）

## 如何编译

用 Keil 打开各工程 `MDK-ARM/` 目录下的 `*.uvprojx`，编译并下载到板子即可。
（工程内文件名沿用 CubeMX 生成时的名字，如 `[18]` 目录下是 `19.uvprojx`、`[19-2]` 目录下是 `19-2.uvprojx`、`[19-3]` 目录下是 `19-3.uvprojx`、`[19-4]` 目录下是 `19-4.uvprojx`、`[19-5]` 目录下是 `19-5.uvprojx`、`[19-6]` 目录下是 `19-6.uvprojx`、`[19-7]` 目录下是 `19-7.uvprojx`、`[19-8]` 目录下是 `19-8.uvprojx`、`[19-10]` 目录下是 `19-10.uvprojx`、`[19-11]` 目录下是 `19-11.uvprojx`、`[21-1]` 目录下是 `21-1.uvprojx`，与目录名不必一致。）

## 目录说明

- `Core/`：应用代码（CubeMX 生成骨架 + 用户代码，用户代码写在 `USER CODE BEGIN/END` 之间）
- `Drivers/`：HAL 库与 CMSIS，随仓库提交以保证 clone 下来可直接编译
- `MDK-ARM/`：Keil 工程文件（`*.uvprojx` / `*.uvoptx` 提交）
- `EWARM/`：CubeMX 一并生成的 IAR 工程文件（本仓库不用，保留备查）
- `doc/`：个人实验笔记

> 构建产物（`MDK-ARM/<工程名>/` 输出目录、`*.o`、`*.axf`、`*.hex`、`*.lst`、`*.uvguix.*`、`DebugConfig/` 等）已由 `.gitignore` 排除。

## 编码约定

C 源文件的中文注释统一使用 **UTF-8 with BOM**：Keil MDK 默认识别 GBK，加 BOM 后可正确识别 UTF-8，同时 GitHub 网页端不会乱码。
