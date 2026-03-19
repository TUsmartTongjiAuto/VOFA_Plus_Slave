// 文件编码: GB18030
/*
 * vofa_client.c
 *
 *  Created on: 2025年4月4日
 *      Author: LHYe200
 */

#include "vofa_client.h"
#include <stdarg.h>
#include <string.h>


typedef union
{
    unsigned char bdata[4];
    float fdata;
    unsigned int udata;
}VOFA_num_data_t;

float vofa_rev_data[VOFA_RECV_CH_NUM] = {0};                      // 记录接收数据
float vofa_last_rev_data = 0;                                     // 最后一次接收数据
unsigned int vofa_last_rev_ch = VOFA_RECV_CH_NUM;                 // 最后一次接收通道
unsigned char vofa_rev_new_data_flag[VOFA_RECV_CH_NUM] = {0};     // 接收新数据标志位

unsigned char rev_count = 0;                                      // 接收计数
unsigned char rev_buffer[8] = {0};                                // 接收缓冲区

unsigned char init_failed_flag = 1;                               // 初始化失败标志位

VOFA_num_data_t send_data[VOFA_SEND_CH_NUM + 1];                  // 发送数据
unsigned char little_endian_flag = 0;                             // 大小端标志位
unsigned char vofa_just_float_tail[4] = {0x00, 0x00, 0x80, 0x7f}; // 尾帧数据

static inline void Change_Endian(unsigned char *data)
{
    unsigned char i;
    if(little_endian_flag)
    {
        return;
    }

    for(i = 0; i < 2; i++)
    {
        unsigned char temp = data[i];
        data[i] = data[3 - i];
        data[3 - i] = temp;
    }
}

void VOFA_Data_Init()
{
    VOFA_num_data_t x;
    x.udata = 1;
    if(x.bdata[0] == 1)
    {
        little_endian_flag = 1;
    }
    else
    {
        little_endian_flag = 0;
    }
    memcpy(send_data[VOFA_SEND_CH_NUM].bdata, vofa_just_float_tail, sizeof(vofa_just_float_tail));  // 初始化尾帧数据
    memset(vofa_rev_data, 0, sizeof(vofa_rev_data));                          // 清除接收数据
    memset(vofa_rev_new_data_flag, 0, sizeof(vofa_rev_new_data_flag));        // 清除接收新数据标志位
}


unsigned char VOFA_Connection_Init()
{
    #ifdef USE_ZF_LIBRARY
    #if VOFA_CLIENT_COM_INTERFACE == 0
    // 使用普通串口通信
    uart_init(VOFA_CLIENT_UART_PORT, VOFA_CLIENT_UART_BAUDRATE, VOFA_CLIENT_UART_RX, VOFA_CLIENT_UART_TX);      // 串口初始化
    uart_rx_interrupt(VOFA_CLIENT_UART_PORT, 1);

    #elif VOFA_CLIENT_COM_INTERFACE == 1
    // 使用WIFI SPI通信
    uint32 failure_count = 0;
    while(wifi_spi_init(VOFA_CLIENT_WIFI_SPI_SSID, VOFA_CLIENT_WIFI_SPI_PASSWORD))
    {
        #if VOFA_CLIENT_WIFI_SPI_ENABLE_PRINTF
        printf("\r\n connect wifi failed. \r\n");
        #endif
        failure_count ++;
        if(failure_count > VOFA_CLIENT_WIFI_SPI_FAILURE_RETRY)                  // 如果连接失败超过指定次数则退出
        {
            return 0;
        }
        system_delay_ms(100);                                                   // 初始化失败 等待 100ms
    }

    #if VOFA_CLIENT_WIFI_SPI_ENABLE_PRINTF
    printf("\r\n module version:%s",wifi_spi_version);                          // 模块固件版本
    printf("\r\n module mac    :%s",wifi_spi_mac_addr);                         // 模块 MAC 信息
    printf("\r\n module ip     :%s",wifi_spi_ip_addr_port);                     // 模块 IP 地址
    #endif
    failure_count = 0;
    if(0 == WIFI_SPI_AUTO_CONNECT)                                              // 如果没有开启自动连接 就需要手动连接目标 IP
    {
        while(wifi_spi_socket_connect(                                          // 向指定目标 IP 的端口建立 TCP 连接
            "TCP",                                                              // 指定使用TCP方式通讯
            VOFA_CLIENT_WIFI_SPI_TARGET_IP,                                     // 指定远端的IP地址，填写上位机的IP地址
            VOFA_CLIENT_WIFI_SPI_TARGET_PORT,                                   // 指定远端的端口号，填写上位机的端口号，通常上位机默认是8080
            VOFA_CLIENT_WIFI_SPI_LOCAL_PORT))                                   // 指定本机的端口号
        {
            #if VOFA_CLIENT_WIFI_SPI_ENABLE_PRINTF
            printf("\r\n socket connect failed. \r\n");
            #endif
            failure_count ++;
            if(failure_count > VOFA_CLIENT_WIFI_SPI_FAILURE_RETRY)              // 如果连接失败超过指定次数则退出
            {
                return 0;
            }
            system_delay_ms(100);                                               // 建立连接失败 等待 100ms
        }
    }

    #elif VOFA_CLIENT_COM_INTERFACE == 2
    // 使用无线串口通信 (Unverified)
    uint8 init_good = 1;
    init_good = wireless_uart_init();
    if(!init_good)
    {
        return 0;
    }
    #elif VOFA_CLIENT_COM_INTERFACE == 3
    // 使用WiFi串口通信 (Unverified)
    uint32 failure_count = 0;
    while(wifi_uart_init(VOFA_CLIENT_WIFI_UART_SSID, VOFA_CLIENT_WIFI_UART_PASSWORD, WIFI_UART_STATION))
    {
        #if VOFA_CLIENT_WIFI_UART_ENABLE_PRINTF
        printf("\r\n connect wifi failed. \r\n");
        #endif
        failure_count ++;
        if(failure_count > VOFA_CLIENT_WIFI_UART_FAILURE_RETRY)                 // 如果连接失败超过指定次数则退出
        {
            return 0;
        }
        system_delay_ms(100);                                                   // 初始化失败 等待 100ms
    }
    if(0 == WIFI_UART_AUTO_CONNECT)                                             // 如果没有开启自动连接 就需要手动连接目标 IP
    {
        while(wifi_uart_connect_tcp_servers(                                    // 向指定目标 IP 的 TCP Server 端口建立 TCP 连接
            VOFA_CLIENT_WIFI_UART_TARGET_IP,                                    // 这里使用与自动连接时一样的目标 IP 实际使用时也可以直接填写目标 IP 字符串
            VOFA_CLIENT_WIFI_UART_TARGET_PORT,                                  // 这里使用与自动连接时一样的目标端口 实际使用时也可以直接填写目标端口字符串
            WIFI_UART_COMMAND))                                                 // 采用命令传输模式 当然你可以改成透传模式 实际上差别并不是很大
        {
            #if VOFA_CLIENT_WIFI_UART_ENABLE_PRINTF
            // 如果一直建立失败 考虑一下是不是没有接硬件复位
            printf("\r\n Connect TCP Servers error, try again.");
            #endif
            failure_count ++;
            if(failure_count > VOFA_CLIENT_WIFI_UART_FAILURE_RETRY)             // 如果连接失败超过指定次数则退出
            {
                return 0;
            }
            system_delay_ms(100);                                               // 建立连接失败 等待 100ms
        }
    }

    #elif VOFA_CLIENT_COM_INTERFACE == 4
    // 使用逐飞科技开源库时的发送代码 填写自定义接口的初始化代码

    #endif

    #else
    // 不使用逐飞科技开源库时的初始化代码 这里用于自定义接口的初始化 初始化失败要返回 0

    #endif

    init_failed_flag = 0;
    return 1;
}


unsigned char VOFA_Client_Init()
{
    VOFA_Data_Init();
    return VOFA_Connection_Init();
}

void VOFA_Sender(unsigned char *p, unsigned int len)
{
    if(init_failed_flag) // 如果初始化失败则不发送数据
    {
        return;
    }
    #ifdef USE_ZF_LIBRARY
    #if VOFA_CLIENT_COM_INTERFACE == 0
    uart_write_buffer(VOFA_CLIENT_UART_PORT,p,len);
    #elif VOFA_CLIENT_COM_INTERFACE == 1
    wifi_spi_send_buffer(p,len);
    #elif VOFA_CLIENT_COM_INTERFACE == 2
    wireless_uart_send_buffer(p,len);
    #elif VOFA_CLIENT_COM_INTERFACE == 3
    wifi_uart_send_buffer(p,len);
    #elif VOFA_CLIENT_COM_INTERFACE == 4
    // 使用逐飞科技开源库时的发送代码 填写自定义接口的发送代码

    #endif

    #else
    // 不使用逐飞科技开源库时的发送代码 这里用于自定义接口的发送

    #endif
}

void VOFA_Receiver_Callback()
{
    unsigned char rev_tmp = 0;
    VOFA_num_data_t tmp_data;
    if(init_failed_flag) // 如果初始化失败则不发送数据
    {
        return;
    }
    #ifdef USE_ZF_LIBRARY
    #if VOFA_CLIENT_COM_INTERFACE == 0
    while(uart_query_byte(VOFA_CLIENT_UART_PORT, &rev_tmp))  // UART模式 查询是否有数据接收 有数据则读取一个字节
    #elif VOFA_CLIENT_COM_INTERFACE == 1
    while(wifi_spi_read_buffer(&rev_tmp,1))                  // WIFI SPI模式 查询是否有数据接收 有数据则读取一个字节
    #elif VOFA_CLIENT_COM_INTERFACE == 2
    while(wireless_uart_read_buffer(&rev_tmp,1))             // 无线串口模式 查询是否有数据接收 有数据则读取一个字节
    #elif VOFA_CLIENT_COM_INTERFACE == 3
    while(wifi_uart_read_buffer(&rev_tmp,1))                  // WiFi串口模式 查询是否有数据接收 有数据则读取一个字节
    #elif VOFA_CLIENT_COM_INTERFACE == 4
    // 使用逐飞科技开源库时的接收代码 填写自定义接口的接收代码 每次获取一个字节循环进行处理

    #endif
    #else
    // 不使用逐飞科技开源库时的接收代码 这里用于自定义接口的接收 每次获取一个字节循环进行处理

    #endif
    {
        if(rev_tmp == 0xA6 && rev_buffer[0] != 0xA6) // 判断帧头
        {
            rev_count = 0; // 未收到帧头或者未正确包含帧头则重新接收
        }
        rev_buffer[rev_count++] = rev_tmp; // 保存数据
        if(rev_count >= 7) // 判断是否接收到指定数量的数据
        {
            if(rev_buffer[0] == 0xA6) // 判断帧头是否正确
            {
                if(rev_buffer[1] == rev_buffer[6] && rev_buffer[1] < VOFA_RECV_CH_NUM)
                {
                    tmp_data.bdata[0] = rev_buffer[2]; // 高字节
                    tmp_data.bdata[1] = rev_buffer[3]; // 高字节
                    tmp_data.bdata[2] = rev_buffer[4]; // 低字节
                    tmp_data.bdata[3] = rev_buffer[5]; // 低字节
                    Change_Endian(tmp_data.bdata); // 根据大小端标志位调整字节顺序
                    vofa_last_rev_ch = rev_buffer[1]; // 保存上次接收通道
                    vofa_rev_data[vofa_last_rev_ch] = tmp_data.fdata; // 保存接收数据
                    vofa_rev_new_data_flag[rev_buffer[1]] = 1; // 接收新数据标志位置位
                    vofa_last_rev_data = vofa_rev_data[vofa_last_rev_ch]; // 保存上次接收数据
                }
            }
            rev_count = 0; // 清除缓冲区计数值
            rev_buffer[0] = 0; // 清除缓冲区数据
        }
    }
}

void VOFA_Set_Float_Data(unsigned int index, float data)
{
    send_data[index].fdata = data;
    Change_Endian(send_data[index].bdata);
}

void VOFA_Set_Float_Datas_From_Start(unsigned int set_nums,...)
{
    unsigned int i = 0;
    va_list Valist;
    va_start(Valist, set_nums);

    for(i = 0; i < set_nums; ++i)
    {
        send_data[i].fdata = va_arg(Valist, float);
        Change_Endian(send_data[i].bdata);
    }
    va_end(Valist);
}

void VOFA_Send_Datas(unsigned int send_nums)
{
    unsigned char *p = send_data[0].bdata;
    memcpy(send_data[send_nums].bdata, vofa_just_float_tail, sizeof(vofa_just_float_tail));
    VOFA_Sender(p, 4 * (send_nums + 1));
}


void VOFA_Send_JustFloat_Image(unsigned int IMG_ID, int IMG_WIDTH, int IMG_HEIGHT,unsigned int IMG_DATA_SIZE, ImgFormat_t IMG_FORMAT,unsigned char* IMG_DATA)
{
    int preFrame[7] = {
        IMG_ID, 
        IMG_DATA_SIZE,
        IMG_WIDTH, 
        IMG_HEIGHT, 
        (int)IMG_FORMAT, 
        0x7F800000,
        0x7F800000
    };
    unsigned char preFrameData[28] = {0};
    memcpy(preFrameData, preFrame, sizeof(unsigned int)*7);  // 拷贝预处理数据
    VOFA_Sender(preFrameData, sizeof(unsigned int)*7);       // 发送预处理数据
    VOFA_Sender(IMG_DATA, IMG_DATA_SIZE);           // 发送图像数据
}

