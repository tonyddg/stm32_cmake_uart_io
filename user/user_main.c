#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "main.h"

#include "user_main.h"

// UART IO 示例
#ifdef PROJECT_UART_IO

#include "user_uart.h"
void MainLoopTask(void *argument)
{   
    ConstBuf* resBuf = NULL;
    ByteBuf* printBuf = ByteBuf_Create(128);

    while(1)
    {
        // 读取数据
        resBuf = UART1ReceiveData(osWaitForever);
        if(resBuf == NULL)
        {
            Error_Handler();
        }

        // 使用 ByteBuf_Printf 函数创建格式化字符
        ByteBuf_Printf(printBuf, 0, "[REC]%s[REC]\r\n", resBuf->_buf);
        // 使用 ConstBuf_CreateByBuf 包裹缓冲区, 进行转换并发送
        UART1SendData(ConstBuf_CreateByBuf(printBuf, 0), 100);

        // 删除接收到的数据
        ConstBuf_Delete(resBuf);
        resBuf = NULL;
    }
    return;
}

#endif

// USB VPC IO 示例
#ifdef PROJECT_USB_VPC_IO

#include "usbd_cdc_if.h"
#include "user_usb_vpc.h"

// 主任务
void MainLoopTask(void *argument)
{
    ConstBuf* resBuf = NULL;
    ByteBuf* printBuf = ByteBuf_Create(128);

    while(1)
    {
        resBuf = USB_VPC_ReceiveData(osWaitForever);
        if(resBuf == NULL)
        {
            Error_Handler();
        }

        ByteBuf_Printf(printBuf, 0, "[REC]%s[REC]\r\n", resBuf->_buf);
        USB_VPC_SendData(ConstBuf_CreateByBuf(printBuf, 0), 100);

        ConstBuf_Delete(resBuf);
        resBuf = NULL;
    }
    return;
}

#endif

//////////////////////////

#ifdef PROJECT_I2C_CMD_USB_VPC

#include "user_usb_vpc.h"
#define SendData USB_VPC_SendData
#define ReceiveData USB_VPC_ReceiveData
#define PROJECT_I2C_CMD

#endif 

#ifdef PROJECT_I2C_CMD_UART

#include "user_uart.h"
#define SendData UART1SendData
#define ReceiveData UART1ReceiveData
#define PROJECT_I2C_CMD

#endif 

#ifdef PROJECT_I2C_CMD

// I2C IO 控制台项目 (基于 USB_VPC)
// 该项目为一个通过 USB_VPC 传输指令控制 I2C 的程序
// 提供以下通过 USB_VPC 传输的指令 (每个 [...] 代表一个字节), 其中设备地址需要手动左对齐
// 从设备获取信息 REC [设备地址][寄存器地址][接收长度] (不超过 25)
// 像设备发送信息 SEND  [设备地址][寄存器地址][发送数据]...
// 检查设备是否在 I2C 总线上 TOUCH [设备地址][尝试次数]
// 可通过以下命令测试
// SEND 78008D14AFA5 点亮 SSD1306 LED 屏的屏幕
// TOUCH 7801 测试 SSD1306 是否在 I2C 总线上, TOUCH D001 测试 MPU6050 是否在 I2C 总线上
// REC D07501 获取 MPU6050 的 I2C 地址 (应当返回 68)
// SEND D06B00 启用 MPU6050, REC D03B06 获取三轴加速度 (Z 轴, 即末尾四位约为 0X4000u)
// SEND D06B80 复位 MPU6050, REC D03B06 得到 0 结果

#include "user_i2c.h"
#include "string.h"

void NormalCallBack(uint8_t is_success)
{
    if(is_success)
    {
        SendData(ConstBuf_CreateByStr("Success!\r\n"), 100);
    }
    else
    {
        SendData(ConstBuf_CreateByStr("Fail!\r\n"), 100);
    }
}

void RecCallBack(uint8_t is_success, ConstBuf* data)
{
    static ByteBuf* printBuf = NULL;

    if(printBuf == NULL)
    {
        printBuf = ByteBuf_Create(64);
    }

    if(is_success)
    {
        ConstBuf* dataHex = ConstBuf_BufToHex(data->_buf, data->_len);
        ByteBuf_Printf(printBuf, 0, "Rec: %s\r\n", dataHex->_buf);

        SendData(ConstBuf_CreateByBuf(printBuf, 0), 100);
        ConstBuf_Delete(data);
        ConstBuf_Delete(dataHex);
    }
    else
    {
        SendData(ConstBuf_CreateByStr("Fail!\r\n"), 100);
    }
}

void MainLoopTask(void *argument)
{
    ConstBuf* cmdBuf = NULL;
    ByteBuf* printBuf = ByteBuf_Create(128);

    ConstBuf* cmdBody = NULL;
    ConstBuf* cmdArgs = NULL;

    while(1)
    {
        cmdBuf = ReceiveData(osWaitForever);
        if(cmdBuf == NULL)
        {
            Error_Handler();
        }

        ByteBuf_Printf(printBuf, 1, "[REC]%s[REC]\r\n", cmdBuf->_buf);
        if(CommandResolveText(cmdBuf, &cmdBody, &cmdArgs))
        {
            if(strcmp((const char *)cmdBody->_buf, "SEND") == 0)
            {
                if(cmdArgs->_len < 3)
                {
                    ByteBuf_Printf(printBuf, 0, "Incorrect Args: %u\r\n", cmdArgs->_len);
                }
                else
                {
                    I2CSendData(
                        cmdArgs->_buf[0],
                        cmdArgs->_buf[1],
                        ConstBuf_CreateExtBuf(cmdArgs->_buf, cmdArgs->_len, 2, 0, 0),
                        NormalCallBack,
                        osWaitForever
                    );
                    ByteBuf_Printf(printBuf, 0, "%sSend Done\r\n", printBuf->_buf);                    
                }
            }
            else if(strcmp((const char *)cmdBody->_buf, "REC") == 0)
            {
                if(cmdArgs->_len != 3)
                {
                    ByteBuf_Printf(printBuf, 0, "Incorrect Args: %u\r\n", cmdArgs->_len);
                }
                else
                {
                    I2CRecData(
                        cmdArgs->_buf[0],
                        cmdArgs->_buf[1],
                        cmdArgs->_buf[2],
                        RecCallBack,
                        osWaitForever
                    );
                    ByteBuf_Printf(printBuf, 0, "%sRec Done\r\n", printBuf->_buf);
                }
            }
            else if(strcmp((const char *)cmdBody->_buf, "TOUCH") == 0)
            {
                if(cmdArgs->_len != 2)
                {
                    ByteBuf_Printf(printBuf, 0, "Incorrect Args: %u\r\n", cmdArgs->_len);
                }
                else
                {
                    I2CTouch(
                        cmdArgs->_buf[0],
                        cmdArgs->_buf[1],
                        NormalCallBack,
                        osWaitForever
                    );
                    ByteBuf_Printf(printBuf, 0, "%sTouch Done\r\n", printBuf->_buf);                    
                }
            }
            else
            {
                ByteBuf_Printf(printBuf, 0, "%sUnknown Body\r\n", printBuf->_buf);
            }

            ConstBuf_Delete(cmdBody);
            ConstBuf_Delete(cmdArgs);
        }
        else
        {
            ByteBuf_Printf(printBuf, 0, "%sUnknown CMD\r\n", printBuf->_buf);
        }

        SendData(ConstBuf_CreateByBuf(printBuf, 0), 100);

        ConstBuf_Delete(cmdBuf);
        cmdBuf = NULL;
    }
    return;
}

#endif

// LED 闪烁任务
void LedBlinkTask(void *argument)
{
    while(1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);   
    }
    return;
}