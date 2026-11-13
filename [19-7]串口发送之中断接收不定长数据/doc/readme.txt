11.13
串口中断接收不定长数据

使用HAL_UARTEx_ReceiveToIdle_IT()
实现接收任意长度数据	自动判断接收完成（通过UART空闲中断）
避免循环接收单字节并且可以自己判断结束

接收到的数据在HAL_UARTEx_RxEventCallback（）中处理
通常配合环形缓冲区、消息帧处理使用

跟接收定长数据做对比
