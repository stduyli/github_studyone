## 📌 WiFi串口驱动代码解析（模块化注释版）

------

[TOC]



------



### **1️⃣ 头文件定义 (Drv_WIFI.h)**

清晰划分数据结构与接口

```c
#ifndef Drv_WIFI_H
#define Drv_WIFI_H

#include <stdint.h>

/*---------------------------------------
  WiFi状态结构体定义
---------------------------------------*/
typedef struct {
    uint8_t state;       // WiFi连接状态（0:断开 1:连接）
    uint8_t level;       // WiFi信号强度（0-4级）
    uint8_t blueState;   // 蓝牙状态（0:关闭 1:开启）
    uint8_t sendSwitch;  // 实时数据发送开关（0:关闭 1:开启）
} WifiState;

/*---------------------------------------
  函数声明
---------------------------------------*/
void wifi_init(void);                   // WiFi模块初始化
void wifi_enable(uint8_t sw);           // WiFi使能控制
void wifi_send_to_wifi(uint8_t *buf, uint32_t len); // 数据发送至WiFi模块
void wifi_rx_process(void);             // WiFi数据接收处理
void wifi_query_latest_app_version(void); // 查询最新固件版本

#endif // Drv_WIFI_H
```

---

### **2️⃣ USB-CDC核心驱动 (WIFI.c)**

模块化代码段 + 协议处理

```c
#include "drv_usb_hw.h"
#include "string.h"
#include "device.h"
#include "utility.h"
#include "Drv_wifi.h"

#define USB_CDC_TX_BUF_SIZE  1024  // USB发送缓冲区大小

/*---------------------------------------
  全局变量定义
---------------------------------------*/
usb_core_driver cdc_acm;          // USB核心驱动实例
static uint8_t usbConnect = 0;    // USB连接状态标志
static uint8_t usbTx[USB_CDC_TX_BUF_SIZE]; // 发送缓冲区

/*=======================================
  USB硬件配置层
=======================================*/

/**
  * @brief  USB GPIO配置
  * @note   配置PA11(DP)、PA12(DM)为USB功能
  */
static void usb_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_SYSCFG);
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_11 | GPIO_PIN_12);
}

/**
  * @brief  USB时钟配置
  * @note   配置PLL为48MHz USB时钟
  */
static void usb_rcu_config(void)
{
    rcu_pll_config(RCU_PLLSRC_HXTAL, 24, 288, 6, 2);
    RCU_CTL |= RCU_CTL_PLLEN;
    while(0U == (RCU_CTL & RCU_CTL_PLLSTB));
    rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
    rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);
    rcu_periph_clock_enable(RCU_USBFS);
}

/**
  * @brief  USB中断配置
  * @note   使能USB FS全局中断
  */
static void usb_intr_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable((uint8_t)USBFS_IRQn, 2U, 0U);
}

/*=======================================
  USB协议处理层
=======================================*/

/**
  * @brief  USB数据接收处理
  * @param  rx   接收数据指针
  * @param  len  数据长度
  * @note   实现功能：
  *         1. 固件升级协议处理
  *         2. 设备信息查询
  *         3. WiFi数据转发
  */
static void rx_data_process(uint8_t *rx, uint16_t len)
{
#if APP == 0  // Bootloader模式
    /* IAP固件升级处理 */
    if (1 == iap_receive_available(IAP_MODE_USB)) {
        IapError err = iap_write_handle(rx, len);
        if (err == IAP_NO_ERR) {
            usb_cdc_write_string(TRANS_ACK);
        } else if (err > IAP_WARN_BUF_NO_FULL) {
            usb_cdc_write_string(TRANS_FAIL);
            memcpy((uint8_t*)APP_UPDATE_FLAG, STR_UPDATE_FLAG, strlen(STR_UPDATE_FLAG));
            NVIC_SystemReset();
        }
        return;
    }
#endif

    /* 设备信息查询协议 */
    if (0 == memcmp(rx, JCXX_IDENTITY, strlen(JCXX_IDENTITY))) {
        usbConnect = 1;
        usb_cdc_write_string(APP ? JCXX_USER : JCXX_BOOT);
    } 
    else if (0 == memcmp(rx, JCXX_VERSION, strlen(JCXX_VERSION))) {
        usb_cdc_write_string(APP ? JCID_DEVICE_VERS : JCID_DEVICE_MID JCID_DEVICE_BOOT_VERS);
    }
    else if (0 == memcmp(rx, JCXX_CPU_ID, strlen(JCXX_CPU_ID))) {
        usb_cdc_write_string(mcu_unique_id());
    }
    
    /* WiFi数据透传 */
    else if (0 == memcmp(rx, JC_TO_ESP32, strlen(JC_TO_ESP32))) {
        uint8_t cmdLen = strlen(JC_TO_ESP32);
        if (len > cmdLen + 2) {
            wifi_send_to_wifi(rx + cmdLen, len - cmdLen - 2);
        }
    }
}

/*=======================================
  公共接口函数
=======================================*/

/**
  * @brief  USB CDC初始化
  * @note   初始化流程：
  *         1. GPIO配置 → 2. 时钟配置 → 3. 核心驱动初始化 → 4. 中断配置
  */
void usb_init(void)
{
    usb_gpio_config();
    usb_rcu_config();
    usbd_init(&cdc_acm, USB_CORE_ENUM_FS, &cdc_desc, &cdc_class);
    usb_intr_config();
}

/**
  * @brief  USB数据发送
  * @param  buf 发送数据指针
  * @param  len 数据长度
  * @retval 0:成功 -1:超时
  */
int usb_cdc_write(uint8_t *buf, uint16_t len)
{
    usb_cdc_handler *cdc = (usb_cdc_handler*)cdc_acm.dev.class_data[CDC_COM_INTERFACE];
    uint32_t cnt = 0;
    
    // 等待上次发送完成
    while (cdc->packet_sent == 0) {
        if (++cnt > UINT16_MAX) return -1;
    }
    
    cdc->packet_sent = 0;
    usbd_ep_send(&cdc_acm, CDC_DATA_IN_EP, buf, len);
    return 0;
}
```

---

### **3️⃣ 关键协议说明表**

| 协议命令        | 功能描述       | 响应示例                 |
| --------------- | -------------- | ------------------------ |
| `JCXX_IDENTITY` | 设备身份查询   | 返回固件类型（Boot/App） |
| `JCXX_VERSION`  | 固件版本查询   | 返回版本号字符串         |
| `JCXX_CPU_ID`   | 芯片唯一ID查询 | 返回128位唯一ID          |
| `JC_TO_ESP32`   | WiFi数据透传   | 转发至ESP32模块          |

---

### **4️⃣ 数据流示意图**

```text
[上位机] --USB CDC--> 
        |-- 固件升级数据 → IAP处理
        |-- 信息查询命令 → 返回设备信息
        |-- WiFi数据包 → 转发至ESP32
```

---

### **5️⃣ 典型使用流程**

```c
int main() {
    // 初始化硬件
    usb_init();
    wifi_init();

    while(1) {
        // USB数据处理
        usb_cdc_process();
        
        // WiFi数据处理
        wifi_rx_process();
        
        // 其他应用逻辑
        if(usb_is_connected()) {
            // 发送状态信息
            WifiState state = {1, 3, 0, 1};
            usb_cdc_write((uint8_t*)&state, sizeof(state));
        }
    }
}
```

---

### **6️⃣ 调试注意事项**

1. **USB连接检测**  
   通过`usb_is_connected()`函数获取连接状态，需等待枚举完成后再发送数据。

2. **协议格式验证**  
   使用串口调试工具发送协议命令时，注意包含完整协议头和终止符。

3. **数据边界处理**  
   WiFi透传时需注意数据包分片情况，建议增加长度前缀或帧尾标识。

4. **错误重传机制**  
   在`usb_cdc_write()`中添加重试逻辑，提高通信可靠性。

---

通过模块化注释与协议分层设计，显著提升嵌入式通信代码的可维护性！ 📶
