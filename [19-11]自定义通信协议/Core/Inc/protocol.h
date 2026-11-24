#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>

/* ============================================================================
 *  帧格式
 * ----------------------------------------------------------------------------
 *   偏移   0    1    2     3     4 .... 4+LEN-1   4+LEN  5+LEN
 *         AA   55   LEN   CMD   DATA[LEN]         CRC_L  CRC_H
 *          1    1    1     1       LEN              1      1
 *
 *   整帧长度 = 6 + LEN，最短 6 字节（LEN = 0 时的"纯命令帧"）
 *
 *   三条约定，写错任何一条都会导致对端永远解不出来：
 *     1. LEN 是 DATA 的字节数，不含 CMD，不含帧头，不含 CRC
 *     2. CRC16 的覆盖范围是 LEN + CMD + DATA，不含两个帧头 0xAA 0x55
 *        —— 帧头只负责同步，算进校验没有意义
 *     3. CRC 低字节先发（Modbus 惯例），即先 CRC_L 后 CRC_H
 * ========================================================================== */

#define PROTO_HEAD1       0xAAu
#define PROTO_HEAD2       0x55u
#define PROTO_MAX_DATA    32u    /* DATA 上限；同时是 LEN 的合法上限 */
#define PROTO_OVERHEAD    6u     /* 除 DATA 之外的固定开销：2+1+1+2 */

/* 命令码：这一层只定义"要干什么"，不关心怎么执行 */
#define CMD_LED_ON        0x01u
#define CMD_LED_OFF       0x02u
#define CMD_ECHO          0x03u  /* 原样回显 DATA，用来测多字节数据 */

/* Proto_Parse 的返回值 */
#define PROTO_RET_MORE    0      /* 还没凑够一帧，继续喂字节 */
#define PROTO_RET_FRAME   1      /* 解出一帧且校验通过，out 里是有效数据 */
#define PROTO_RET_CRC    (-1)    /* 校验失败，整帧丢弃 */
#define PROTO_RET_LEN    (-2)    /* LEN 超过 PROTO_MAX_DATA，整帧丢弃 */

/* 解析出来的一帧。注意 data 是定长数组，直接用，不用 malloc */
typedef struct
{
    uint8_t cmd;                       /* 命令码 */
    uint8_t len;                       /* DATA 实际字节数 */
    uint8_t data[PROTO_MAX_DATA];      /* DATA 内容 */
} ProtoFrame_t;

/* 打包一帧，返回整帧字节数（6 + len）；参数非法时返回 0
 *   out 缓冲区至少要 PROTO_OVERHEAD + len 字节 */
uint16_t Proto_Pack(uint8_t cmd, const uint8_t *data, uint8_t len, uint8_t *out);

/* 逐字节解帧。一次只吃 1 个字节，内部带状态，粘包和半包都不用额外处理
 *   byte 新收到的一个字节
 *   out  解出完整帧时写入（可以是临时变量）
 *   返回 PROTO_RET_xxx */
int8_t Proto_Parse(uint8_t byte, ProtoFrame_t *out);

/* 强制复位解析状态机（上电、出错恢复、通信超时后可以手动调） */
void Proto_Reset(void);

#endif /* __PROTOCOL_H */
