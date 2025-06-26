#include "Drv_usart.h"
#include "Drv_delay.h"

static USART_HandleTypeDef husart0;
static USART_RxCompleteCallback RxCompleteCallback = NULL;
static USART_TxCompleteCallback TxCompleteCallback = NULL;

// 私有函数声明
static USART_Status USART_GPIO_Init(void);
static USART_Status DMA_RX_Init(void);
static USART_Status DMA_TX_Init(void);
static void USART_Config(uint32_t baudrate);
static void MyRxCallback(void);

// 初始化USART
USART_Status Drv_usart_init() {
    USART_Status status;
    
    // 初始化GPIO
    status = USART_GPIO_Init();
    if(status != USART_OK) {
        return status;
    }
    
    // 配置USART
    USART_Config(USART0_BAUDRATE);
    
    // 初始化DMA接收
    status = DMA_RX_Init();
    if(status != USART_OK) {
        return status;
    }
    
    // 初始化DMA发送
    status = DMA_TX_Init();
    if(status != USART_OK) {
        return status;
    }
    USART_SetRxCompleteCallback(MyRxCallback);
    // 初始化句柄变量
    husart0.is_receiving = false;
    husart0.is_transmitting = false;
    husart0.receive_len = 0;
    husart0.timeout = 1000000; // 默认超时值
    
    return USART_OK;
}

// GPIO初始化
static USART_Status USART_GPIO_Init(void) {
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
    
    return USART_OK;
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
    
    // 硬件流控制禁用
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    
    // 使能接收和发送
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    
    // 使能DMA
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    
    // 使能USART
    usart_enable(USART0);
    
    // 配置中断
    nvic_irq_enable(USART0_IRQn, USART0_IRQ_PRIORITY, 0);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
}

// DMA接收初始化
static USART_Status DMA_RX_Init(void) {
    dma_single_data_parameter_struct dma_init_struct;
    
    // 使能DMA时钟
    rcu_periph_clock_enable(RCU_DMA1);
    
    // 复位DMA通道
    dma_deinit(DMA1, DMA_CH2);
    
    // 配置DMA参数
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)husart0.rx_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = USART0_DMA_SIZE;
    dma_init_struct.periph_addr = (uint32_t)(&USART_DATA(USART0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    
    // 初始化DMA
    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);
    
    // 配置DMA通道
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);
    
    // 使能DMA通道
    dma_channel_enable(DMA1, DMA_CH2);
    
    // 配置中断
    nvic_irq_enable(DMA1_Channel2_IRQn, DMA_RX_IRQ_PRIORITY, 0);
    
    return USART_OK;
}

// DMA发送初始化
static USART_Status DMA_TX_Init(void) {
    dma_single_data_parameter_struct dma_init_struct;
    
    // 使能DMA时钟
    rcu_periph_clock_enable(RCU_DMA1);
    
    // 复位DMA通道
    dma_deinit(DMA1, DMA_CH7);
    
    // 配置DMA参数
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)husart0.tx_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = USART0_DMA_SIZE;
    dma_init_struct.periph_addr = (uint32_t)(&USART_DATA(USART0));
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    
    // 初始化DMA
    dma_single_data_mode_init(DMA1, DMA_CH7, &dma_init_struct);
    
    // 禁用循环模式
    dma_circulation_disable(DMA1, DMA_CH7);
    
    // 配置DMA通道
    dma_channel_subperipheral_select(DMA1, DMA_CH7, DMA_SUBPERI4);
    
    // 使能中断
    dma_interrupt_enable(DMA1, DMA_CH7, DMA_CHXCTL_FTFIE);
    nvic_irq_enable(DMA1_Channel7_IRQn, DMA_TX_IRQ_PRIORITY, 0);
    
    return USART_OK;
}

// 通过DMA发送数据
USART_Status USART_SendDataDMA(uint8_t *buffer, uint16_t len) {
    // 参数检查
    if(buffer == NULL || len == 0 || len > USART0_DMA_SIZE) {
        return USART_INVALID_PARAM;
    }
    
    // 检查是否正在传输
    if(husart0.is_transmitting) {
        return USART_BUSY;
    }
    
    // 等待前一个传输完成(带超时)
    uint32_t timeout = husart0.timeout;
    while(husart0.is_transmitting) {
        if(--timeout == 0) {
            return USART_TIMEOUT;
        }
    }
    
    // 禁用DMA通道
    dma_channel_disable(DMA1, DMA_CH7);
    
    // 配置DMA
    dma_memory_address_config(DMA1, DMA_CH7, DMA_MEMORY_0, (uint32_t)buffer);
    dma_transfer_number_config(DMA1, DMA_CH7, len);
    
    // 重新使能DMA
    dma_channel_enable(DMA1, DMA_CH7);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    
    // 设置传输状态
    husart0.is_transmitting = true;
    
    return USART_OK;
}

// 普通方式发送数据
USART_Status USART_SendData(uint8_t *buffer, uint16_t len) {
    // 参数检查
    if(buffer == NULL || len == 0) {
        return USART_INVALID_PARAM;
    }
    
    for(uint16_t i = 0; i < len; i++) {
        // 等待发送寄存器空
        uint32_t timeout = husart0.timeout;
        while(usart_flag_get(USART0, USART_FLAG_TBE) == RESET) {
            if(--timeout == 0) {
                return USART_TIMEOUT;
            }
        }
        
        // 发送数据
        usart_data_transmit(USART0, buffer[i]);
    }
    
    // 等待传输完成
    uint32_t timeout = husart0.timeout;
    while(usart_flag_get(USART0, USART_FLAG_TC) == RESET) {
        if(--timeout == 0) {
            return USART_TIMEOUT;
        }
    }
    
    return USART_OK;
}

// printf实现
void USART_Printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int len = vsnprintf((char *)husart0.tx_buffer, USART0_DMA_SIZE, format, args);
    va_end(args);

    if(len > 0) {
        // 根据长度选择发送方式
        if(len > 64) {  // 大于64字节使用DMA
            USART_SendDataDMA(husart0.tx_buffer, len);
        } else {
            USART_SendData(husart0.tx_buffer, len);
        }
    }
}

// 获取接收缓冲区
uint8_t* USART_GetReceiveBuffer(void) {
    return husart0.rx_buffer;
}

// 获取接收数据长度
uint32_t USART_GetReceivedLength(void) {
    return husart0.receive_len;
}

// 处理接收到的数据
void USART_ProcessReceivedData(void) {
    if(husart0.is_receiving) {
        husart0.is_receiving = false;
        
        // 如果有回调函数，执行回调
        if(RxCompleteCallback != NULL) {
            RxCompleteCallback();
        }
    }
}

// 设置接收完成回调
void USART_SetRxCompleteCallback(USART_RxCompleteCallback callback) {
    RxCompleteCallback = callback;
}

// 设置发送完成回调
void USART_SetTxCompleteCallback(USART_TxCompleteCallback callback) {
    TxCompleteCallback = callback;
}

// USART空闲中断处理
void USART0_IRQHandler(void) {
    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE)) {
        // 清除空闲中断标志
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);
        (void)USART_DATA(USART0); // 读取数据寄存器清除状态
        
        // 计算接收到的数据长度
        husart0.receive_len = USART0_DMA_SIZE - dma_transfer_number_get(DMA1, DMA_CH2);
        husart0.is_receiving = true;
        
        // 重置DMA接收
        dma_channel_disable(DMA1, DMA_CH2);
        dma_flag_clear(DMA1, DMA_CH2, DMA_FLAG_FTF);
        dma_memory_address_config(DMA1, DMA_CH2, DMA_MEMORY_0, (uint32_t)husart0.rx_buffer);
        dma_transfer_number_config(DMA1, DMA_CH2, USART0_DMA_SIZE);
        dma_channel_enable(DMA1, DMA_CH2);
    }
}

// DMA发送完成中断
void DMA1_Channel7_IRQHandler(void) {
    if(dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INT_FLAG_FTF)) {
        // 清除中断标志
        dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_FLAG_FTF);
        
        // 更新传输状态
        husart0.is_transmitting = false;
        
        // 如果有回调函数，执行回调
        if(TxCompleteCallback != NULL) {
            TxCompleteCallback();
        }
    }
}

// 接收完成回调函数
static void MyRxCallback(void) {
    uint8_t *data = USART_GetReceiveBuffer();
    uint32_t len = USART_GetReceivedLength();
    
    // 处理接收到的数据
    // ...
    
    // 回显接收到的数据
    USART_SendDataDMA(data, len);
}
