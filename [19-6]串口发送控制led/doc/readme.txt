11.13
串口控制led
使用中断方式
判断接收到的内容
接收到on点亮led
接收到off关闭led

设置异步通信
设置gpio三个led
中断使能要勾选add

#include<string.h>
strncmp()
作用：检查接收缓冲区字符是否相同

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart -> Instance == USART1)
	{
		RxCount ++;
	
		if(RxCount == 2 && strncmp((char *)RxBuff , "ON" ,2) == 0)
		{
			HAL_GPIO_WritePin(GPIOA,LED_R_Pin|LED_B_Pin|LED_G_Pin,GPIO_PIN_SET);//设置成高电平点亮led
			RxCount = 0;	//计数清空
			memset(RxBuff,0,sizeof(RxBuff));//清除数组即接收缓冲区的内容
			
		}
		else if(RxCount == 3 &&strncmp((char *)RxBuff , "OFF" ,3) == 0)
		{
			HAL_GPIO_WritePin(GPIOA,LED_R_Pin|LED_B_Pin|LED_G_Pin,GPIO_PIN_RESET);//设置成低电平熄灭led
			RxCount = 0;	//计数清空
			memset(RxBuff,0,sizeof(RxBuff));//清除数组即接收缓冲区的内容
			
		}
		HAL_UART_Receive_IT(&huart1,&RxBuff[RxCount],1);//重新启用接收中断
	}
}