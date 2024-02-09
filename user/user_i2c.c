#ifdef USE_I2C

#include "stm32f1xx_hal.h"
#include "i2c.h"
#include "cmsis_os.h"

#include "string.h"

#include "user_i2c.h"
#include "byte_buf.h"

/// @brief I2C 行动类型
typedef enum I2CACTTYPE
{
    /// @brief 接收数据
    I2C_ACT_REC,
    /// @brief 发送数据
    I2C_ACT_SEND,
    /// @brief 测试设备
    I2C_ACT_TOUCH
} I2CActType;

typedef struct I2CDATAFRAME
{
    // I2C 动作类型
    I2CActType _actType;
    // I2C 设备地址
    uint8_t _daddr;
    // I2C 设备寄存器地址, 当测试设备时为测试次数
    uint8_t _raddr;
    // I2C 接收 / 发送数据
    ConstBuf* _data;
    // 动作执行完成时释放的信号量
    void* _callBack;
} I2CDataFrame;

I2CDataFrame* I2CDataFrame_Create(uint8_t daddr, uint8_t raddr, ConstBuf* data, I2CActType type, void* callBack)
{
    I2CDataFrame* res = pvPortMalloc(sizeof(I2CDataFrame));
    res->_daddr = daddr;
    res->_raddr = raddr;
    res->_data = data;
    res->_actType = type;
    res->_callBack = callBack;
    return res;
}

void I2CDataFrame_Delete(I2CDataFrame* obj, uint8_t is_success)
{
    switch (obj->_actType)
    {
    case I2C_ACT_REC:
    {
        if(is_success)
        {
            if(obj->_callBack != NULL)
            {
                I2CRecCallbackTypeDef callBack = obj->_callBack;
                callBack(1, obj->_data);
            }
            else
            {
                ConstBuf_Delete(obj->_data);
            }
        }
        else
        {
            ConstBuf_Delete(obj->_data);
            if(obj->_callBack != NULL)
            {
                I2CRecCallbackTypeDef callBack = obj->_callBack;
                callBack(0, NULL);
            }
        }
        break;
    }
    case I2C_ACT_SEND:
    {
        if(obj->_callBack != NULL)
        {
            I2CNormalCallbackTypeDef callBack = obj->_callBack;
            callBack(is_success);
        }
        ConstBuf_Delete(obj->_data);
        break;
    }
    case I2C_ACT_TOUCH:
    {
        if(obj->_callBack != NULL)
        {
            I2CNormalCallbackTypeDef callBack = obj->_callBack;
            callBack(is_success);
        }
        break;
    }
    }
    vPortFree(obj);
}

/////////////////////////////

// I2C 发送队列长度
const uint32_t I2C_DATA_QUEUE_SIZE = 12;
// I2C 平均每次测试时长
const uint32_t I2C_WAIT_TIMEOUT = 100;
// I2C 是否启用 DMA 传输
#define I2C_USE_DMA 1

// 发送数据暂存队列 (以暂存的常量数据块为元素)
osMessageQueueId_t i2c1DataQueue = NULL;

#if (I2C_USE_DMA == 1)
osSemaphoreId_t i2cFrameDone = NULL;

void I2CFrameDoneCallBack(I2C_HandleTypeDef* hi2c)
{
    if(i2cFrameDone != NULL)
    {
        osSemaphoreRelease(i2cFrameDone);
    }
}

#endif

/**
 * @brief 向任务队列插入新的任务
 * 
 * @param frame I2C 任务帧句柄
 * @param timeout 等待时长
 * @return osStatus_t 插入任务队列状态
 */
osStatus_t I2CPutFrame(I2CDataFrame* frame, uint32_t timeout)
{
    osStatus_t res = osMessageQueuePut(i2c1DataQueue, &frame, 0, timeout);

    if(res != osOK)
    {
        I2CDataFrame_Delete(frame, 0);
    }
    return res;
}

osStatus_t I2CSendData(uint8_t daddr, uint8_t raddr, ConstBuf* data, I2CNormalCallbackTypeDef callBack, uint32_t timeout)
{
    return I2CPutFrame(I2CDataFrame_Create(daddr, raddr, data, I2C_ACT_SEND, callBack), timeout);
}

osStatus_t I2CRecData(uint8_t daddr, uint8_t raddr, size_t len, I2CRecCallbackTypeDef callBack, uint32_t timeout)
{
    return I2CPutFrame(I2CDataFrame_Create(daddr, raddr, ConstBuf_CreateEmpty(len), I2C_ACT_REC, callBack), timeout);
}

osStatus_t I2CTouch(uint8_t daddr, uint8_t trail, I2CNormalCallbackTypeDef callBack, uint32_t timeout)
{
    return I2CPutFrame(I2CDataFrame_Create(daddr, trail, NULL, I2C_ACT_TOUCH, callBack), timeout);
}

void I2CManageTask(void* args)
{
    I2CDataFrame* queueData = NULL;
    i2c1DataQueue = osMessageQueueNew(I2C_DATA_QUEUE_SIZE, sizeof(I2CDataFrame*), NULL);

    #if (I2C_USE_DMA == 1)
    
    i2cFrameDone = osSemaphoreNew(1, 0, NULL);
    HAL_I2C_RegisterCallback(&hi2c1, HAL_I2C_MEM_TX_COMPLETE_CB_ID, I2CFrameDoneCallBack);
    HAL_I2C_RegisterCallback(&hi2c1, HAL_I2C_MEM_RX_COMPLETE_CB_ID, I2CFrameDoneCallBack);

    #endif

    while(1)
    {
        osMessageQueueGet(i2c1DataQueue, &queueData, NULL, osWaitForever);
        uint8_t is_success = 0;

        switch(queueData->_actType)
        {
        case I2C_ACT_REC:
        {
        #if (I2C_USE_DMA == 1)
            if(HAL_I2C_Mem_Read_DMA(
                &hi2c1,
                queueData->_daddr,
                queueData->_raddr,
                I2C_MEMADD_SIZE_8BIT,
                queueData->_data->_buf,
                queueData->_data->_len
            ) == HAL_OK)
            {
                if(osSemaphoreAcquire(i2cFrameDone, I2C_WAIT_TIMEOUT) == osOK)
                {
                    is_success = 1;
                }
            }
            break;
        #else
            if(HAL_I2C_Mem_Read(
                &hi2c1,
                queueData->_daddr,
                queueData->_raddr,
                I2C_MEMADD_SIZE_8BIT,
                queueData->_data->_buf,
                queueData->_data->_len,
                I2C_WAIT_TIMEOUT
            ) == HAL_OK)
            {
                is_success = 1;
            }
            break;
        #endif
        }
        case I2C_ACT_SEND:
        {
        #if (I2C_USE_DMA == 1)
            if(HAL_I2C_Mem_Write_DMA(
                &hi2c1,
                queueData->_daddr,
                queueData->_raddr,
                I2C_MEMADD_SIZE_8BIT,
                queueData->_data->_buf,
                queueData->_data->_len
            ) == HAL_OK)
            {
                if(osSemaphoreAcquire(i2cFrameDone, I2C_WAIT_TIMEOUT) == osOK)
                {
                    is_success = 1;
                }
            }
            break;
        #else
            if(HAL_I2C_Mem_Write(
                &hi2c1,
                queueData->_daddr,
                queueData->_raddr,
                I2C_MEMADD_SIZE_8BIT,
                queueData->_data->_buf,
                queueData->_data->_len,
                I2C_WAIT_TIMEOUT
            ) == HAL_OK)
            {
                is_success = 1;
            }
            break;
        #endif
        }
        case I2C_ACT_TOUCH:
        {
            if(HAL_I2C_IsDeviceReady(
                &hi2c1,
                queueData->_daddr,
                queueData->_raddr,
                I2C_WAIT_TIMEOUT
            ) == HAL_OK)
            {
                is_success = 1;
            }
            break;
        }
        }
        I2CDataFrame_Delete(queueData, is_success);
    }

    return;
}

I2CTaskState I2CGetTaskState()
{
    if(i2c1DataQueue == NULL)
    {
        return I2C_TASK_UNINIT;
    }
    else if(HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_ERROR)
    {
        return I2C_TASK_ERROR;
    }
    else if(HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_RESET)
    {
        return I2C_TASK_RESET;
    }
    else if(osMessageQueueGetCount(i2c1DataQueue) == I2C_DATA_QUEUE_SIZE)
    {
        return I2C_TASK_QUEUEFULL;
    }
    else
    {
        return I2C_TASK_READY;
    }
}

#endif