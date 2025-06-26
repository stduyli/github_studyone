# 🎛 GD32 DAC波形发生器驱动文档

[TOC]

## 📌 核心组件
| 模块     | 配置                   | 功能描述         |
| -------- | ---------------------- | ---------------- |
| DAC0     | 12位分辨率, 缓冲使能   | 数模转换输出     |
| DMA0_CH5 | 循环模式, 超高速优先级 | 正弦波数据传输   |
| TIMER7   | 触发源                 | 控制DAC更新速率  |
| GPIOA4   | 模拟模式               | DAC_OUT0输出引脚 |

## 🛠 驱动实现

### 1. 正弦波数据表
```c
static const uint16_t sin_data[64] = {
    2046, 2041, ..., 2041  // 64点正弦波(12位分辨率)
};
// 对应电压: Vout = 3.3V * (val/4095)
```

### 2. DMA配置

```c
static void dma0_init() {
    dma_single_data_parameter_struct dma_cfg = {
        .direction = DMA_MEMORY_TO_PERIPH,
        .number = 64,
        .memory0_addr = (uint32_t)sin_data,
        .periph_addr = (uint32_t)&DAC0_DO,  // 0x40007408
        .circular_mode = DMA_CIRCULAR_MODE_ENABLE,
        // ...其他参数...
    };
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_cfg);
}
```

### 3. DAC-TIMER联动配置

```c
static void dac0_init() {
    dac_trigger_enable(DAC0, DAC_OUT0);
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T7_TRGO);
    dac_dma_enable(DAC0, DAC_OUT0);
}
```

## 📊 关键参数计算

### 输出频率公式

```math
f_{out} = \frac{f_{TIM7}}{N_{samples}}
```

其中：

- `f_TIM7`：定时器触发频率
- `N_samples`：波形点数(64)

### 示例配置

| 目标频率 | 所需TIM7频率 | 预分频配置 |
| :------- | :----------- | :--------- |
| 1kHz     | 64kHz        | PSC=839    |
| 10kHz    | 640kHz       | PSC=83     |

## 🚀 使用示例

### 基本波形输出

```c
void main() {
    dac_init();  // 初始化DAC+DMA
    
    // 配置TIM7为64kHz触发
    timer_auto_reload_value_config(TIMER7, 839);
    timer_enable(TIMER7);  // 开始输出1kHz正弦波
}
```

### 动态频率调整

```c
void SetWaveFrequency(uint32_t freq) {
    uint32_t timer_clk = 108000000;  // 假设系统时钟108MHz
    uint32_t arr_val = (timer_clk / (freq * 64)) - 1;
    timer_auto_reload_value_config(TIMER7, arr_val);
}
```

## ⚠️ 注意事项

1. **数据对齐要求**

   - 波形数据必须存储在CCM RAM或常规RAM
   - 数据地址需4字节对齐

2. **时序约束**

   | 参数        | 最大值 |
   | :---------- | :----- |
   | DMA传输时间 | 1μs    |
   | DAC建立时间 | 3μs    |
   | 最小周期    | 5μs    |

3. **功耗管理

   ```c
   void EnableOutput(bool enable) {
       enable ? dac_enable(DAC0, DAC_OUT0) 
              : dac_disable(DAC0, DAC_OUT0);
   }
   ```

## 🔍 高级应用

### 多波形切换

```c
enum WaveformType {SINE, TRIANGLE, SQUARE};
const uint16_t* GetWaveTable(WaveformType type) {
    static const uint16_t square_wave[64] = {...};
    // 返回不同波形指针
}
```

### 实时波形更新

```c
void UpdateWaveform(const uint16_t* new_data) {
    dma_channel_disable(DMA0, DMA_CH5);
    dma_memory_address_config(DMA0, DMA_CH5, new_data);
    dma_channel_enable(DMA0, DMA_CH5);
}
```

## 📈 性能优化

### 使用32点波形表

```c
#define WAVE_SIZE 32  // 内存减半，频率加倍
```

### 双缓冲技术

```c
uint16_t wave_buf[2][64];
// 在DMA传输一半和完成中断中切换缓冲区
```

## ⚡ 典型性能

| 指标          | 数值           |
| :------------ | :------------- |
| 最大输出频率  | 200kHz (32点)  |
| 波形更新延迟  | <2μs           |
| 谐波失真(THD) | <1% (缓冲使能) |
| 输出阻抗      | 50Ω            |

## 📖完整代码

### dac_dma.c

```c
#include "Drv_dac.h"
#include "lrc.h"

// 正弦波数据
static const uint16_t sin_data[sin_size] = {
    2046, 2041, 2026, 2001, 1968, 1925, 1873, 1813,
    1746, 1671, 1591, 1505, 1414, 1319, 1222, 1123,
    1022, 922, 823, 726, 631, 540, 454, 374,
    299, 232, 172, 120, 77, 44, 19, 4,
    0, 4, 19, 44, 77, 120, 172, 232,
    299, 374, 454, 540, 631, 726, 823, 922,
    1023, 1123, 1222, 1319, 1414, 1505, 1591, 1671,
    1746, 1813, 1873, 1925, 1968, 2001, 2026, 2041
};

// DMA0初始化(正弦波输出)
static void dma0_init()
{
    dma_single_data_parameter_struct dma_initpara;
    rcu_periph_clock_enable(RCU_DMA0);

    // 初始化DMA
    dma_deinit(DMA0, DMA_CH5);
    dma_flag_clear(DMA0, DMA_CH5, DMA_INTF_FTFIF);

    dma_initpara.direction = DMA_MEMORY_TO_PERIPH; // 内存到外设
    dma_initpara.number = sin_size;
    dma_initpara.memory0_addr = (uint32_t)sin_data;
    dma_initpara.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_initpara.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_initpara.periph_addr = (uint32_t)0x40007408; // 0x40007408
    dma_initpara.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_initpara.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_initpara.circular_mode = DMA_CIRCULAR_MODE_ENABLE;

    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_initpara);
    // 配置DMA通道子外设
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI7);

    // 使能DMA通道
    dma_channel_enable(DMA0, DMA_CH5);
}

// DAC0初始化
static void dac0_init()
{
    rcu_periph_clock_enable(RCU_DAC);
    rcu_periph_clock_enable(RCU_GPIOA);

    // 配置PA4为模拟模式（DAC_OUT0）
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);

    // 初始化DAC
    dac_deinit(DAC0);
    dac_trigger_enable(DAC0, DAC_OUT0);
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T7_TRGO);
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE); // 禁用波形生成
    dac_output_buffer_enable(DAC0, DAC_OUT0);
    dac_dma_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT0);
}

void dac_init()
{
	dma0_init();
	dac0_init();
}
```



### dac_dma.h

```c
#ifndef DRV_DAC_H
#define DRV_DAC_H

#include "gd32f4xx.h"

#define sin_size          64

void dac_init();

#endif 

```

