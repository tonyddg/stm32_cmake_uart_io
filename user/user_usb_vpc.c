#ifdef USE_USB_VPC

#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

#include "byte_buf.h"
#include "user_usb_vpc.h"

#include "usbd_cdc_if.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

//********** USB VPC 接收管理 **********//

// 使用 ByteBuf 对象包裹系统的 USB 接收缓冲区
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
ByteBuf wrapRxBuf = {
    ._buf = UserRxBufferFS,
    ._len = 0,
    ._size = APP_RX_DATA_SIZE
};

// 结果数据块队列长度
const uint32_t USB_VPC_RECEIVE_QUEUE_SIZE = 8;
// 读取缓冲区长度
const uint32_t USB_VPC_RECEIVE_BUF_SIZE = APP_RX_DATA_SIZE;
// 是否将结果作为字符串处理
const uint8_t USB_VPC_RECEIVE_AS_STRING = 1;

// 接收数据暂存队列 (以暂存的常量数据块为元素)
osMessageQueueId_t uvRecQueue = NULL;
// 接收完成信号
osSemaphoreId_t uvRecDone = NULL;

// 接收直到空闲完成回调函数, 函数的第二个参数为接收到的数据量
void USB_VPC_ReceiveCmpltCallBack(uint32_t len)
{
    if(uvRecDone != NULL)
    {
        wrapRxBuf._len = len;
        osSemaphoreRelease(uvRecDone);
    }
}

void USB_VPC_ReceiveTask(void* args)
{
    // 初始化接收队列, 信号量与缓冲区
    uvRecQueue = osMessageQueueNew(USB_VPC_RECEIVE_QUEUE_SIZE, sizeof(ByteBuf*), NULL);
    uvRecDone = osSemaphoreNew(1, 0, NULL);

    while(1)
    {
        // 等待一次数据接收完成
        osSemaphoreAcquire(uvRecDone, osWaitForever);            

        ConstBuf* tmpResBuf = NULL;
        // 当队列满时, 删除最早插入的数据
        if(osMessageQueueGetCount(uvRecQueue) == USB_VPC_RECEIVE_QUEUE_SIZE)
        {
            tmpResBuf = USB_VPC_ReceiveData(osWaitForever);
            ConstBuf_Delete(tmpResBuf);
        }

        // 将缓冲区数据复制到一个常量缓冲区中, 并缓存到接收队列
        tmpResBuf = ConstBuf_CreateByBuf(&wrapRxBuf, USB_VPC_RECEIVE_AS_STRING);
        osMessageQueuePut(uvRecQueue, &tmpResBuf, 0, osWaitForever);
    }
}

ConstBuf* USB_VPC_ReceiveData(uint32_t timeout)
{
    ConstBuf* tmpResBuf = NULL;
    osMessageQueueGet(uvRecQueue, &tmpResBuf, NULL, timeout);
    return tmpResBuf;
}

USB_VPC_RecState USB_VPC_ReceiveGetState()
{
    if(uvRecQueue == NULL)
    {
        return USB_VPC_REC_UNINIT;
    }
    else if(HAL_PCD_GetState(&hpcd_USB_FS) == HAL_PCD_STATE_ERROR)
    {
        return USB_VPC_REC_ERROR;
    }
    else if(HAL_PCD_GetState(&hpcd_USB_FS) == HAL_PCD_STATE_RESET)
    {
        return USB_VPC_REC_RESET;
    }
    else if(osMessageQueueGetCount(uvRecQueue) == 0)
    {
        return USB_VPC_REC_EMPTY;
    }
    else
    {
        return USB_VPC_REC_READY;
    }
}

//********** USB VPC 发送管理 **********//

// USB VPC 发送队列长度
const uint32_t USB_VPC_SEND_QUEUE_SIZE = 8;

// 发送数据暂存队列 (以暂存的常量数据块为元素)
osMessageQueueId_t uvSendQueue = NULL;

// 数据发送管理任务
void USB_VPC_SendTask(void* args)
{
    // 在管理任务启动时, 初始化信号量与队列
    ConstBuf* sendData = NULL;
    uvSendQueue = osMessageQueueNew(USB_VPC_SEND_QUEUE_SIZE, sizeof(ConstBuf*), NULL);

    while(1)
    {
        // 等待发送队列中插入数据
        osMessageQueueGet(uvSendQueue, &sendData, NULL, osWaitForever);

        if(CDC_Transmit_FS(sendData->_buf, sendData->_len) != USBD_OK)
        {
            Error_Handler();
        }

        // 删除已发送数据块
        ConstBuf_Delete(sendData);
    }
}

osStatus_t USB_VPC_SendData(ConstBuf* data, uint32_t timeout)
{
    if(uvSendQueue == NULL)
    {
        return osError;
    }

    osStatus_t res = osMessageQueuePut(uvSendQueue, &data, 0, timeout);

    // 插入队列失败时, 自动删除数据块
    if(res != osOK)
    {
        ConstBuf_Delete(data);
    }
    return res;
}

USB_VPC_SendState USB_VPC_SendGetState()
{
    if(uvSendQueue == NULL)
    {
        return USB_VPC_SEND_UNINIT;
    }
    else if(HAL_PCD_GetState(&hpcd_USB_FS) == HAL_PCD_STATE_ERROR)
    {
        return USB_VPC_SEND_ERROR;
    }
    else if(HAL_PCD_GetState(&hpcd_USB_FS) == HAL_PCD_STATE_RESET)
    {
        return USB_VPC_SEND_RESET;
    }
    else if(osMessageQueueGetCount(uvSendQueue) == USB_VPC_SEND_QUEUE_SIZE)
    {
        return USB_VPC_SEND_QUEUEFULL;
    }
    else
    {
        return USB_VPC_SEND_READY;
    }
}

#endif
