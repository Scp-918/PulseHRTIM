#include "ble.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* 声明外部USB发送函数，确保链接正确 */
extern uint8_t CDC_Transmit_FS2(uint8_t* Buf, uint16_t Len);

/* 内部缓冲区 */
static uint8_t ble_rx_buffer[128];
static uint8_t usb_tx_buffer[256];

/**
  * @brief  初始化BLE相关的非UART GPIO (PE0, PE1)
  * 由于gpio.c未配置这些引脚，需在此处补充
  */
void BLE_System_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 开启GPIOE时钟 */
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* 配置PE1 (BLE_RST) 为输出 */
    HAL_GPIO_WritePin(BLE_RST_PORT, BLE_RST_PIN, GPIO_PIN_RESET); // 默认拉低(非复位状态)
    GPIO_InitStruct.Pin = BLE_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BLE_RST_PORT, &GPIO_InitStruct);

    /* 配置PE0 (BLE_STATE) 为输入 */
    GPIO_InitStruct.Pin = BLE_STATE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BLE_STATE_PORT, &GPIO_InitStruct);
}

/**
  * @brief  动态修改USART1波特率
  */
static void BLE_Set_UART_Baud(uint32_t baudrate)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = baudrate;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        // 初始化失败处理
    }
}

/**
  * @brief  发送指令并等待特定回复
  * @param  cmd: 发送的指令字符串
  * @param  expected_resp: 期望收到的回复子串
  * @param  timeout: 超时时间(ms)
  * @return 1:成功, 0:失败
  */
static uint8_t BLE_SendCmd_Check(char *cmd, char *expected_resp, uint32_t timeout)
{
    // 1. 清空接收缓冲
    memset(ble_rx_buffer, 0, sizeof(ble_rx_buffer));
    
    // 2. 发送指令
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
    
    // 3. 接收回复 (阻塞式接收，用于初始化检测)
    // 注意：实际项目中建议用中断或DMA，此处为了流程清晰使用超时接收
    HAL_UART_Receive(&huart1, ble_rx_buffer, sizeof(ble_rx_buffer) - 1, timeout);
    
    // 4. 检查是否包含期望字符串
    if (strstr((char*)ble_rx_buffer, expected_resp) != NULL)
    {
        return 1;
    }
    return 0;
}

/**
  * @brief  USB 打印辅助函数
  */
static void USB_Printf(const char *format, ...)
{
    va_list args;
    uint16_t len;
    
    va_start(args, format);
    len = vsnprintf((char*)usb_tx_buffer, sizeof(usb_tx_buffer), format, args);
    va_end(args);
    
    CDC_Transmit_FS2(usb_tx_buffer, len);
}

/**
  * @brief  核心测试循环：硬件复位 -> 配置 -> 检查 -> 上报
  */
void BLE_Run_Test_Cycle(void)
{
    uint8_t step_wake = 0;
    uint8_t step_baud = 0;
    uint8_t step_power = 0;
    uint8_t step_name = 0;
    char temp_cmd[32];

    USB_Printf("=== Start BLE Init Process (Target: %d) ===\r\n", BLE_TARGET_BAUD);

    // --- 步骤 1: 硬件复位 HJ131 ---
    // 手册：P00R高电平有效，持续1ms以上
    HAL_GPIO_WritePin(BLE_RST_PORT, BLE_RST_PIN, GPIO_PIN_SET); 
    HAL_Delay(10); 
    HAL_GPIO_WritePin(BLE_RST_PORT, BLE_RST_PIN, GPIO_PIN_RESET);
    // 等待模块启动，手册建议复位后有一定延时
    HAL_Delay(600); 

    // --- 步骤 2: 尝试同步波特率 ---
    // 策略：我们不知道模块当前处于什么波特率，尝试常见波特率发送唤醒/更改指令
    
    // 2.1 尝试以默认19200连接并切换到目标波特率
    BLE_Set_UART_Baud(BLE_DEFAULT_BAUD);
    // 发送唤醒字节(0xAA)以防模块休眠
    uint8_t wake_bytes[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    HAL_UART_Transmit(&huart1, wake_bytes, 5, 10);
    HAL_Delay(20);
    // 发送更改波特率指令
    sprintf(temp_cmd, "<ST_BAUD=%d>", BLE_TARGET_BAUD);
    HAL_UART_Transmit(&huart1, (uint8_t*)temp_cmd, strlen(temp_cmd), 50);
    HAL_Delay(50); // 等待模块处理

    // 2.2 尝试以旧代码115200连接并切换 (以防上次设置了此值)
    BLE_Set_UART_Baud(BLE_PREV_BAUD);
    HAL_UART_Transmit(&huart1, wake_bytes, 5, 10);
    HAL_Delay(20);
    HAL_UART_Transmit(&huart1, (uint8_t*)temp_cmd, strlen(temp_cmd), 50);
    HAL_Delay(50);

    // 2.3 最终将MCU切换到目标波特率 460800 进行正式配置
    BLE_Set_UART_Baud(BLE_TARGET_BAUD);
    HAL_Delay(50);

    // --- 步骤 3: 发送配置指令并检查质量 ---
    
    // 3.1 设置全速工作模式 (Wake Forever)
    // 唤醒前先发几个Dummy Byte确保Rx唤醒
    HAL_UART_Transmit(&huart1, wake_bytes, 5, 10); 
    HAL_Delay(5);
    step_wake = BLE_SendCmd_Check("<ST_WAKE=FOREVER>", "ok", 200);

    // 3.2 确认波特率 (此时应该已经是460800了，发送查询指令确认)
    // 或者是再次发送设置指令看是否回复ok
    step_baud = BLE_SendCmd_Check(temp_cmd, "ok", 200); 
    
    // 如果失败，尝试读取当前波特率
    if(step_baud == 0) {
        step_baud = BLE_SendCmd_Check("<RD_BAUD>", "rd_baud", 200);
    }

    // 3.3 设置发射功率 +2.5dBm
    step_power = BLE_SendCmd_Check("<ST_TX_POWER=+2.5>", "ok", 200);

    // 3.4 设置名称
    step_name = BLE_SendCmd_Check("<ST_NAME=HJ-G474-TEST>", "ok", 200);

    // --- 步骤 4: 通过USB上报结果 ---
    USB_Printf("Init Result:\r\n");
    USB_Printf("1. Mode(Forever): %s\r\n", step_wake ? "Success" : "Fail");
    USB_Printf("2. Baud(%d):  %s\r\n", BLE_TARGET_BAUD, step_baud ? "Success" : "Fail");
    USB_Printf("3. Power(+2.5):   %s\r\n", step_power ? "Success" : "Fail");
    USB_Printf("4. Name Set:      %s\r\n", step_name ? "Success" : "Fail");
    USB_Printf("------------------------------\r\n");
}

void BLE_Send_Data(const char *data)
{
    if (data == NULL) return;
    
    // 直接通过 USART1 发送，HJ131会自动将非指令数据透传
    HAL_UART_Transmit(&huart1, (uint8_t*)data, strlen(data), 100);
}