#include "stm32f1xx_hal.h"
#include "usart.h"
#include "cmsis_os.h"

#include "string.h"

#include "user_uart.h"
#include "byte_buf.h"

//********** UART1 发送管理 **********//

// UART1 发送队列长度
const uint32_t UART1_SEND_QUEUE_SIZE = 8;
// 是否使用 DMA 进行发送
const uint8_t UART1_SEND_USE_DMA = 1;

// 发送数据暂存队列 (以暂存的常量数据块为元素)
osMessageQueueId_t uart1SendQueue = NULL;
// 发送完成信号
osSemaphoreId_t uart1SendDone = NULL;

// 数据发送完成回调
void UART1SendCmpltCallBack(UART_HandleTypeDef *huart)
{
    if(uart1SendDone != NULL)
    {
        osSemaphoreRelease(uart1SendDone);
    }
}

// 数据发送管理任务
void UART1SendTask(void* args)
{
    // 在管理任务启动时, 初始化信号量与队列
    ConstBuf* sendData = NULL;
    uart1SendQueue = osMessageQueueNew(UART1_SEND_QUEUE_SIZE, sizeof(ConstBuf*), NULL);
    

    // 注册发送完成回调函数
    if(UART1_SEND_USE_DMA)
    {
        uart1SendDone = osSemaphoreNew(1, 0, NULL);
        HAL_UART_RegisterCallback(&huart1, HAL_UART_TX_COMPLETE_CB_ID, &UART1SendCmpltCallBack);
    }

    while(1)
    {
        // 等待发送队列中插入数据
        osMessageQueueGet(uart1SendQueue, &sendData, NULL, osWaitForever);

        // 使用 HAL 提供的方法发送数据
        if(UART1_SEND_USE_DMA)
        {
            if(HAL_UART_Transmit_DMA(&huart1, sendData->_buf, sendData->_len) != HAL_OK)
            {
                Error_Handler();
            }
            // 等待发送完成
            osSemaphoreAcquire(uart1SendDone, osWaitForever);            
        }
        else
        {
            if(HAL_UART_Transmit(&huart1, sendData->_buf, sendData->_len, 0xFFFFFFFFU) != HAL_OK)
            {
                Error_Handler();
            } 
        }

        // 删除已发送数据块
        ConstBuf_Delete(sendData);
    }
}

osStatus_t UART1SendData(ConstBuf* data, uint32_t timeout)
{
    if(uart1SendQueue == NULL)
    {
        return osError;
    }

    return osMessageQueuePut(uart1SendQueue, &data, 0, timeout);
}

//********** UART1 接收管理 **********//

// 结果数据块队列长度
const uint32_t UART1_RECEIVE_QUEUE_SIZE = 8;
// 读取缓冲区长度
const uint32_t UART1_RECEIVE_BUF_SIZE = 256;
// 结果数据块插入等待时间
const uint32_t UART1_RECEIVE_WAIT_TIMEOUT = osWaitForever;
// 是否将结果作为字符串处理
const uint8_t UART1_RECEIVE_AS_STRING = 1;
// 是否使用 DMA 进行接收
const uint8_t UART1_REC_USE_DMA = 1;

// 接收数据暂存队列 (以暂存的常量数据块为元素)
osMessageQueueId_t uart1RecQueue = NULL;
// 接收完成信号
osSemaphoreId_t uart1RecDone = NULL;
// 接收缓冲区
ByteBuf* recBuf = NULL;

// 接收直到空闲完成回调函数, 函数的第二个参数为接收到的数据量
void UART1ReceiveCmpltCallBack(UART_HandleTypeDef *huart, uint16_t len)
{
    if(uart1RecDone != NULL)
    {
        recBuf->_len = len;
        osSemaphoreRelease(uart1RecDone);
    }
}

void UART1ReceiveTask(void* args)
{
    // 初始化接收队列, 信号量与缓冲区
    recBuf = ByteBuf_Create(UART1_RECEIVE_BUF_SIZE);
    uart1RecQueue = osMessageQueueNew(UART1_RECEIVE_QUEUE_SIZE, sizeof(ByteBuf*), NULL);
    
    // 注册接收直到空闲回调函数
    if(UART1_REC_USE_DMA)
    {
        uart1RecDone = osSemaphoreNew(1, 0, NULL);
        HAL_UART_RegisterRxEventCallback(&huart1, &UART1ReceiveCmpltCallBack);
    }

    while(1)
    {
        // 使用 HAL 提供的方法发送数据
        if(UART1_REC_USE_DMA)
        {
            if(HAL_UARTEx_ReceiveToIdle_DMA(&huart1, recBuf->_buf, recBuf->_size))
            {
                Error_Handler();
            }
            // 等待一次数据接收完成
            osSemaphoreAcquire(uart1RecDone, osWaitForever);            
        }
        else
        {
            uint16_t len = 0;

            if(HAL_UARTEx_ReceiveToIdle(&huart1, recBuf->_buf, recBuf->_size, &len, 0xFFFFFFFFu))
            {
                Error_Handler();
            }
            recBuf->_len = len;
        }

        // 将缓冲区数据复制到一个常量缓冲区中, 并缓存到接收队列
        ConstBuf* tmpResBuf = ConstBuf_CreateByBuf(recBuf, UART1_RECEIVE_AS_STRING);
        osMessageQueuePut(uart1RecQueue, &tmpResBuf, 0, UART1_RECEIVE_WAIT_TIMEOUT);
    }
}

ConstBuf* UART1ReceiveData(uint32_t timeout)
{
    ConstBuf* tmpResBuf = NULL;
    osMessageQueueGet(uart1RecQueue, &tmpResBuf, NULL, timeout);
    return tmpResBuf;
}
