# 嵌入式多任务系统模板笔记

[TOC]



## 一、概述

这是一个基于**任务调度系统**的嵌入式应用程序模板，使用了自定义的 

done_os

 操作系统框架。该模板展示了如何创建和管理多个并发任务，每个任务都有自己的执行周期和功能。



## 二、文件结构

```c
#include "app_main.h"
#include "gd32f4xx.h"        // MCU硬件抽象层
#include "done_os.h"         // 任务调度系统
#include "valdefine.H"       // 全局定义
#include "Drv_delay.h"       // 延时驱动
// ... 其他驱动头文件
```

## 三、任务定义模板

### 3.1 任务句柄声明

```c
static task_t task_name;  // 任务句柄
```

### 3.2 任务事件处理函数模板

```c
static void task_name_event(uint32_t event)
{
    static unsigned long task_tik = 0;
    
    // 定时检查 - 控制任务执行频率
    if ((delay_get() - task_tik) < TICK_VALUE_XXms) {
        return;
    }
    task_tik = delay_get();
    
    // 任务具体功能实现
    // ...
}
```

## 四、系统任务分类

### 4.1 主任务 (main_task)

- **执行周期**: 5ms

- 功能

  :

  - 按键处理
  - 蜂鸣器控制
  - 系统状态监控

### 4.2 显示任务 (lcd_task)

- **执行周期**: 100ms
- **功能**: LCD显示更新

### 4.3 通信任务

#### USB任务 (usb_task)

- **执行周期**: 100ms
- **功能**: USB CDC通信处理

#### WiFi任务 (wifi_task)

- **执行周期**: 100ms
- **功能**: WiFi数据收发处理

### 4.4 数据采集任务 (sample_task)

- **执行周期**: 5ms

- 功能

  :

  - 传感器数据采集
  - ADC采样
  - 电压/电流测量

### 4.5 消息处理任务 (message_task)

- **执行周期**: 10ms
- **功能**: 消息队列处理

### 4.6 电池管理任务 (batt_task)

- **执行周期**: 900ms
- **功能**: 电池状态监测

### 4.7 控制任务 (control_task)

- **执行周期**: 20ms

- 功能

  :

  - 启动流程控制
  - 电压测量控制

## 五、任务时间配置

```c
#define TICK_VALUE_5MS    5
#define TICK_VALUE_10MS   10
#define TICK_VALUE_20MS   20
#define TICK_VALUE_100MS  100
#define TICK_VALUE_900MS  900
```

## 六、初始化函数

```c
void app_main_init(void)
{
    // 创建所有任务
    done_task_create(&main_task, main_task_event);
    done_task_create(&message_task, message_task_event);
    done_task_create(&sample_task, sample_task_event);
    done_task_create(&lcd_task, lcd_task_event);
    done_task_create(&usb_task, usb_task_event);
    done_task_create(&wifi_task, wifi_task_event);
    done_task_create(&batt_task, batt_task_event);
    done_task_create(&control_task, control_task_event);
}
```

## 七、使用指南

### 7.1 添加新任务的步骤

1. **声明任务句柄**

```c
   static task_t new_task;
   
```

1. **实现任务事件函数**

```c
   static void new_task_event(uint32_t event)
   {
       static unsigned long new_tik = 0;
       if ((delay_get() - new_tik) < TICK_VALUE_XXms) {
           return;
       }
       new_tik = delay_get();
       
       // 任务功能代码
   }
   
```

1. **在初始化函数中创建任务**

```c
   done_task_create(&new_task, new_task_event);
   
```

### 7.2 任务优先级设计建议

- **高频任务** (5ms-20ms): 实时性要求高的控制和采样
- **中频任务** (50ms-200ms): 用户界面、通信处理
- **低频任务** (500ms-1s): 状态监测、日志记录

## 八、注意事项

1. **避免任务阻塞**: 任务中不应有长时间阻塞操作
2. **合理设置周期**: 根据实际需求设置任务执行周期
3. **资源互斥**: 多任务访问共享资源时需要考虑互斥保护
4. **堆栈大小**: 确保每个任务有足够的堆栈空间
5. **定时器精度**: delay_get() 的精度会影响任务调度精度

## 九、扩展功能建议

1. **任务间通信**: 实现消息队列或事件标志
2. **动态任务管理**: 支持运行时创建/删除任务
3. **任务监控**: 添加任务执行时间统计
4. **错误处理**: 添加任务异常处理机制
5. **低功耗支持**: 在空闲时进入低功耗模式

## 十、示例代码模板

```c
// 完整的任务模板示例
static task_t example_task;

static void example_task_event(uint32_t event)
{
    static unsigned long example_tik = 0;
    static uint8_t task_state = 0;
    
    // 定时执行控制
    if ((delay_get() - example_tik) < TICK_VALUE_50MS) {
        return;
    }
    example_tik = delay_get();
    
    // 状态机实现
    switch(task_state) {
        case 0:
            // 初始化状态
            task_state = 1;
            break;
            
        case 1:
            // 工作状态
            // 执行具体功能
            break;
            
        default:
            task_state = 0;
            break;
    }
}
```