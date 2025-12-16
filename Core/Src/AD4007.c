#include "AD4007.h"
#include "spi.h" 
#include <stdio.h>

extern SPI_HandleTypeDef hspi3;

// 简单的微秒延时
static void AD4007_Delay_Short(void)
{
    for(volatile int i=0; i<1000; i++) { __NOP(); }
}

static void AD4007_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET); 

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

HAL_StatusTypeDef AD4007_Init_Safe(void)
{
    uint8_t cfg_write_cmd[2];
    uint8_t rx_buffer[3] = {0};
    uint8_t tx_dummy[3] = {0xFF, 0xFF, 0xFF}; // 发送全1，确保 MOSI 为高
    uint8_t retry_count = 0;
    const uint8_t MAX_RETRIES = 200;

    AD4007_GPIO_Init();

    // 唤醒序列
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    AD4007_Delay_Short();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    AD4007_Delay_Short();
    
    // 清空总线 (使用 TransmitReceive 发送 FF)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi3, tx_dummy, rx_buffer, 3, 10); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET); // 此时 MOSI 为高，保持 CS 模式
    AD4007_Delay_Short();

    // 配置命令：Bit 0 必须为 1
    cfg_write_cmd[0] = AD4007_CMD_WRITE_CFG; 
    cfg_write_cmd[1] = 0x11; // 0x11: Status Enable + SDI Keep High

    for (retry_count = 0; retry_count < MAX_RETRIES; retry_count++)
    {
        // 1. 写入配置
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(&hspi3, cfg_write_cmd, 2, 10) != HAL_OK)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
            continue;
        }
        // 关键：cfg_write_cmd[1] 最后一位是 1，CNV 上升时 SDI 为高 -> 锁定 CS 模式
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET); 

        AD4007_Delay_Short();

        // 2. 启动验证转换
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
        __NOP(); __NOP(); __NOP(); 
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
        
        AD4007_Delay_Short(); 

        // 3. 读取验证 (发送 0xFF)
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
            
            if (HAL_SPI_TransmitReceive(&hspi3, tx_dummy, rx_buffer, 3, 10) == HAL_OK)
            {
                // [修复]：0x00 是代表 0V 的有效数据，不应视为错误。
                // 只要不是全 0xFF (通常意味着 MISO 悬空高电平)，就认为 SPI 连通了。
                // 如果你想更严谨，可以检查 rx_buffer[2] 的低位状态，但仅排除 0xFF 通常足够。
                
                if (rx_buffer[0] != 0xFF) 
                {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
                    return HAL_OK; 
                }
            }
            
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
            HAL_Delay(10);
        }

    return HAL_ERROR;
}

int32_t AD4007_Read_Single(void)
{
    uint8_t rx_data[3] = {0};
    uint8_t tx_dummy[3] = {0xFF, 0xFF, 0xFF}; // 关键：发送全1
    int32_t adc_val = 0;

    // 1. 启动转换 (CNV 上升沿)
    // 此时 MOSI 应该是高 (由上一次读取的 0xFF 保持)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET); 
    
    // 2. 转换等待
    for(volatile int i=0; i<500; i++) { __NOP(); } 

    // 3. 读取数据
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    
    // [修正] 使用 TransmitReceive 发送 0xFF，强迫 MOSI 拉高
    if (HAL_SPI_TransmitReceive(&hspi3, tx_dummy, rx_data, 3, 5) != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
        return 0;
    }
    
    // 4. 结束读取 (CNV 上升沿)
    // 因为刚才发送了 0xFF，此时 MOSI 为高。
    // AD4007 检测到 SDI=1，继续保持 CS 模式，下一次转换正常。
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

    // 5. 数据处理
    uint32_t raw = ((uint32_t)rx_data[0] << 16) | ((uint32_t)rx_data[1] << 8) | rx_data[2];
    
    // 18位数据在高位，右移6位对齐 (假设读取24位，后6位为状态)
    raw = raw >> 6; 
    raw &= 0x3FFFF;

    if (raw & 0x20000) 
    {
        adc_val = raw | 0xFFFC0000;
    }
    else
    {
        adc_val = raw;
    }

    return adc_val;
}

float AD4007_ConvertToVoltage(int32_t code)
{
    return ((float)code * AD4007_VREF) / 131072.0f;
}

/* 读取函数：假设 CNV 已经由 HRTIM 触发并完成转换 */
/* * 函数功能：在HRTIM中断中调用，快速读取SPI FIFO中的AD转换结果
 * 返回值：经过移位对齐和符号扩展后的 18-bit 有符号整数 (int32_t)
 */
/**
 * @brief  在 HRTIM 中断中调用，快速读取 SPI 数据
 * @return 右移 6 位对齐后的 18-bit 原始数据 (无符号，未进行符号扩展)
 * 数据范围: 0x00000 ~ 0x3FFFF
 */
uint32_t AD4007_Read_SPI_Only(void)
{
    SPI_TypeDef *SPIx = hspi3.Instance; // 获取 SPI3 寄存器基地址
    uint32_t rx_val = 0;
    const uint8_t tx_dummy = 0xFF;      // 发送空字节产生时钟

    // 1. 确保 SPI 已使能 (双重保险，通常初始化已开启)
    if((SPIx->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
    {
        __HAL_SPI_ENABLE(&hspi3);
    }

    // 2. 连续读取 3 个字节 (24 Clocks)
    // 直接操作 DR 寄存器，避免 HAL 库函数调用的开销
    for (int i = 0; i < 3; i++)
    {
        // 等待发送缓冲区空 (TXE)
        // 注意：在极短的中断中通常不加超时，若担心硬件故障可自行添加简易计数器
        while (!((SPIx->SR) & SPI_FLAG_TXE)); 
        
        // 发送数据，启动时钟
        *((__IO uint8_t *)&SPIx->DR) = tx_dummy;

        // 等待接收缓冲区非空 (RXNE)
        while (!((SPIx->SR) & SPI_FLAG_RXNE));
        
        // 读取数据并拼接到 rx_val
        // 第一次读: High Byte, 第二次: Mid Byte, 第三次: Low Byte
        rx_val = (rx_val << 8) | (*((__IO uint8_t *)&SPIx->DR));
    }

    // 3. 数据对齐处理
    // 原始数据(24bit): [D17...D0] [0 0 0 0 0 0]
    // 右移 6 位:       [0 0 0 0 0 0] [D17...D0]
    rx_val = rx_val >> 6;

    // 4. 屏蔽高位垃圾数据 (保留低 18 位)
    // 此时得到的是纯粹的 18-bit ADC Code
    return (rx_val & 0x3FFFF);
}

/**
 * @brief  将 18-bit 对齐后的原始数据转换为实际电压值
 * @param  adc_raw_code: 由 AD4007_Read_SPI_Only 返回的 18-bit 数据
 * @return 实际电压值 (float)，单位 V
 */
float AD4007_ConvertToVoltage_SPI(uint32_t adc_raw_code)
{
    int32_t s_code;

    // 1. 符号扩展 (Sign Extension)
    // AD4007 输出为补码格式。
    // 检查 Bit 17 (第18位)，如果是 1，说明是负数
    if (adc_raw_code & 0x20000) 
    {
        // 是负数：将 32位整数的高 14 位 (Bit 18-31) 全部置 1
        // 0xFFFC0000 对应二进制 1111 1111 1111 1100 ...
        s_code = (int32_t)(adc_raw_code | 0xFFFC0000);
    }
    else
    {
        // 是正数：直接转换
        s_code = (int32_t)adc_raw_code;
    }

    // 2. 转换为电压
    // 公式: Voltage = (Code * VREF) / 2^(N-1)
    // AD4007 是 18位，分母为 2^17 = 131072
    // 请确保 AD4007_VREF 宏已定义，例如 5.0f 或 4.096f
    return ((float)s_code * AD4007_VREF) / 131072.0f;
}

/**
 * @brief  手动控制 CNV 并读取数据 (用于主循环验证)
 * @return 右移 6 位对齐后的 18-bit 原始数据 (与 Read_SPI_Only 返回一致)
 */
uint32_t AD4007_Read_Single2(void)
{
    // 1. 控制 CNV 引脚启动转换 (Rising Edge)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET); 
    
    // 保持高电平至少 10ns (tCNVH)，这里给一点延时
    for(volatile int i=0; i<10; i++) __NOP(); 

    // 2. 等待转换完成 (tCONV)
    // AD4007 的转换时间约为 300ns 左右，根据系统时钟调整循环次数
    // 系统时钟若为 170MHz，300ns 约为 50 个指令周期
    for(volatile int i=0; i<60; i++) __NOP(); 

    // 3. 拉低 CNV 进入采集/读取模式 (CS Mode)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET); 
    
    // 给一点建立时间 (tCSCNV 等)
    __NOP(); __NOP();

    // 4. 调用 SPI 读取函数获取数据
    // 注意：这里获得的是无符号的 18-bit Raw Code
    uint32_t raw_data = AD4007_Read_SPI_Only();

    // 5. 恢复 CNV 为高电平 (准备下一次，或进入空闲)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET); 

    return raw_data;
}