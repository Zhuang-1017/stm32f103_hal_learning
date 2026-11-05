11.5
串口通信
串行通讯方式：一位一位地传输
TX传输端
RX接受端
波特率：TX/RX，波特率越大越快，但传输和接收要对的上
usart.c初始化串口文件
用串口发送字节：
uint8_t ch = "F";
HAL_UART_Transmit(&huart1,&ch,1,HAL_MAX_DELAY);

HAL_Dealy(1000);
USB转TTL模块的3.3V不要接，将TX接到PA10，RX接到PA9