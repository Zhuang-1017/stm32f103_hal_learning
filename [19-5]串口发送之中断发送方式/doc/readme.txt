2026.11.10
串口接收之中断接受定长数据
接受模式：阻塞接收、非阻塞接收、中断接收、DMA接收
中断接收：当串口接收到预定数量的数据后，硬件会触发一个中断
hal函数
HAL_UART_RECEIVE_IT   ( )启动中断接收，指定接收的缓冲区和长度
HAL_UART_Transmit_IT（）使用中断方式发送数据
HAL_UART_RxCplitCallback() 	接收完成时自动调用用户可在此处理数据

cubemx
串口的设置NIVCseting打开
到NVIC中断管理就可以看到 中断等级保持默认


/**
  * @brief  Rx Transfer completed callbacks.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart -> Instance == USART1) //判断发生接收中断的串口
	{
		printf("\r\n已经接收到数据，数据如下：");
		HAL_UART_Transmit_IT(&huart1,RxBuff,LENGTH);//回显发送的数据
		
		HAL_UART_Receive_IT(&huart1,RxBuff,LENGTH);//再次使能接收中断
	}
}
