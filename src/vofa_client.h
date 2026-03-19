// 文件编码: GB18030
/*
 * vofa_client.h
 *
 *  Created on: 2025年4月4日
 *      Author: LHYe200
 */

#ifndef CODE_VOFA_CLIENT_H_
#define CODE_VOFA_CLIENT_H_

#define VOFA_RECV_CH_NUM                  (64)                 // 接收通道数 最高255
#define VOFA_SEND_CH_NUM                  (32)                 // 发送通道数
#define USE_ZF_LIBRARY                    // 是否使用逐飞科技开源库

#ifdef USE_ZF_LIBRARY
#include "zf_common_headfile.h"
#define VOFA_CLIENT_COM_INTERFACE         (0)                  // 0:串口 1:WIFI无线SPI  2:自定义

#if VOFA_CLIENT_COM_INTERFACE == 0                    // 普通串口  如果使用的串口和Debug串口一致，请注释掉逐飞的Debug串口中断处理函数调用，再使用该库的Callback函数
#define VOFA_CLIENT_UART_PORT            UART_0               // 串口号
#define VOFA_CLIENT_UART_BAUDRATE        115200               // 波特率
#define VOFA_CLIENT_UART_RX              UART0_RX_P14_1       // 串口接收引脚
#define VOFA_CLIENT_UART_TX              UART0_TX_P14_0       // 串口发送引脚

#elif VOFA_CLIENT_COM_INTERFACE == 1                  // WiFi SPI  以TCP Client模式，如需使用TCP Server或UDP请使用INTERFACE 4进行自定义
#define VOFA_CLIENT_WIFI_SPI_SSID            "YOUR_SSID"          // 填写你的WiFi SSID
#define VOFA_CLIENT_WIFI_SPI_PASSWORD        "YOUR_PASSWORD"      // 填写你的WiFi密码 如果没有密码则替换为NULL
#define VOFA_CLIENT_WIFI_SPI_TARGET_IP       "YOUR_COMPUTER_IP"   // 填写上位机的IP地址 即VOFA+PC所在电脑的IP地址
#define VOFA_CLIENT_WIFI_SPI_TARGET_PORT     "1347"               // 填写上位机的端口号 VOFA+PC默认端口号为1347
#define VOFA_CLIENT_WIFI_SPI_LOCAL_PORT      "6666"               // 填写本机的端口号 可以随意填写 但是不要超过端口允许范围0~65535
#define VOFA_CLIENT_WIFI_SPI_FAILURE_RETRY   (3)                  // 连接失败重试次数
#define VOFA_CLIENT_WIFI_SPI_ENABLE_PRINTF   (0)                  // 是否使能打印调试信息

#elif VOFA_CLIENT_COM_INTERFACE == 2                  // 无线串口 (Unverified)
// 相关串口配置定义请在逐飞库中自行按需更改

#elif VOFA_CLIENT_COM_INTERFACE == 3                  // WiFi串口 (Unverified)  以TCP Client模式，如需使用TCP Server或UDP请使用INTERFACE 4进行自定义
#define VOFA_CLIENT_WIFI_UART_SSID            "YOUR_SSID"          // 填写你的WiFi SSID
#define VOFA_CLIENT_WIFI_UART_PASSWORD        "YOUR_PASSWORD"      // 填写你的WiFi密码 如果没有密码则替换为NULL
#define VOFA_CLIENT_WIFI_UART_TARGET_IP       "YOUR_COMPUTER_IP"   // 填写上位机的IP地址 即VOFA+PC所在电脑的IP地址
#define VOFA_CLIENT_WIFI_UART_TARGET_PORT     "1347"               // 填写上位机的端口号 VOFA+PC默认端口号为1347
#define VOFA_CLIENT_WIFI_UART_FAILURE_RETRY   (3)                  // 连接失败重试次数
#define VOFA_CLIENT_WIFI_UART_ENABLE_PRINTF   (0)                  // 是否使能打印调试信息

#elif VOFA_CLIENT_COM_INTERFACE == 4
// 填写使用逐飞库的自定义接口的相关信息

#endif

#else
// 填写不使用逐飞库的自定义接口的相关信息

#endif

typedef enum{
    Format_Invalid,
    Format_Mono,
    Format_MonoLSB,
    Format_Indexed8,
    Format_RGB32,
    Format_ARGB32,
    Format_ARGB32_Premultiplied,
    Format_RGB16,
    Format_ARGB8565_Premultiplied,
    Format_RGB666,
    Format_ARGB6666_Premultiplied,
    Format_RGB555,
    Format_ARGB8555_Premultiplied,
    Format_RGB888,
    Format_RGB444,
    Format_ARGB4444_Premultiplied,
    Format_RGBX8888,
    Format_RGBA8888,
    Format_RGBA8888_Premultiplied,
    Format_BGR30,
    Format_A2BGR30_Premultiplied,
    Format_RGB30,
    Format_A2RGB30_Premultiplied,
    Format_Alpha8,
    Format_Grayscale8,
    
    // 以下格式发送时，IMG_WIDTH和IMG_HEIGHT不需要强制指定，设置为-1即可
    Format_BMP,
    Format_GIF,
    Format_JPG,
    Format_PNG,
    Format_PBM,
    Format_PGM,
    Format_PPM,
    Format_XBM,
    Format_XPM,
    Format_SVG,
} ImgFormat_t ;

extern unsigned char init_failed_flag;                             // 初始化失败标志位
extern float vofa_rev_data[VOFA_RECV_CH_NUM];                      // 记录接收数据
extern unsigned char vofa_rev_new_data_flag[VOFA_RECV_CH_NUM];     // 接收新数据标志位,需手动置零
extern float vofa_last_rev_data;                                   // 最后一次接收数据
extern unsigned int vofa_last_rev_ch;                              // 最后一次接收通道

void VOFA_Data_Init(void);                                          // VOFA数据初始化函数
unsigned char VOFA_Connection_Init(void);                           // VOFA连接初始化函数
unsigned char VOFA_Client_Init(void);                               // VOFA客户端初始化函数 等同于调用了 VOFA_Data_Init 和 VOFA_Connection_Init
void VOFA_Set_Float_Data(unsigned int index, float data);           // 设置某通道数据
void VOFA_Set_Float_Datas_From_Start(unsigned int set_nums,...);    // 设置从第一个通道开始的多个通道数据
void VOFA_Send_Datas(unsigned int send_nums);                       // 从第一个通道开始发送数据指定数量的数据
void VOFA_Send_JustFloat_Image(unsigned int IMG_ID, unsigned int IMG_WIDTH, unsigned int IMG_HEIGHT, unsigned int IMG_DATA_SIZE, ImgFormat_t IMG_FORMAT,unsigned char *IMG_DATA); // 发送图像数据
void VOFA_Receiver_Callback(void);                                  // 接收回调函数 需要在主循环或者接收中断中调用


#endif /* CODE_VOFA_CLIENT_H_ */
 
