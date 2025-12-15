/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "hrtim.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>

/* 全局变量用于存储 中断次数 */
volatile int32_t num_10us = 0;
volatile int32_t num_30us = 0;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t CDC_Transmit_Wait(uint8_t* Buf, uint16_t Len); // [新增] 阻塞式发送原型
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define US_DELAY_COUNT 2400
static void Delay_us_50(void)
{
    // 使用 volatile 和 __NOP() 确保编译器不会优化掉循环，以实现准确的忙等。
    for (volatile uint32_t i = 0; i < US_DELAY_COUNT; i++)
    {
        __NOP();
    }
}

uint8_t CDC_Transmit_Wait(uint8_t* Buf, uint16_t Len)
{
    uint8_t status;
    uint32_t timeout = 20000; // 约 1秒超时 (20000 * 50us)
    
    do 
    {
        status = CDC_Transmit_FS(Buf, Len);
        if (status == USBD_BUSY) 
        {
            Delay_us_50(); 
            if (--timeout == 0) return USBD_BUSY; // 超时退出，避免死锁
        }
    } while (status == USBD_BUSY);
    
    return status;
}
/**
  * @brief  阻塞式 CDC 发送函数，直到 USB 缓冲区空闲并接受数据为止
  * @param  Buf: 待发送数据缓冲区
  * @param  Len: 数据长度
  * @retval USBD_StatusTypeDef 状态 (USBD_OK, USBD_FAIL 等)
  */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  num_10us = 0;
  num_30us = 0;
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
  // MX_GPIO_Init();
  // MX_HRTIM1_Init();
  // MX_I2C3_Init();
  // MX_SPI1_Init();
  // MX_SPI3_Init();
  // MX_USART1_UART_Init();
  // MX_USB_Device_Init();
  /* USER CODE BEGIN 2 */
  MX_GPIO_Init();
  MX_HRTIM1_Init();
  // MX_I2C3_Init();
  // MX_SPI1_Init();
  // MX_SPI3_Init();
  // MX_USART1_UART_Init();
  MX_USB_Device_Init();
  HAL_Delay(5000);

  char msg[64];
  sprintf(msg, "HRTIM Start config\r\n");
  CDC_Transmit_Wait((uint8_t*)msg, strlen(msg));
  HAL_Delay(1000);

  // [新增步骤]：手动触发更新事件，将预装载寄存器值加载到影子寄存器
  //HAL_HRTIM_SoftwareUpdate(&hhrtim1, HRTIM_TIMERUPDATE_MASTER | HRTIM_TIMERUPDATE_A);


  // 1. 先启动 Master Timer (虽然它可能不输出波形，但它提供时基和复位信号)
  HAL_HRTIM_WaveformCountStart_IT(&hhrtim1, HRTIM_TIMERID_MASTER);
  // 2. 启动 Timer A 的计数器，并使能中断
  // 修改参数为 TIMERINDEX，并检查返回值
  if (HAL_HRTIM_WaveformCountStart_IT(&hhrtim1, HRTIM_TIMERID_TIMER_A) != HAL_OK)
  {
      Error_Handler(); // 如果启动失败，进入错误处理
  }
  else{
      sprintf(msg, "HRTIM Timer A started with interrupt!\r\n");
      CDC_Transmit_Wait((uint8_t*)msg, strlen(msg));
  }

    // 这一步直接操作 TIMADIER 寄存器，确保门控打开
  __HAL_HRTIM_TIMER_ENABLE_IT(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_TIM_IT_CMP1 | HRTIM_TIM_IT_CMP2);

  // 3. 启动 Timer A 的 PWM 输出 (TA1 和 TA2)
  // 如果你只用 TA1，可以只写 HRTIM_OUTPUT_TA1
  HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 循环尝试启动 Master Timer (虽然它可能不输出波形，但它提供时基和复位信号)
    // if (HAL_HRTIM_WaveformCountStart_IT(&hhrtim1, HRTIM_TIMERINDEX_MASTER)!= HAL_OK){
    //     Error_Handler(); // 如果启动失败，进入错误处理
    // }
    // else{
    //     sprintf(msg, "HRTIM Timer master started with interrupt!\r\n");
    //     CDC_Transmit_Wait((uint8_t*)msg, strlen(msg));
    // }
    // // 2. 启动 Timer A 的计数器，并使能中断
    // // 修改参数为 TIMERINDEX，并检查返回值
    // if (HAL_HRTIM_WaveformCountStart_IT(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A) != HAL_OK)
    // {
    //     Error_Handler(); // 如果启动失败，进入错误处理
    // }
    // else{
    //     sprintf(msg, "HRTIM Timer A started with interrupt!\r\n");
    //     CDC_Transmit_Wait((uint8_t*)msg, strlen(msg));
    // }


    sprintf(msg, "while\r\n");
    CDC_Transmit_Wait((uint8_t*)msg, strlen(msg));
    // 格式化输出字符串
    // 显示当前的计数值，理论上每秒应该打印出 "CMP1: 1000, CMP2: 1000"
    sprintf(msg, "CMP1(10us): %ld, CMP2(30us): %ld\r\n", (long)num_10us, (long)num_30us);
    
    // 发送数据
    CDC_Transmit_Wait((uint8_t*)msg, strlen(msg));
    HAL_Delay(1000);

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 18;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// 处理 Compare Unit 1 事件 (对应配置的 360 ticks, 即 10us)
void HAL_HRTIM_Compare1EventCallback(HRTIM_HandleTypeDef *hhrtim, uint32_t TimerIdx)
{
    // 判断是否是 Timer A 产生的中断
    if (TimerIdx == HRTIM_TIMERINDEX_TIMER_A)
    {
        num_10us++; // 这里对应 10us 事件
    }
}

// 处理 Compare Unit 2 事件 (对应配置的 1080 ticks, 即 30us)
void HAL_HRTIM_Compare2EventCallback(HRTIM_HandleTypeDef *hhrtim, uint32_t TimerIdx)
{
    if (TimerIdx == HRTIM_TIMERINDEX_TIMER_A)
    {
        num_30us++;  // 这里对应 30us 事件
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
