#ifndef __CRC16_H
#define __CRC16_H

#include <stdint.h>

/* CRC-16/MODBUS 变体
 *   多项式   0xA001（0x8005 的按位反转，因为算法是右移）
 *   初值     0xFFFF
 *   输出反转 无
 *   结果异或 0x0000
 * 标准自测输入 "123456789" 的正确结果是 0x4B37
 */
uint16_t CRC16_Modbus(const uint8_t *buf, uint16_t len);

/* 自测：返回 1 表示算出 0x4B37，0 表示实现有错 */
uint8_t CRC16_SelfTest(void);

#endif /* __CRC16_H */
