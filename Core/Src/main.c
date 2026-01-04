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
#include "TMUX1108.h"
#include "AD4007.h" 

/* 全局变量用于存储 中断次数 */
volatile int32_t num_3us = 0;
volatile int32_t num_300us = 0;
/* 全局变量用于存储 ADC 结果 */
volatile uint32_t adc_raw_3us = 0;
volatile uint32_t adc_raw_300us = 0;
volatile float adc_voltage_3us = 0;
volatile float adc_voltage_300us = 0;
volatile uint8_t measure_done = 0;

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
/* USER CODE BEGIN PV */
volatile uint8_t data_ready_flag = 0; // 采样完成标志位

#define PACKET_SIZE 11        // 单个采样点的字节数
#define BATCH_COUNT 10       // 每积攒10个采样点发送一次 (可根据实时性需求调整)
#define TX_BUF_SIZE (PACKET_SIZE * BATCH_COUNT)

uint8_t usb_tx_cache[TX_BUF_SIZE]; // USB 发送缓存
uint8_t sample_counter = 0;        // 缓存计数器

// 用于暂存采集到的数据
volatile int32_t current_adc_3us = 0;
volatile int32_t current_adc_300us = 0;
/* USER CODE END PV */
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
    uint32_t timeout = 2000; // 约 0.1秒超时 (2000 * 50us)
    
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
  num_3us = 0;
  num_300us = 0;
  adc_raw_3us = 0;
  adc_raw_300us = 0;
  adc_voltage_3us = 0;
  adc_voltage_300us = 0;
  uint8_t data_frame[11];
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
  // MX_HRTIM1_Init();
  // MX_I2C3_Init();
  // MX_SPI1_Init();
  MX_SPI3_Init();
  // MX_USART1_UART_Init();
  MX_USB_Device_Init();

  //初始化TMUX GPIO
  TMUX_Global_Init();
  HAL_Delay(100);

  char msg[64];
  //从这里开始先注释
  // // 1. 电源上电序列
  // HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET);  // E5V
  // HAL_Delay(50);
  // HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);  // E3.3V
  // HAL_Delay(50);
  // HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET); // E4V
  // HAL_Delay(50); // 等待电源稳定

  // // [静态配置]
  // // KH: 常态连接 S3 (Channel 3: A2=0, A1=1, A0=0)
  // TMUX_KH_SetChannel(TMUX_CH_S2);
  // // KL: 常态连接 S2 (Channel 2: A2=0, A1=0, A0=1)
  // TMUX_KL_SetChannel(TMUX_CH_S3);
  // // KB: 初始状态设为断开
  // TMUX_KB_SetChannel(TMUX_CH_S7);

  // sprintf(msg, "GPIO Start config\r\n");
  // CDC_Transmit_FS2((uint8_t*)msg, strlen(msg));
  // HAL_Delay(100);

  // //2. AD4007 初始化
  // if (AD4007_Init_Safe() == HAL_OK) {
  //     strcpy(msg, "System Ready: AD4007 OK\r\n");
  // } else {
  //     strcpy(msg, "System Ready: AD4007 FAIL\r\n");
  // }
  // HAL_Delay(100); // 等待USB连接稳定
  // CDC_Transmit_FS2((uint8_t*)msg, strlen(msg));

  // // 3. 初始读取一次 ADC，验证功能
  // int32_t code = AD4007_Read_Single();
  // float voltage = AD4007_ConvertToVoltage(code);
  // sprintf(msg, "ADC:%.4f V\r\n", voltage);
  // CDC_Transmit_FS2((uint8_t*)msg, strlen(msg));  

  // HAL_Delay(100);

  // // 3.[修复 GPIO] 确保 PA8/PA9 复用为 HRTIM
  // GPIO_InitTypeDef GPIO_InitStruct = {0};
  // __HAL_RCC_GPIOA_CLK_ENABLE();
  
  // // PA8 -> HRTIM_CHA1, PA9 -> HRTIM_CHA2
  // GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  // GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;       // 复用推挽输出
  // GPIO_InitStruct.Pull = GPIO_NOPULL;
  // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  // GPIO_InitStruct.Alternate = GPIO_AF13_HRTIM1; // 必须是 AF13
  // HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  // // 初始化 HRTIM1
  // MX_HRTIM1_Init();
  

  // // 2. 启动 Timer A 的计数器，并使能中断
  // // 修改参数为 TIMERINDEX，并检查返回值
  // if (HAL_HRTIM_WaveformCountStart_IT(&hhrtim1, HRTIM_TIMERID_TIMER_A) != HAL_OK)
  // {
  //     Error_Handler(); // 如果启动失败，进入错误处理
  // }
  // else{
  //     sprintf(msg, "HRTIM Timer A started with interrupt!\r\n");
  //     CDC_Transmit_FS2((uint8_t*)msg, strlen(msg));
  // }

  // // 3. 启动 Timer A 的 PWM 输出 (TA1 和 TA2)
  // HAL_HRTIM_WaveformOutputStart(&hhrtim1,  HRTIM_OUTPUT_TA2);
  // HAL_HRTIM_WaveformOutputStart(&hhrtim1,  HRTIM_OUTPUT_TA1);

  //   // 1. 先启动 Master Timer (虽然它可能不输出波形，但它提供时基和复位信号)
  // HAL_HRTIM_WaveformCountStart_IT(&hhrtim1, HRTIM_TIMERID_MASTER);

  // sprintf(msg, "HRTIM Started System-Wide\r\n");
  // CDC_Transmit_FS2((uint8_t*)msg, strlen(msg));
  //从这里结束注释
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // while (1)
  // {
  //   /* USER CODE END WHILE */

  //   /* USER CODE BEGIN 3 */

  //   // --- 1. 原子读取全局变量 ---
  //   // 这里的变量在 HRTIM 中断中更新，读取时需关中断防止数据撕裂
  //   uint32_t temp_3us, temp_300us;
    
  //   __disable_irq(); // 进入临界区
  //   temp_3us = adc_raw_3us;
  //   temp_300us = adc_raw_300us;
  //   __enable_irq();  // 退出临界区

  //   // --- 2. 填充帧头 ---
  //   data_frame[0] = 0xAA;
  //   data_frame[1] = 0xBB;

  //   // --- 3. 填充数据 (各3字节, 大端模式 MSB First) ---
  //   // 取 temp_3us 的低24位
  //   data_frame[2] = (uint8_t)((temp_3us >> 16) & 0xFF);
  //   data_frame[3] = (uint8_t)((temp_3us >> 8) & 0xFF);
  //   data_frame[4] = (uint8_t)(temp_3us & 0xFF);

  //   // 取 temp_300us 的低24位
  //   data_frame[5] = (uint8_t)((temp_300us >> 16) & 0xFF);
  //   data_frame[6] = (uint8_t)((temp_300us >> 8) & 0xFF);
  //   data_frame[7] = (uint8_t)(temp_300us & 0xFF);

  //   // --- 4. 计算校验位 ---
  //   // 逻辑：(3us内部异或) ^ (300us内部异或) 等同于 所有6个字节直接异或
  //   uint8_t checksum = 0;
  //   for(int i = 2; i <= 7; i++) // 遍历 data_frame[2] 到 data_frame[7]
  //   {
  //       checksum ^= data_frame[i];
  //   }
  //   data_frame[8] = checksum;

  //   // --- 5. 填充帧尾 ---
  //   data_frame[9] = 0xCC;
  //   data_frame[10] = 0xDD;

  //   // --- 6. 发送数据 ---
  //   // 直接使用带环形缓冲的发送函数
  //   CDC_Transmit_FS2(data_frame, 11);

  //   // --- 7. 周期延时 ---
  //   HAL_Delay(1); // 10ms
    
  //   // if(num_3us == 10){
  //   //   HAL_HRTIM_WaveformOutputStart(&hhrtim1,  HRTIM_OUTPUT_TA1);
  //   // }

  // }

  //while1先注释
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      // if (data_ready_flag)
      // {
      //     data_ready_flag = 0; // 清除标志

      //     // --- 开始打包数据到缓存 ---
      //     uint8_t *p = &usb_tx_cache[sample_counter * PACKET_SIZE];
          
      //     // // 填充单点数据 (11字节)
      //     // p[0] = 0xAA; 
      //     // p[1] = (uint8_t)((current_adc_3us >> 24) & 0xFF);
      //     // p[2] = (uint8_t)((current_adc_3us >> 16) & 0xFF);
      //     // p[3] = (uint8_t)((current_adc_3us >> 8) & 0xFF);
      //     // p[4] = (uint8_t)(current_adc_3us & 0xFF);
      //     // p[5] = (uint8_t)((current_adc_300us >> 24) & 0xFF);
      //     // p[6] = (uint8_t)((current_adc_300us >> 16) & 0xFF);
      //     // p[7] = (uint8_t)((current_adc_300us >> 8) & 0xFF);
      //     // p[8] = (uint8_t)(current_adc_300us & 0xFF);
      //     // p[9] = 0x0D;
      //     // p[10] = 0x0A;
      //     // --- 2. 填充帧头 ---
      //     p[0] = 0xAA; 
      //     p[1] =  0xBB;
      //     p[2] = (uint8_t)((current_adc_3us >> 16) & 0xFF);
      //     p[3] = (uint8_t)((current_adc_3us >> 8) & 0xFF);
      //     p[4] = (uint8_t)(current_adc_3us & 0xFF);
      //     p[5] = (uint8_t)((current_adc_300us >> 16) & 0xFF);
      //     p[6] = (uint8_t)((current_adc_300us >> 8) & 0xFF);
      //     p[7] = (uint8_t)(current_adc_300us & 0xFF);
      //     uint8_t checksum = 0;
      //     for(int i = 2; i <= 7; i++) // 遍历 p[2] 到 p[7]
      //     {
      //         checksum ^= p[i];
      //     }
      //     p[8] = checksum;
      //     p[9] = 0xCC;
      //     p[10] = 0xDD;

      //     sample_counter++;

      //     // --- 当达到指定的批次数量时，通过 USB 发送一次 ---
      //     if (sample_counter >= BATCH_COUNT)
      //     {
      //         // 检查 CDC 发送状态，如果忙则循环等待或做丢包处理
      //         // 使用你的非阻塞发送函数 CDC_Transmit_FS2
      //         uint8_t result = CDC_Transmit_FS2(usb_tx_cache, TX_BUF_SIZE);
              
      //         if (result == USBD_OK) {
      //             sample_counter = 0; // 发送成功才清空计数器
      //         } else {
      //             // 如果 USB 忙，建议这里直接放弃这一包或者覆盖，防止主循环卡死
      //             sample_counter = 0; 
      //         }
      //     }
      // }
    sprintf(msg, "while\r\n");
    CDC_Transmit_FS2((uint8_t*)msg, strlen(msg));
    HAL_Delay(1000); 
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
// 处理 Compare Unit 2 事件 (对应配置的 504 ticks, 即 3.5us)
void HAL_HRTIM_Compare2EventCallback(HRTIM_HandleTypeDef *hhrtim, uint32_t TimerIdx)
{
    // 判断是否是 Timer A 产生的中断
    if (TimerIdx == HRTIM_TIMERINDEX_TIMER_A)
    {
      adc_raw_3us  = AD4007_Read_Single2();
      //adc_voltage_3us = AD4007_ConvertToVoltage_SPI(adc_raw_3us);
      // num_3us++; // 这里对应 3us 事件
    }
}

// 处理 Compare Unit 4 事件 (对应配置的 43272 ticks, 即 300.5us)
void HAL_HRTIM_Compare4EventCallback(HRTIM_HandleTypeDef *hhrtim, uint32_t TimerIdx)
{
    if (TimerIdx == HRTIM_TIMERINDEX_TIMER_A)
    {
        adc_raw_300us  = AD4007_Read_Single2();
        //adc_voltage_300us = AD4007_ConvertToVoltage_SPI(adc_raw_300us);
        //num_300us++;  // 这里对应 300us 事件
        // 2. 暂存当前一轮的所有数据（保证主循环拿到的数据是同一时刻的）
        current_adc_3us = adc_raw_3us;
        current_adc_300us = adc_raw_300us;

        // 3. 通知主循环有新数据
        data_ready_flag = 1;
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
