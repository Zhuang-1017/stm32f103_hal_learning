11.05
串口输出字符串/print函数

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