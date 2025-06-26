## 📌 I2C驱动代码解析（软件模拟实现）

------

[TOC]

------



### **1️⃣ 硬件配置与宏定义**

#### 📌 引脚定义 (I2C.h)
```c
/* I2C GPIO端口与引脚定义 */
#define IIC_GPIO       GPIOB
#define IIC_SCL_PIN    GPIO_PIN_8  // PB8作为SCL
#define IIC_SDA_PIN    GPIO_PIN_9  // PB9作为SDA

/* 电平控制宏 */
#define IIC_SCL_L      gpio_bit_write(IIC_GPIO, IIC_SCL_PIN, 0) // SCL拉低
#define IIC_SCL_H      gpio_bit_write(IIC_GPIO, IIC_SCL_PIN, 1) // SCL拉高
#define IIC_SDA_L      gpio_bit_write(IIC_GPIO, IIC_SDA_PIN, 0) // SDA拉低
#define IIC_SDA_H      gpio_bit_write(IIC_GPIO, IIC_SDA_PIN, 1) // SDA拉高
#define READ_SDA       gpio_input_bit_get(IIC_GPIO, IIC_SDA_PIN) // 读取SDA状态

/* SDA方向控制 */
#define SDA_IN()  gpio_mode_set(IIC_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, IIC_SDA_PIN)  // 输入模式
#define SDA_OUT() gpio_mode_set(IIC_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, IIC_SDA_PIN);  // 输出模式
```

---

### **2️⃣ 核心信号生成**

#### 📌 起始信号（Start Condition）
```c
void IIC_Start(void) {
    SDA_OUT();          // SDA设置为输出模式
    IIC_SDA_H; IIC_SCL_H;
    delay_us(10);       // 保持高电平≥4us
    IIC_SDA_L;          // SDA在SCL高时变低
    delay_us(10);
    IIC_SCL_L;          // 钳住总线准备传输
}
```

#### 📌 停止信号（Stop Condition）
```c
void IIC_Stop(void) {
    SDA_OUT();
    IIC_SCL_L; IIC_SDA_L;
    delay_us(10);
    IIC_SCL_H;          // SCL先变高
    delay_us(10);
    IIC_SDA_H;          // SDA在SCL高时变高
    delay_us(10);       // 保持高电平≥4us
}
```

---

### **3️⃣ 数据传输函数**

#### 📌 发送单字节
```c
void IIC_Send_Byte(uint8_t txd) {
    SDA_OUT();
    IIC_SCL_L;  // 时钟线拉低允许数据变化
    for(uint8_t t=0; t<8; t++) {
        (txd & 0x80) ? IIC_SDA_H : IIC_SDA_L; // 发送最高位
        txd <<= 1;
        delay_us(8);
        IIC_SCL_H;      // 上升沿锁存数据
        delay_us(8);
        IIC_SCL_L;
        delay_us(8);
    }
}
```

#### 📌 接收单字节（带ACK控制）
```c
uint8_t IIC_Read_Byte(uint8_t ack) {
    uint8_t receive = 0;
    SDA_IN();           // 设置为输入模式
    for(uint8_t i=0; i<8; i++) {
        receive <<= 1;
        IIC_SCL_L; delay_us(8);
        IIC_SCL_H;      // 时钟上升沿采样数据
        if(READ_SDA) receive |= 0x01;
        delay_us(8);
    }
    ack ? IIC_Ack() : IIC_NAck(); // 发送应答信号
    return receive;
}
```

---

### **4️⃣ 应答信号处理**

#### 📌 产生ACK信号
```c
void IIC_Ack(void) {
    SDA_OUT();
    IIC_SCL_L;
    IIC_SDA_L;          // SDA拉低表示ACK
    delay_us(8);
    IIC_SCL_H;          // 保持时钟高电平≥4us
    delay_us(8);
    IIC_SCL_L;
}
```

#### 📌 产生NACK信号
```c
void IIC_NAck(void) {
    SDA_OUT();
    IIC_SCL_L;
    IIC_SDA_H;          // SDA保持高表示NACK
    delay_us(8);
    IIC_SCL_H; 
    delay_us(8);
    IIC_SCL_L;
}
```

---

### **5️⃣ 完整数据帧操作**

#### 📌 写入多字节数据
```c
uint8_t IIC_WriteBytes(uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *data) {
    IIC_Start();
    IIC_Send_Byte(dev_addr << 1); // 发送设备地址（写模式）
    if(IIC_Wait_Ack()) return 0;  // 检测从机应答
    
    IIC_Send_Byte(reg);           // 发送寄存器地址
    if(IIC_Wait_Ack()) return 0;
    
    for(uint8_t i=0; i<len; i++) {
        IIC_Send_Byte(data[i]);   // 逐字节发送数据
        if(IIC_Wait_Ack()) return 0;
    }
    IIC_Stop();
    return 1; // 成功返回1
}
```

#### 📌 读取多字节数据
```c
uint8_t IIC_ReadBytes(uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *data) {
    IIC_Start();
    IIC_Send_Byte(dev_addr << 1); // 发送设备地址（写模式）
    if(IIC_Wait_Ack()) return 0;
    
    IIC_Send_Byte(reg);           // 发送寄存器地址
    if(IIC_Wait_Ack()) return 0;
    
    IIC_Start();                  // 重复起始条件
    IIC_Send_Byte((dev_addr << 1) | 0x01); // 切换为读模式
    if(IIC_Wait_Ack()) return 0;
    
    for(uint8_t i=0; i<len; i++) {
        data[i] = IIC_Read_Byte(i != (len-1)); // 最后字节发NACK
    }
    IIC_Stop();
    return len; // 返回读取的字节数
}
```

---

### **6️⃣ 关键时序参数**

| 时序参数         | 典型值 | 说明                         |
| ---------------- | ------ | ---------------------------- |
| **起始条件保持** | ≥4.7μs | SCL高时SDA从高到低的变化时间 |
| **停止条件保持** | ≥4.0μs | SCL高时SDA从低到高的变化时间 |
| **时钟低电平**   | ≥4.7μs | SCL低电平持续时间            |
| **数据建立时间** | ≥250ns | 数据在SCL上升沿前的稳定时间  |
| **数据保持时间** | ≥0μs   | 数据在SCL下降沿后的保持时间  |

---

### **7️⃣ 使用示例**

#### 读取MPU6050器件ID
```c
uint8_t id = IIC_Read_One_Byte(0x68, 0x75); // 0x68为设备地址，0x75为WHO_AM_I寄存器
if(id == 0x68) {
    printf("MPU6050检测成功!\n");
}
```

#### 写入配置寄存器
```c
uint8_t config[2] = {0x6B, 0x00}; // PWR_MGMT_1寄存器地址和值
IIC_WriteBytes(0x68, config[0], 1, &config[1]);
```

---

### **8️⃣ 注意事项**

1. **上拉电阻**：确保SCL/SDA线上有4.7kΩ上拉电阻
2. **延时精度**：`delay_us`需根据主频精确实现，误差需<10%
3. **错误重试**：关键操作添加重试机制：
   ```c
   for(uint8_t retry=0; retry<3; retry++){
       if(IIC_WriteBytes(...)) break;
   }
   ```
4. **从机地址**：7位地址需左移1位，末位表示读/写方向

---

通过模块化代码结构与详细时序注释，显著提升I2C驱动代码的可维护性！ 🔌
