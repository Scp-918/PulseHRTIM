#include "MAX30101.h"
#include <stdio.h>

// 简单的I2C写封装
uint8_t MAX30101_WriteReg(uint8_t reg, uint8_t data) {
    // 使用HAL库的阻塞式写函数
    // Timeout设置为10ms
    if (HAL_I2C_Mem_Write(&hi2c3, MAX30101_I2C_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 10) != HAL_OK) {
        return 0; // 失败
    }
    return 1; // 成功
}

// 简单的I2C读封装
uint8_t MAX30101_ReadReg(uint8_t reg, uint8_t *pData) {
    if (HAL_I2C_Mem_Read(&hi2c3, MAX30101_I2C_ADDR_READ, reg, I2C_MEMADD_SIZE_8BIT, pData, 1, 10) != HAL_OK) {
        return 0; // 失败
    }
    return 1; // 成功
}

// 读取FIFO数据 (Burst Read)
uint8_t MAX30101_ReadFIFO(uint8_t *buffer, uint16_t length) {
    if (HAL_I2C_Mem_Read(&hi2c3, MAX30101_I2C_ADDR_READ, REG_FIFO_DATA, I2C_MEMADD_SIZE_8BIT, buffer, length, 50) != HAL_OK) {
        return 0;
    }
    return 1;
}

// 初始化函数
uint8_t MAX30101_Init(void) {
    uint8_t id = 0;
    
    // 1. 检查ID，确认I2C通信正常
    if (!MAX30101_ReadReg(REG_PART_ID, &id)) {
        return id; // I2C错误
    }
    if (id != 0x15) { // MAX30101的Part ID通常是0x15
        // 注意：有些版本可能是0x11(MAX30105)等，视具体芯片而定，这里假设0x15
        // 如果读到0x00或0xFF，说明通讯失败
    }

    // 2. 复位
    MAX30101_WriteReg(REG_MODE_CONFIG, 0x40); 
    HAL_Delay(10);

    // 3. 配置FIFO
    // SMP_AVE=010 (4 samples avg), FIFO_ROLLOVER_EN=1
    MAX30101_WriteReg(REG_FIFO_CONFIG, 0x50); 

    // 4. 配置模式
    // Mode = 0x07 (Multi-LED Mode) 用于绿光/红光切换
    MAX30101_WriteReg(REG_MODE_CONFIG, 0x07); 

    // 5. 配置SpO2
    // SPO2_ADC_RGE=01 (4096nA), SPO2_SR=001 (100Hz), LED_PW=11 (411us, 18-bit)
    MAX30101_WriteReg(REG_SPO2_CONFIG, 0x27); 

    // 6. 配置LED电流 (默认先关，测试时再开)
    MAX30101_WriteReg(REG_LED1_PA, 0x00);
    MAX30101_WriteReg(REG_LED2_PA, 0x00);
    MAX30101_WriteReg(REG_LED3_PA, 0x00);

    // 7. 配置时间槽 (Multi-LED Control)
    // Slot1 = LED1 (Red), Slot2 = LED2 (IR)
    MAX30101_WriteReg(REG_MULTI_LED_CTRL1, 0x21); 
    // Slot3 = LED3 (Green), Slot4 = Disabled
    MAX30101_WriteReg(REG_MULTI_LED_CTRL2, 0x03); 
    
    // 8. 清除指针
    MAX30101_WriteReg(REG_FIFO_WR_PTR, 0x00);
    MAX30101_WriteReg(REG_OVF_COUNTER, 0x00);
    MAX30101_WriteReg(REG_FIFO_RD_PTR, 0x00);

    return 1;
}

uint8_t MAX30101_GetPartID(void) {
    uint8_t id;
    if(MAX30101_ReadReg(REG_PART_ID, &id)) {
        return id;
    }
    return 0x00; // Error
}