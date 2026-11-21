2026.11.21
DMA与串口的应用

DMA：直接存储器访问
一种不依赖CPU的数据传输方式
可以在外设和存储之间或者存储器与存储器之间进行直接搬运数据

用DMA代替CPU来完成数据传输，CPU就能专注于更复杂的任务

DMA 的工作原理
只需配置：源地址、目标地址、传输方向、数据长度
传输过程完全由DMA控制
数据传输完成后，DMA会通过中断通知CPU

DMA的核心参数：源地址（数据从哪里来）、目标地址（数据放到哪里）、传输数据量、传输模式（单次、循环）

hal库函数
空闲中断+DMA接收：HAL_UARTEx_ReceiveToIdle_DMA()    启动DMA接收，支持空闲中断，可解决不定长接收问题
发送数据（DMA方式）：HAL_UART_Transmit_DMA()    使用DMA方式发送数据
DMA接收完成回调函数：HAL_UARTEx_RxEventCallback()   当接收完成或检测到空闲中断时自动调用，返回实际接收数据长度

实现步骤（main.c）

1、CubeMX 里给 USART1 同时勾 DMA RX 和 DMA TX：
   RX -> DMA1 Channel5（外设到内存），TX -> DMA1 Channel4（内存到外设），模式都用 Normal。
   两个 Channel 的 NVIC 中断都要 Enable，缺一个后面的状态机就卡死（见坑①）。
2、code begin PD：  #define LENGTH 64
   code begin PV：  uint8_t RxBuff[LENGTH];
3、code begin 2：   HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuff, LENGTH);
4、回调 HAL_UARTEx_RxEventCallback：
   printf 提示长度 -> HAL_UART_Transmit_DMA 回显 -> 再 ReceiveToIdle_DMA 重开接收。

三个坑（按 HAL 源码行为推演，未逐条实测）

① 两个 DMA 通道中断没开：DMA 照样把数据搬完（硬件搬运不需要 CPU），
   但 UART_DMATransmitCplt 不会被调用，TC 中断使能不了，gState 永远停在 BUSY_TX。
   之后 printf（走阻塞 HAL_UART_Transmit）和 Transmit_DMA 全部 return HAL_BUSY。
   现象是"第一帧回显正常，之后一句话都没有"。

② ReceiveToIdle_DMA 同时开了半传输（HT）中断：
   发超过 32 字节（LENGTH/2）时，会先以 Size=32 触发一次 RXEVENT_HT，
   回调里就把前半段回显掉并重开接收，后半段被截断。
   需要在回调里判断只处理 huart->RxEventType == HAL_UART_RXEVENT_IDLE 或 RXEVENT_TC。

③ 回显和重开接收共用同一个 RxBuff：
   HAL_UART_Transmit_DMA 是异步的，一返回就结束，此时一个字节都还没出去；
   紧接着 ReceiveToIdle_DMA 把 RxBuff 交给 DMA 接收，新数据会写进正在发送的同一块内存。
   连续发两帧时会看到回显内容错位。要彻底解决得用双缓冲（乒乓）。
