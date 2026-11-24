/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : [19-11] 自定义通信协议（帧头 + 长度 + CRC16）
  *
  *  本文件的 SystemClock_Config 与 [19-10] 完全一致（HSE 8M x9 = 72MHz）。
  *  如果你的工程时钟配置不同，请只取带 USER CODE 注释的段落粘贴，
  *  不要整文件覆盖，否则会把自己的时钟配置冲掉。
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "crc16.h"
#include "protocol.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* ============================================================================
 *  串口输出文案（中文按 GBK 字节固化）
 * ----------------------------------------------------------------------------
 *  问题：源文件是 UTF-8 with BOM（这样 GitHub 与 Keil 里的中文注释都正常），
 *        但编译器是把字符串字面量按源文件字节原样放进 Flash 的，也就是
 *        UTF-8 字节；而野火 / XCOM 这类串口助手默认按 GBK 解码，中文就乱码。
 *  办法：把要发到串口的中文预先转成 GBK 字节，用反斜杠加两位十六进制的
 *        转义形式写死。源文件仍是 UTF-8（注释可读、git 可看），芯片里存的
 *        却是 GBK，任何默认 GBK 的终端都能正常显示。
 *  代价：若把串口助手切到 UTF-8 解码，这些文案反而会乱 —— 二者只能取一。
 *  每条上方的注释就是它显示出来的中文原文；改文案时按同样方式重新转义。
 * ========================================================================== */
/* 上电横幅：  ===== [19-11] 自定义通信协议 ===== */
#define MSG_BANNER    "\r\n===== [19-11] \xD7\xD4\xB6\xA8\xD2\xE5\xCD\xA8\xD0\xC5\xD0\xAD\xD2\xE9 =====\r\n"
/* 帧格式说明：  帧格式: AA 55 | LEN | CMD | DATA | CRC_L CRC_H */
#define MSG_FMT       "\xD6\xA1\xB8\xF1\xCA\xBD: AA 55 | LEN | CMD | DATA | CRC_L CRC_H\r\n"
/* 长度说明：  整帧 = 6 + LEN,  LEN 上限 %d */
#define MSG_LENFMT    "\xD5\xFB\xD6\xA1 = 6 + LEN,  LEN \xC9\xCF\xCF\xDE %d\r\n"
/* 自测失败：  [SELFTEST] CRC16 FAIL! 先修 crc16.c，别往下测 */
#define MSG_CRC_FAIL  "[SELFTEST] CRC16 FAIL! \xCF\xC8\xD0\xDE crc16.c\xA3\xAC\xB1\xF0\xCD\xF9\xCF\xC2\xB2\xE2\r\n"
/* 就绪提示：  接收已就绪，等待数据... */
#define MSG_READY     "\xBD\xD3\xCA\xD5\xD2\xD1\xBE\xCD\xD0\xF7\xA3\xAC\xB5\xC8\xB4\xFD\xCA\xFD\xBE\xDD...\r\n\r\n"
/* DATA 为空时：  (空) */
#define MSG_EMPTY     "(\xBF\xD5)"


/* 演示用的 LED。沿用 [19-6] 的接法：PA1。
 * 如果 CubeMX 里没把 PA1 配成 GPIO_Output，这一段写 ODR 也不会报错，
 * 只是灯不会亮、printf 照常有输出。想看到灯，就回 CubeMX 把 PA1 设成
 * Output Push Pull、Low speed，然后重新 Generate。 */
#define LED_PORT   GPIOA
#define LED_PIN    GPIO_PIN_1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t  s_rx_byte;              /* Receive_IT 的单字节落脚点 */

static ProtoFrame_t s_frame;            /* 主循环读到的那一帧 */
static ProtoFrame_t s_tmp;              /* ISR 里的临时落脚点，解成功了再搬过去 */
static volatile uint8_t s_frame_ready;  /* ISR 置 1，主循环取走后清 0 */

static volatile uint32_t s_err_crc;     /* CRC 错帧计数 */
static volatile uint32_t s_err_len;     /* LEN 非法帧计数 */
static volatile uint32_t s_err_ovf;     /* 主循环来不及处理，丢帧计数 */

static volatile uint32_t s_ok_cnt;      /* 成功解出的帧数 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void HandleFrame(const ProtoFrame_t *f);
static void PrintHex(const uint8_t *p, uint8_t n);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ----------------------------------------------------------------------------
 *  串口接收完成回调：每收到 1 个字节进一次
 * ----------------------------------------------------------------------------
 *  这里只做"喂字节"，绝不做业务。
 *  Proto_Parse 每处理一个字节约 1~2us（72MHz），完全安全；
 *  但一旦在里面 printf 或 HAL_Delay，115200 下 87us 就会来下一个字节，
 *  立刻 ORE 丢数据——[19-7] 和 [19-8] 都踩过这个坑。
 * -------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    int8_t r;

    if (huart->Instance != USART1)
    {
        return;
    }

    r = Proto_Parse(s_rx_byte, &s_tmp);

    if (r == PROTO_RET_FRAME)
    {
        s_ok_cnt++;
        if (s_frame_ready == 0u)
        {
            /* 主循环已经取走了上一帧，可以放新帧进去 */
            memcpy(&s_frame, &s_tmp, sizeof(s_frame));
            s_frame_ready = 1u;
        }
        else
        {
            /* 主循环还没处理完，这一帧丢掉。
             * 注意这里只丢"帧"不丢"字节"——状态机照样往下走，
             * 所以后面紧跟的帧不会跟着失步。 */
            s_err_ovf++;
        }
    }
    else if (r == PROTO_RET_CRC)
    {
        s_err_crc++;
    }
    else if (r == PROTO_RET_LEN)
    {
        s_err_len++;
    }

    /* 必须重开！否则只收得到这 1 个字节，后面全没反应。
     * 放在最后而不是最前，是为了让上面解析耗时的那一刻不再中断嵌套。
     * HAL 在进这个回调之前已经把 RxState 置回 READY，所以这里重开是合法的。 */
    HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  printf(MSG_BANNER);
  printf(MSG_FMT);
  printf(MSG_LENFMT, (int)PROTO_MAX_DATA);

  /* 先验 CRC 实现。这一步不过，后面所有"协议不通"都无法判断是 CRC 错
   * 还是状态机错，所以必须最先跑 */
  if (CRC16_SelfTest() != 0u)
  {
      printf("[SELFTEST] CRC16 OK  (\"123456789\" -> 0x4B37)\r\n");
  }
  else
  {
      printf(MSG_CRC_FAIL);
  }

  Proto_Reset();
  HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);   /* 开第一枪 */
  printf(MSG_READY);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* 业务全在这里做。ISR 只负责把"解出一帧"的消息递过来 */
    if (s_frame_ready != 0u)
    {
        s_frame_ready = 0u;
        HandleFrame(&s_frame);
    }

    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* ----------------------------------------------------------------------------
 *  语义层：把 cmd 分发到具体动作
 * ----------------------------------------------------------------------------
 *  能走到这里，说明帧头对、LEN 合法、CRC 也对，数据可信。
 *  CRC 校验和命令执行必须分开，先验后用。
 * -------------------------------------------------------------------------- */
static void HandleFrame(const ProtoFrame_t *f)
{
    switch (f->cmd)
    {
    case CMD_LED_ON:
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        printf("[OK]  CMD=0x01 LED_ON   LEN=%u DATA=", f->len);
        PrintHex(f->data, f->len);
        printf("\r\n");
        break;

    case CMD_LED_OFF:
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        printf("[OK]  CMD=0x02 LED_OFF  LEN=%u DATA=", f->len);
        PrintHex(f->data, f->len);
        printf("\r\n");
        break;

    case CMD_ECHO:
        printf("[OK]  CMD=0x03 ECHO     LEN=%u DATA=", f->len);
        PrintHex(f->data, f->len);
        printf("\r\n");
        break;

    default:
        /* 帧是完整的，但命令不认识。这种情况要明确回错误，不能静默 */
        printf("[ERR] UNKNOWN CMD=0x%02X\r\n", f->cmd);
        break;
    }
}

/* -------------------------------------------------------------------------- */
static void PrintHex(const uint8_t *p, uint8_t n)
{
    uint8_t i;

    if ((p == 0) || (n == 0u))
    {
        printf(MSG_EMPTY);
        return;
    }

    for (i = 0; i < n; i++)
    {
        printf("%02X ", p[i]);
    }
}

/* ----------------------------------------------------------------------------
 *  printf 重定向。Keil 里必须勾上 Target -> Use MicroLIB，
 *  否则会卡在 semihosting，程序停在启动阶段出不来。
 * -------------------------------------------------------------------------- */
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
