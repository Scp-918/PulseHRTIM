#ifndef __MAX30101_H
#define __MAX30101_H

#include "main.h" // 包含HAL库定义

// I2C 句柄声明，假设CubeMX生成的句柄为 hi2c1
extern I2C_HandleTypeDef hi2c3;

// MAX30101 I2C地址 (7-bit address shifted left by 1)
#define MAX30101_I2C_ADDR_WRITE  0xAE
#define MAX30101_I2C_ADDR_READ   0xAF

// 寄存器地址定义
#define REG_INTR_STATUS_1        0x00
#define REG_INTR_STATUS_2        0x01
#define REG_INTR_ENABLE_1        0x02
#define REG_INTR_ENABLE_2        0x03
#define REG_FIFO_WR_PTR          0x04
#define REG_OVF_COUNTER          0x05
#define REG_FIFO_RD_PTR          0x06
#define REG_FIFO_DATA            0x07
#define REG_FIFO_CONFIG          0x08
#define REG_MODE_CONFIG          0x09
#define REG_SPO2_CONFIG          0x0A
#define REG_LED1_PA              0x0C // Red
#define REG_LED2_PA              0x0D // IR
#define REG_LED3_PA              0x0E // Green
#define REG_LED4_PA              0x0F
#define REG_PILOT_PA             0x10
#define REG_MULTI_LED_CTRL1      0x11
#define REG_MULTI_LED_CTRL2      0x12
#define REG_TEMP_INT             0x1F
#define REG_TEMP_FRAC            0x20
#define REG_TEMP_CONFIG          0x21
#define REG_PROX_INT_THRESH      0x30
#define REG_REV_ID               0xFE
#define REG_PART_ID              0xFF

// 函数原型
uint8_t MAX30101_Init(void);
uint8_t MAX30101_WriteReg(uint8_t reg, uint8_t data);
uint8_t MAX30101_ReadReg(uint8_t reg, uint8_t *pData);
uint8_t MAX30101_ReadFIFO(uint8_t *buffer, uint16_t length);
uint8_t MAX30101_GetPartID(void);

#endif