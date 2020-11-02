/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "zm_cli.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CLI_NAME "test cli >"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
struct cli_buf_t
{
    uint8_t buf[256];
    uint8_t wpos;
    uint8_t rpos;
    uint16_t len;
}cli_data = {
    .wpos = 0,
    .rpos = 0,
    .len = 0
};

static void uartRecvHandler(uint8_t *buf, uint16_t len)
{
    uint16_t cnt = 0;
    
    for(cnt = 0; cnt < len; cnt++)
    {
        cli_data.buf[cli_data.wpos++] = buf[cnt];
    }
    cli_data.len += cnt;
}

static int test_write(const void *a_data, uint16_t length)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)a_data, length, 0x10);
    return length;
}
static int test_read(void *a_buf, uint16_t length)
{
    uint16_t cnt = 0;
    uint8_t *p = (uint8_t *)a_buf;
    if(cli_data.len)
    {
        for(cnt = 0; cnt < length; cnt++)
        {
            p[cnt] = cli_data.buf[cli_data.rpos++];
        }
        cli_data.len -= cnt;
    }
    return cnt;
}
static cli_trans_api_t trans_api = 
{
  .write = test_write,
  .read = test_read
};


static void cli_print_hello(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    p_cli->m_printf_ctx->printf(p_cli, ZM_CLI_NORMAL, "hello\n");
}

static void cli_print_world(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    p_cli->m_printf_ctx->printf(p_cli, ZM_CLI_NORMAL, "world\n");
}
static void cli_print_help(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    print_usage(p_cli, argv[0], "<-h : get help> [--help]");
}


ZM_CLI_DEF(zm_cli, CLI_NAME, trans_api);

CLI_CREATE_STATIC_SUBCMD_SET(sub_2)
{
    CLI_CMD_LOAD_PARA(print_world, NULL, "printf word", cli_print_world),
    CLI_CMD_LOAD_PARA(help, NULL, "test help", cli_print_help),

    CLI_SUBCMD_SET_END
};

CLI_CREATE_STATIC_SUBCMD_SET(sub_1)
{
    CLI_CMD_LOAD_PARA(print_hello, NULL, "print hello", cli_print_hello),
    CLI_CMD_LOAD_PARA(print_2, &sub_2, "test print", NULL),

    CLI_SUBCMD_SET_END
};

CLI_CMD_REGISTER(print, &sub_1, "print", NULL);


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF); 
  return (ch);
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  uartReceiveInit(uartRecvHandler);
  uartStartReceive();
  
  zm_cli_init(&zm_cli);
  printf("system initialize success!!!\r\n");
  zm_cli_start(&zm_cli);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    zm_cli_process(&zm_cli);
    if((HAL_GetTick() % 1000) ==0)
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  }
  /* USER CODE END 3 */
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

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
