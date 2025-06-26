# 📡 GD32 USART0 驱动文档

[TOC]



## 🌟 概述

本驱动实现GD32F4xx系列MCU的USART0串口通信功能，支持：
- 全双工异步通信
- 115200bps默认波特率
- 8位数据位/1位停止位/无校验
- 中断接收模式
- printf重定向支持

## 📌 硬件配置
| 功能 | 引脚 | 模式         | 速率  | 复用功能 |
| ---- | ---- | ------------ | ----- | -------- |
| TX   | PA9  | 复用推挽输出 | 50MHz | AF7      |
| RX   | PA10 | 复用浮空输入 | -     | AF7      |

## 🛠 驱动实现

### 1. 初始化流程
```c
void Drv_usart0_init() {
    USART_GPIO_Init();      // GPIO初始化
    USART_Config(115200);   // USART参数配置
}
```

### 2. GPIO配置

```c
static void USART_GPIO_Init(void) {
    // 时钟使能
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);
    
    // 引脚复用配置
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
    
    // TX配置（输出）
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    
    // RX配置（输入）
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
}
```

### 3. USART参数配置

```c
static void USART_Config(uint32_t baudrate) {
    // 复位USART
    usart_deinit(USART0);
    
    // 基本参数配置
    usart_word_length_set(USART0, USART_WL_8BIT);    // 8位数据
    usart_stop_bit_set(USART0, USART_STB_1BIT);      // 1位停止位
    usart_parity_config(USART0, USART_PM_NONE);      // 无校验
    usart_baudrate_set(USART0, baudrate);            // 波特率设置
    
    // 硬件流控制禁用
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    
    // 使能收发功能
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    
    // 中断配置
    nvic_irq_enable(USART0_IRQn, 0, 0);
    usart_interrupt_enable(USART0, USART_INT_RBNE);  // 接收缓冲区非空中断
    
    // 使能USART
    usart_enable(USART0);
}
```

## 📊 关键特性

### 通信参数配置

| 参数     | 可选值           | 当前配置  |
| :------- | :--------------- | :-------- |
| 数据位   | 8/9位            | 8位       |
| 停止位   | 0.5/1/1.5/2位    | 1位       |
| 校验方式 | 无/奇校验/偶校验 | 无校验    |
| 波特率   | 1200-6000000bps  | 115200bps |

### 中断类型

| 中断源         | 描述           |
| :------------- | :------------- |
| USART_INT_RBNE | 接收缓冲区非空 |
| USART_INT_TBE  | 发送缓冲区空   |
| USART_INT_TC   | 传输完成       |
| USART_INT_PERR | 校验错误       |

## 🚀 使用示例

### 1. printf重定向

```c
// 重定义fputc函数
int fputc(int ch, FILE *f) {
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}

// 使用示例
printf("System Ready!\r\n");
```

### 2. 中断接收处理

```c
void USART0_IRQHandler(void) {
    if(usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)) {
        uint8_t ch = usart_data_receive(USART0);
        
        // 回显数据
        usart_data_transmit(USART0, ch);
        
        // 清除中断标志
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
    }
}
```

### 3. 数据发送函数

```c
void USART_SendString(char *str) {
    while(*str) {
        usart_data_transmit(USART0, *str++);
        while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    }
}
```

## ⚠️ 注意事项

1. **波特率计算**：

   ```math
   BR = \frac{f_{CK}}{16 \times USARTDIV}
   ```

   - 需根据系统时钟精确计算

2. **中断优先级**：

   - 根据系统需求合理配置NVIC优先级

3. **缓冲区管理**：

   - 长时间通信建议增加环形缓冲区

4. **电气特性**：

   - 确保信号电平匹配（3.3V TTL）

## 🔍 调试技巧

1. **信号测量**：
   - 使用逻辑分析仪观察TX/RX波形
   - 检查起始位/停止位是否正常
2. **常见问题**：
   - 无通信：检查GPIO复用配置
   - 乱码：检查波特率和时钟配置
   - 数据丢失：增加流控或降低波特率
3. **性能优化**：
   - 使用DMA传输大数据块
   - 适当提高中断优先级

## 📈 性能参数

| 参数         | 数值              |
| :----------- | :---------------- |
| 最大波特率   | 6Mbps             |
| 中断响应时间 | <1μs              |
| 字符传输时间 | 86.8μs @115200bps |
| 传输效率     | >98%              |

## 📖完整代码

### usart.c

```c
#include "Drv_usart0.h"


// 私有函数声明
static void USART_GPIO_Init(void);
static void USART_Config(uint32_t baudrate);

// 初始化USART
void Drv_usart0_init() 
{
    USART_GPIO_Init();
    USART_Config(USART0_BAUDRATE);
}

// GPIO初始化
static void USART_GPIO_Init(void) {
    // 使能时钟
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);
    
    // 配置USART0 TX (PA9) 和 RX (PA10)
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
    
    // TX引脚配置
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    
    // RX引脚配置
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
   
}

// USART配置
static void USART_Config(uint32_t baudrate) {
    // 复位USART
    usart_deinit(USART0);
    
    // 配置USART参数
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_baudrate_set(USART0, baudrate);
        
    // 使能接收和发送
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
	
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    // 使能USART
    usart_enable(USART0);
    
    // 配置中断
    nvic_irq_enable(USART0_IRQn, 0, 0);
    usart_interrupt_enable(USART0, USART_INT_RBNE);
}

int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}

// USART空闲中断处理
void USART0_IRQHandler(void) {
    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)) {
        // 清除空闲中断标志
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
        
		uint8_t ch = usart_data_receive(USART0);
		// 回显接收到的数据
        usart_data_transmit(USART0, ch);
//		usb_cdc_write_string((char *)ch);
    }
}
```

### usart.h

```c
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
#define USART0_BAUDRATE      115200  // 默认波特率


// 函数声明
void Drv_usart0_init();


#endif /* __DRV_USART_H */
```

