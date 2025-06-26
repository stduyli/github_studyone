# USART0 驱动模块文档 (`Drv_usart0.h`)

[TOC]



## 概述

------

GD32微控制器的USART0驱动模块，提供串口初始化、数据收发及中断处理功能。

## 功能特性

------



- 支持可配置波特率
- 8位数据位、1位停止位、无校验位
- 中断接收模式
- 标准输出重定向(`fputc`)

## 函数说明

------



### 初始化函数

```c
void Drv_usart0_init()
```

- 功能：初始化USART0外设
- 调用关系：
  - `USART_GPIO_Init()`: 初始化GPIO
  - `USART_Config()`: 配置串口参数

### 私有函数

```c
static void USART_GPIO_Init(void)
```

- 配置PA9(TX)和PA10(RX)为USART功能引脚
- 时钟使能：GPIOA和USART0
- 引脚模式：复用功能、上拉、50MHz速度(TX)

```c
static void USART_Config(uint32_t baudrate)
```

- 参数：`baudrate` - 波特率(如115200)
- 配置项：
  - 8位数据位/1位停止位/无校验
  - 禁用硬件流控
  - 使能接收中断(RBNE)

### 标准输出重定向

```c
int fputc(int ch, FILE *f)
```

- 实现`printf`输出重定向到USART0
- 阻塞等待发送完成

## 中断处理

------



```c
void USART0_IRQHandler(void)
```

- 中断类型：接收缓冲区非空中断(RBNE)
- 处理流程：
  1. 读取接收到的字节
  2. 回显数据(发送相同字节)
  3. 清除中断标志

## 典型配置参数

------



```c
#define USART0_BAUDRATE 115200  // 常用波特率
```

## 使用示例

------



1. 初始化：

```c
Drv_usart0_init();
```

1. 使用标准输出：

```c
printf("Hello USART0\r\n");
```

## 注意事项

------



```tex
1. 需在工程设置中勾选"Use MicroLIB"以支持printf

2. 中断优先级配置在`nvic_irq_enable()`中设置(当前为0)

3. 如需修改流控/校验等参数，需修改`USART_Config()`函数

```



## usart.c

------



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
    }
}



```

## usart.h

------

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

