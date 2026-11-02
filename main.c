/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_LOCKED = 0,   // 待機：紅1呼吸
    STATE_WRONG,        // 錯誤：紅1恆亮 3 秒
    STATE_LOCKOUT,      // 鎖定：紅1紅2交閃 30 秒
    STATE_UNLOCKED      // 解鎖：綠燈呼吸
} LockState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_MAX       1000
#define BREATH_STEP   10
#define BREATH_PERIOD 10     // ms
#define BLINK_PERIOD  250    // ms
#define WRONG_HOLD_MS 3000   // 錯誤恆亮 3 秒
#define LOCKOUT_SEC   30     // 鎖定 30 秒
#define RX_BUF_SIZE   16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
LockState state = STATE_LOCKED;

uint8_t  wrong_count = 0;
uint32_t state_tick  = 0;
uint32_t breath_tick = 0;
uint32_t blink_tick  = 0;
uint32_t last_sec    = 99;

int16_t  duty = 0;
int8_t   dir  = 1;
uint8_t  blink = 0;

char    rx_buf[RX_BUF_SIZE];
uint8_t rx_idx = 0;
char    rx_char;

char password[] = "1234";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void uart_print(const char *s);
static void set_leds(uint16_t r1, uint16_t r2, uint16_t g);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void uart_print(const char *s)
{
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

static void set_leds(uint16_t r1, uint16_t r2, uint16_t g)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, r1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, r2);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, g);
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
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);   // 紅1
     HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);   // 紅2
     HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);   // 綠

     set_leds(0, 0, 0);

     HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_char, 1);

     state = STATE_LOCKED;
     breath_tick = HAL_GetTick();

     uart_print("\r\n=== UART Password Lock ===\r\n");
     uart_print("Enter password:\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


	  switch (state)
	         {
	         /* ---------- 待機：紅1呼吸 ---------- */
	         case STATE_LOCKED:
	             if (HAL_GetTick() - breath_tick >= BREATH_PERIOD)
	             {
	                 breath_tick = HAL_GetTick();
	                 duty += dir * BREATH_STEP;
	                 if (duty >= PWM_MAX) { duty = PWM_MAX; dir = -1; }
	                 if (duty <= 0)       { duty = 0;       dir = 1;  }

	                 set_leds((uint16_t)duty, 0, 0);
	             }
	             break;

	         /* ---------- 錯誤：紅1恆亮 3 秒 ---------- */
	         case STATE_WRONG:
	             set_leds(PWM_MAX, 0, 0);

	             if (HAL_GetTick() - state_tick >= WRONG_HOLD_MS)
	             {
	                 if (wrong_count >= 3)
	                 {
	                     state = STATE_LOCKOUT;
	                     state_tick  = HAL_GetTick();
	                     blink_tick  = HAL_GetTick();
	                     last_sec    = 99;
	                     blink       = 0;
	                     uart_print("Locked 30s\r\n");
	                 }
	                 else
	                 {
	                     state = STATE_LOCKED;
	                     breath_tick = HAL_GetTick();
	                     uart_print("Enter password:\r\n");
	                 }
	             }
	             break;

	         /* ---------- 鎖定：紅1紅2交閃 30 秒 ---------- */
	         case STATE_LOCKOUT:
	             if (HAL_GetTick() - blink_tick >= BLINK_PERIOD)
	             {
	                 blink_tick = HAL_GetTick();
	                 blink = !blink;
	                 set_leds(blink ? PWM_MAX : 0,
	                          blink ? 0 : PWM_MAX,
	                          0);
	             }

	             {
	                 uint32_t elapsed = (HAL_GetTick() - state_tick) / 1000;
	                 uint32_t remain  = (elapsed >= LOCKOUT_SEC) ? 0 : (LOCKOUT_SEC - elapsed);

	                 if (elapsed != last_sec)
	                 {
	                     last_sec = elapsed;
	                     char msg[32];
	                     sprintf(msg, "Locked: %lu s\r\n", (unsigned long)remain);
	                     uart_print(msg);
	                 }

	                 if (elapsed >= LOCKOUT_SEC)
	                 {
	                     wrong_count = 0;
	                     state = STATE_LOCKED;
	                     breath_tick = HAL_GetTick();
	                     set_leds(0, 0, 0);
	                     uart_print("Enter password:\r\n");
	                 }
	             }
	             break;

	         /* ---------- 解鎖：綠色呼吸 ---------- */
	         case STATE_UNLOCKED:
	             if (HAL_GetTick() - breath_tick >= BREATH_PERIOD)
	             {
	                 breath_tick = HAL_GetTick();
	                 duty += dir * BREATH_STEP;
	                 if (duty >= PWM_MAX) { duty = PWM_MAX; dir = -1; }
	                 if (duty <= 0)       { duty = 0;       dir = 1;  }

	                 set_leds(0, 0, (uint16_t)duty);
	             }
	             break;

	         default:
	             state = STATE_LOCKED;
	             break;
	         }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 169;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 169;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1)
    {
        char c = rx_char;

        if (c == '\r' || c == '\n')
        {
            rx_buf[rx_idx] = '\0';

            if (state == STATE_LOCKED && rx_idx > 0)
            {
                if (strcmp(rx_buf, password) == 0)
                {
                    state = STATE_UNLOCKED;
                    wrong_count = 0;
                    breath_tick = HAL_GetTick();
                    uart_print("Unlocked\r\n");
                }
                else
                {
                    wrong_count++;
                    state = STATE_WRONG;
                    state_tick = HAL_GetTick();
                    uart_print("Wrong password\r\n");
                }
            }
            rx_idx = 0;
        }
        else
        {
            if (rx_idx < RX_BUF_SIZE - 1)
            {
                rx_buf[rx_idx++] = c;
            }
        }

        HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_char, 1);
    }
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
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
