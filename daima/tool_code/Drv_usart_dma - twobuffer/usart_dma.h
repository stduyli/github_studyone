#ifndef __DRV_USART_H
#define __DRV_USART_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "Drv_delay.h"
#include "Drv_usb.h"

// 配置参数
#define USART0_DMA_SIZE      1024     // DMA缓冲区大小
#define USART0_BAUDRATE      115200  // 默认波特率
#define USART0_IRQ_PRIORITY  1       // 中断优先级
#define DMA_RX_IRQ_PRIORITY  2       // DMA接收中断优先级
#define DMA_TX_IRQ_PRIORITY  2       // DMA发送中断优先级

// 状态枚举
typedef enum {
    USART_OK = 0,           // 操作成功完成
    USART_ERROR,            // 硬件/底层错误（如寄存器操作失败、DMA异常等）
    USART_BUSY,             // 资源被占用（如前一次传输未完成）
    USART_TIMEOUT,          // 操作超时（如等待标志位超时）
    USART_INVALID_PARAM     // 参数非法（如NULL指针、长度越界）
} USART_Status;

// USART句柄结构
typedef struct {
    uint8_t tx_buffer[USART0_DMA_SIZE];
    uint8_t rx_buffer[2][USART0_DMA_SIZE];
    volatile uint8_t active_buffer;         // 当前活动缓冲区(0或1)
    volatile bool is_receiving;
    volatile bool is_transmitting;
    volatile uint32_t receive_len;
    uint32_t timeout;
} USART_HandleTypeDef;

// 函数声明
USART_Status Drv_usart0_init();
USART_Status USART_SendData(uint8_t *buffer, uint16_t len);
USART_Status USART_SendDataDMA(uint8_t *buffer, uint16_t len);
void USART_Printf(const char *format, ...);
void USART_ProcessReceivedData(void);
uint32_t USART_GetReceivedLength(void);
uint8_t* USART_GetReceiveBuffer(void);

// 回调函数类型定义
typedef void (*USART_RxCompleteCallback)(void);
typedef void (*USART_TxCompleteCallback)(void);

// 设置回调函数
void USART_SetRxCompleteCallback(USART_RxCompleteCallback callback);
void USART_SetTxCompleteCallback(USART_TxCompleteCallback callback);

#endif /* __DRV_USART_H */