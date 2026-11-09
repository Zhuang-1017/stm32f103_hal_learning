2026.11.9
串口发送之更换脚组实验
更换时要把TX和RX换到对应的引脚上

要去魔术棒中勾选使用USE MicroLIB 才可以使用printf函数

#include<string.h>
strlen获取字符串长度

print输出字符通过串口发送

#include<stdio.h>
/**
	* @brief 重定向printf的输出到串口
	* @param ch 要发送的字符
	* @param f 文件指针（标准库要求的参数一般不使用）
	* @retval 返回发送的字符
	*/
int fputc(int ch, FILE*f)
{
	HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,HAL_MAX_DELAY);
	return ch;
}

使用pa2会有电平波动使led灯闪烁

TXD要接RX
RXD要接TX