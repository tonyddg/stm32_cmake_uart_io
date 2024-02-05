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