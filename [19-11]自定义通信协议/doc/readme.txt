2026.11.24
自定义通信协议（帧头 + 长度 + CRC16）

【为什么需要协议】
串口硬件只保证电平时序：把每个字节拆成"起始位 + 8 数据位 + 停止位"按波特率推上线，
接收端按时钟采样还原。它不提供三样东西：
  边界   —— 连发 100 个字节，收到的就是一个数组，里面没有"组"的信息
  完整性 —— 8N1 的 N 就是无校验；就算开奇偶校验，也只保护单个字节、只查奇数个比特错
  应答   —— 线断了、对端没上电，你只会"收不到"，不会知道为什么
所以真实产品都在字节流上叠一层自定义帧。这一层就是协议。

【帧格式】
  AA 55 | LEN | CMD | DATA[LEN] | CRC_L CRC_H
  帧头     长度   命令    数据段        校验

  - 整帧 = 6 + LEN；LEN 上限 32；LEN = 0 时是最短的 6 字节纯命令帧
  - CRC 覆盖 LEN + CMD + DATA，不含两个帧头（帧头只负责同步，算它没有额外价值）
  - CRC 低字节先发（Modbus 惯例）。算出来是 0x4B37 就发 37 4B，别按大端发
  - CRC16/MODBUS：多项式 0xA001（0x8005 的按位镜像，因为这里是右移）、
    初值 0xFFFF（不是 0）、无输出反转
    自测标准值："123456789" → 0x4B37。算出别的数就是算法错了，别去测协议

【命令码】
  0x01  点亮 PA1
  0x02  熄灭 PA1
  0x03  回显 DATA（用来测多字节数据）

【实现步骤】
1、CubeMX 以 [19-10] 为模板复制：
   - DMA Settings 里删掉 USART1_RX 和 USART1_TX 两行（本工程不用 DMA）
   - NVIC 里保留 USART1 global interrupt（删 DMA 时别顺手关掉它）
2、放文件：crc16.c / protocol.c → Core/Src，crc16.h / protocol.h → Core/Inc
   Keil 里右键 Application/User/Core 分组 → Add Existing Files 加这两个 .c
   注意：加完之后就不要再回 CubeMX 点 Generate，它不认识手动加的文件，
         重新生成会把它们从 .uvprojx 里移出去，链接时报 undefined symbol
3、Options for Target → Target 页 → 勾 Use MicroLIB（printf 重定向必需，
   不勾会走 semihosting，程序停在启动阶段出不来）
4、main.c：
   回调 HAL_UART_RxCpltCallback 里只做一件事——把字节喂给 Proto_Parse；
   主循环 while(1) 里看 s_frame_ready 标志，switch(cmd) 执行业务。
   ISR 里绝不能 printf 或 HAL_Delay，115200 下 87us 就来下一个字节。

【解析：7 态状态机】
  HEAD1 → HEAD2 → LEN → CMD → DATA×(LEN) → CRC_L → CRC_H → 出帧

  int8_t Proto_Parse(uint8_t byte, ProtoFrame_t *out);
    0 未成帧继续喂 / 1 解出一帧 / -1 校验错 / -2 长度非法

  它一次只吃一个字节，状态存在 static 变量里。这就是粘包、半包天然成立的原因：
  粘包 = 出帧后回 HEAD1，紧跟的 AA 55 自动成为下一帧开头；
  半包 = 状态跨时间保留，来了 3 个字节就停在中途，下次接着走。

三个非显然的设计决定：
  ① ST_HEAD2 里再来一个 0xAA 故意不动作，留在原状态
     → "AA AA 55 ..." 这种重叠帧头也能同步；若回 HEAD1 会被自己的帧头带偏
  ② ST_LEN 里 LEN > 32 立即 Reset 并返回 -2
     → s_raw 只有 2+32=34 字节，按 40 收会越界写，这是内存安全问题不是逻辑问题
  ③ ST_CRC_H 里不论对错都 Reset
     → 对了要接下一帧（粘包），错了要重新找同步点，两种情况都得回起点

【三个坑】
① Transmit_IT 之后紧跟阻塞发送（[19-8] 踩过）
   _IT 一返回就把 gState 置成 BUSY_TX，阻塞版 HAL_UART_Transmit 第一句
   if (gState == HAL_UART_STATE_READY) 不成立，直接 return HAL_BUSY，
   整串提示语一个字节都发不出去。转发要用阻塞版，或者别在同一个串口上混用。

② 靠 IDLE 判帧（[19-10] 踩过）
   HAL_UARTEx_ReceiveToIdle_DMA 内部同时开了半传输（HT）中断，LENGTH=64 时
   收到第 32 字节就会以 Size=32、RxEventType=HT 回调一次，长帧被切成两段；
   帧内停顿超过 1 个字符时间也会被切。IDLE 是物理现象，不是数据内容。
   有了 LEN 字段之后就不该再依赖它。

③ 把长度字段直接当索引用（[19-6] 踩过）
   逐字节接收时用 &RxBuff[3] 写数组，越界正好命中 uwTickFreq(0x20000004)，
   uwTick 被加错值，所有 HAL_Delay 快 88 倍。现象是"程序整体跑飞"，
   根因离现场很远——所以长度字段必须先做范围检查再拿来用。

【验收：串口助手切 HEX 发送，不要勾"发送新行"】
  1  AA 55 00 01 C0 70            → [OK] CMD=0x01 LED_ON
  2  AA 55 00 02 80 71            → [OK] CMD=0x02 LED_OFF
  3  AA 55 03 03 11 22 33 68 F4   → [OK] CMD=0x03 ECHO LEN=3 DATA=11 22 33
  4  AA 55 00 01 C1 70            → 无输出（CRC 错，s_err_crc 加 1）
  5  AA 55 28 01 DE 70            → 无输出（LEN=40 超限，s_err_len 加 1）
  6  1 和 2 两条拼一起一次发完     → 两条 OK（粘包）
  7  第 3 条分三次发、中间停 1 秒   → 仍能完整解出（半包）

  第 4、5 条"没反应"是正确行为，不是故障：坏帧必须静默丢弃，
  回一个"我收到坏帧了"反而容易被上位机误当成成功。
  第 5 条的 CRC 其实是对的（28 01 的 CRC 就是 0x70DE），它坏在 LEN 越界——
  所以这两条区分的是"传输坏了"和"参数非法"两类错误。

【中文显示为什么不乱码】
  源文件是 UTF-8 with BOM，编译器把字面量按源文件字节原样放进 Flash，
  也就是 UTF-8 字节；而野火 / XCOM 这类助手默认按 GBK 解码 → 中文乱码。
  本工程的做法：把要发到串口的中文预转成 GBK 字节，用 \xNN 转义写进
  main.c 的 MSG_* 宏，printf 只引用宏。源文件仍是 UTF-8，芯片里存的是 GBK。
  代价：串口助手若切到 UTF-8 解码，这些文案反而会乱——二者只能取一。
  链接器给的中文字符串按字节编译，注意 "\xNN" 后面紧跟十六进制字符时
  要用 "" 断开，否则转义会把后面的字符吞掉。

【本目录其他文件】
  自定义协议-面试一页纸.md / .html / .pdf
    面试讲稿，A4 一页。源文件在 03-参考资料\19-11-自定义协议-面试一页纸.md，
    改内容请改那边，再重新导出 HTML/PDF，不要直接改本目录的副本。

【已处理】
  工程文件与 .ioc 已统一改名为 19-11。原先沿用 CubeMX 生成的 19-10，
  与 [19-10] 工程重名，hex 也同名，容易烧错文件。
  crc16.c / protocol.c 及其 .h 原先在 Keil 工程里是绝对路径引用
  （指向 04-代码草稿），已改为 ../Core/Src、../Core/Inc 相对路径，
  换机器或 clone 下来都能直接编译。
  注意：若之后回 CubeMX 重新生成代码，它会把手动 Add 的这两个 .c 移出工程，
  需要重新 Add 一次。
