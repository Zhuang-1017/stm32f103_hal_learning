2026.11.16
多串口应用
实验目标：同时使用USART1和USART2
串口1收到数据->转发给串口2

核心：
开启两个串口的接收中断
将其中一个串口接收到的串口传到另一个串口上

可以参考19-7的空闲中断

步骤：
1、开启两个串口的使能接收中断
在code begin2
hal_uartx_receivetoidle_it
2、缓冲区的设置
在code begin init
uint8_t rxbuff[length];
两个串口要有两个缓冲区
在code begin pv
定义缓冲区的大小
#define length 64
3、回调函数内容
在code begin4
void hal_uatrx_rxeventcallback
{
	if(huart ->instance ==uasrt1)
	{
		hal_uart_transmit_it//回显发送的数据
		hal_uartx_receivetoidle_it//使能接收中断
	}
}
但是重定向printf一次只能重定向一个串口
所以重新编写一个支持任何串口发送数据的类似于printf的函数
void usart_sendstring(uart_handletypedef *huanrt,char *str)
{
	while(*str)//当前字符不是结束符“\0”
	{
		hal_usart_transmit(huart,(uint8_t *)str,1,hal_delay_max_delay)//阻塞式串口发送
		str++;	
	}
}
函数声明要放在code begin pfp
4、回调函数
void hal_uatrx_rxeventcallback
{
	if(huart ->instance ==uasrt1)
	{
		usart_sendstring(&huart1,"……\r\n");
		hal_uart_transmit(&huart2,rxbuff1,size,hal_max_delay)//回显发送的数据，用阻塞发送，不会被覆盖
		usart_sendstring(&huart1,"……\r\n");
		hal_uartx_receivetoidle_it(&huart1,rxbuff1,length)//使能接收中断
	}
	else if(huart ->instance ==uasrt2)
	{
		usart_sendstring(&huart1,"……\r\n");
		hal_uart_transmit(&huart1,rxbuf2,size,hal_max_delay)//回显发送的数据，用阻塞发送，不会被覆盖
		usart_sendstring(&huart1,"……\r\n");
		hal_uartx_receivetoidle_it(&huart2,rxbuff2,length)//使能接收中断
	}


}

踩坑：
转发时必须用阻塞的 hal_uart_transmit，不能用 hal_uart_transmit_it。
因为 hal_uart_transmit_it 一返回就把 gState 置成 BUSY_TX，
紧接着调用 hal_uart_transmit 会在第一句 if(gState==READY) 判断失败，
直接 return HAL_BUSY，后面的提示语一个字节都发不出去。
而且 _it 还没发完就重开接收，同一个 buffer 会被边发边写覆盖。
