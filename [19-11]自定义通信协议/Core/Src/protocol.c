#include "protocol.h"
#include "crc16.h"
#include <string.h>

/* ============================================================================
 *  解帧状态机
 * ----------------------------------------------------------------------------
 *  设计要点：它不关心"字节什么时候到"，只关心"我已经收到第几个字节了"。
 *  正因为不依赖时间，它天然就能处理半包（来了 3 个字节就停在中途，下次接着）
 *  和粘包（出一帧立刻回 ST_HEAD1，后面紧跟的字节自动成为下一帧的开头）。
 * ========================================================================== */
typedef enum
{
    ST_HEAD1 = 0,   /* 等第一个帧头 0xAA */
    ST_HEAD2,       /* 等第二个帧头 0x55 */
    ST_LEN,         /* 收 LEN 并判合法性 */
    ST_CMD,         /* 收命令码 */
    ST_DATA,        /* 收 DATA，收满 LEN 个 */
    ST_CRC_L,       /* 收 CRC 低字节 */
    ST_CRC_H        /* 收 CRC 高字节并比对，出帧 */
} ParseState_t;

/* 逐字节累积的原始字节，[0]=LEN [1]=CMD [2..]=DATA
 * 这正好就是 CRC 要覆盖的范围，所以最后直接对它算 CRC 即可 */
static uint8_t       s_raw[2 + PROTO_MAX_DATA];

static ParseState_t  s_state = ST_HEAD1;
static uint8_t       s_len;      /* 本帧的 LEN */
static uint8_t       s_idx;      /* DATA 已经收了多少个 */
static uint8_t       s_crc_l;    /* 暂存 CRC 低字节 */

/* -------------------------------------------------------------------------- */
void Proto_Reset(void)
{
    s_state = ST_HEAD1;
    s_len   = 0;
    s_idx   = 0;
    s_crc_l = 0;
}

/* ----------------------------------------------------------------------------
 *  打包
 * -------------------------------------------------------------------------- */
uint16_t Proto_Pack(uint8_t cmd, const uint8_t *data, uint8_t len, uint8_t *out)
{
    uint16_t crc;
    uint8_t  i;

    if (out == 0)                     return 0;
    if (len > PROTO_MAX_DATA)         return 0;
    if ((len > 0) && (data == 0))     return 0;

    out[0] = PROTO_HEAD1;
    out[1] = PROTO_HEAD2;
    out[2] = len;
    out[3] = cmd;

    for (i = 0; i < len; i++)
    {
        out[4 + i] = data[i];
    }

    /* 校验范围从 out[2] 开始，长度 = LEN(1) + CMD(1) + DATA(len)
     * 注意：不含 out[0] / out[1] 两个帧头 */
    crc = CRC16_Modbus(&out[2], (uint16_t)(len + 2u));

    out[4 + len] = (uint8_t)(crc & 0xFFu);   /* 低字节先发 */
    out[5 + len] = (uint8_t)(crc >> 8);

    return (uint16_t)(len + PROTO_OVERHEAD);
}

/* ----------------------------------------------------------------------------
 *  逐字节解帧
 * -------------------------------------------------------------------------- */
int8_t Proto_Parse(uint8_t byte, ProtoFrame_t *out)
{
    int8_t ret = PROTO_RET_MORE;

    switch (s_state)
    {
    /* ---------------------------------------------------------------- */
    case ST_HEAD1:
        if (byte == PROTO_HEAD1)
        {
            s_state = ST_HEAD2;
        }
        /* 不是帧头就什么都不做，留在原状态继续找 */
        break;

    /* ---------------------------------------------------------------- */
    case ST_HEAD2:
        if (byte == PROTO_HEAD2)
        {
            s_state = ST_LEN;               /* 同步成功，进入帧体 */
        }
        else if (byte != PROTO_HEAD1)
        {
            s_state = ST_HEAD1;             /* 既不是 55 也不是 AA，从头找 */
        }
        /* byte == 0xAA 时故意什么都不做：留在 ST_HEAD2
         * 这样 "AA AA 55 ..." 这种带重叠帧头的流也能正确同步 */
        break;

    /* ---------------------------------------------------------------- */
    case ST_LEN:
        if (byte > PROTO_MAX_DATA)
        {
            /* LEN 非法：可能是误码，也可能根本没对齐到帧头。
             * 不能按这个长度去收数据（会写爆缓冲区），直接整帧丢弃 */
            Proto_Reset();
            ret = PROTO_RET_LEN;
        }
        else
        {
            s_len    = byte;
            s_raw[0] = byte;
            s_idx    = 0;
            s_state  = ST_CMD;
        }
        break;

    /* ---------------------------------------------------------------- */
    case ST_CMD:
        s_raw[1] = byte;
        /* LEN = 0 的纯命令帧直接跳到 CRC，不要进 ST_DATA 白等 */
        s_state = (s_len == 0u) ? ST_CRC_L : ST_DATA;
        break;

    /* ---------------------------------------------------------------- */
    case ST_DATA:
        s_raw[2 + s_idx] = byte;
        s_idx++;
        if (s_idx >= s_len)
        {
            s_state = ST_CRC_L;
        }
        break;

    /* ---------------------------------------------------------------- */
    case ST_CRC_L:
        s_crc_l = byte;
        s_state = ST_CRC_H;
        break;

    /* ---------------------------------------------------------------- */
    case ST_CRC_H:
    {
        /* 低字节先到，所以要拼成 (低 | 高<<8) */
        uint16_t crc_recv = (uint16_t)s_crc_l | ((uint16_t)byte << 8);
        /* s_raw 里有效的是 LEN + CMD + DATA，共 2 + s_len 字节 */
        uint16_t crc_calc = CRC16_Modbus(s_raw, (uint16_t)(s_len + 2u));

        if (crc_recv == crc_calc)
        {
            if (out != 0)
            {
                out->cmd = s_raw[1];
                out->len = s_len;
                if (s_len > 0u)
                {
                    memcpy(out->data, &s_raw[2], s_len);
                }
            }
            ret = PROTO_RET_FRAME;
        }
        else
        {
            /* 校验失败：绝不把数据交给上层。
             * 如果这里"宽容"一下放行，一个比特翻转就可能把 0x02 关灯
             * 变成 0x03 读 ADC，直接把设备驱动到错误状态 */
            ret = PROTO_RET_CRC;
        }

        /* 不管对错都回起点：对了要接下一帧（粘包），错了要重新找同步点 */
        Proto_Reset();
        break;
    }

    /* ---------------------------------------------------------------- */
    default:
        Proto_Reset();
        break;
    }

    return ret;
}
