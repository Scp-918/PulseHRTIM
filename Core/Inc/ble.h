#ifndef __BLE_H__
#define __BLE_H__

#include "main.h"
#include "usart.h"

/* 用户参数设置 */
#define BLE_TARGET_BAUD  460800  // 目标波特率
#define BLE_DEFAULT_BAUD 19200   // HJ131出厂默认波特率
#define BLE_PREV_BAUD    115200  // 上次代码使用的波特率

/* 引脚定义 (参考Excel连接表) */
#define BLE_RST_PORT     GPIOE
#define BLE_RST_PIN      GPIO_PIN_1
#define BLE_STATE_PORT   GPIOE
#define BLE_STATE_PIN    GPIO_PIN_0

/* 函数声明 */
void BLE_System_Init(void);      // 系统级初始化(GPIO等)
void BLE_Run_Test_Cycle(void);   // 放在while中循环调用的测试函数
void BLE_Send_Data(const char *data); // <--- 新增：发送数据函数

#endif /* __BLE_H__ */