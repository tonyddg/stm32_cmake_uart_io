#ifndef USER_I2C_DEF
#define USER_I2C_DEF

#include "stdint.h"
#include "byte_buf.h"

/**
 * @brief I2C 发送 / 设备测试完成回调
 * @note `is_success` 为 1 表明操作成功, 为 0 表明操作失败  
 */
typedef  void (*I2CNormalCallbackTypeDef)(uint8_t is_success); 

/**
 * @brief I2C 接收完成回调
 * @note `is_success` 为 1 表明操作成功, 为 0 表明操作失败  
 * @note `data` 接收到的数据, 接收成功时由回调函数负责销毁, 接收失败时为 NULL
 */
typedef  void (*I2CRecCallbackTypeDef)(uint8_t is_success, ConstBuf* data); 

/**
 * @brief 从 I2C 总线上发送数据
 * 
 * @param daddr I2C 设备地址
 * @param raddr I2C 设备寄存器地址
 * @param data 待发送的数据
 * @param callBack 发送成功 / 失败回调, 若传入 NULL 则不进行回调
 * @param timeout 等待插入任务队列的时间
 * @return osStatus_t 插入任务队列状态
 */
osStatus_t I2CSendData(uint8_t daddr, uint8_t raddr, ConstBuf* data, I2CNormalCallbackTypeDef callBack, uint32_t timeout);

/**
 * @brief 从 I2C 总线上读取数据
 * 
 * @param daddr I2C 设备地址
 * @param raddr I2C 设备寄存器地址
 * @param len 读取数据长度 (字节数)
 * @param callBack 发送成功 / 失败回调, 若传入 NULL 则不进行回调
 * @param timeout 等待插入任务队列的时间
 * @return osStatus_t 插入任务队列状态
 */
osStatus_t I2CRecData(uint8_t daddr, uint8_t raddr, size_t len, I2CRecCallbackTypeDef callBack, uint32_t timeout);

/**
 * @brief 测试 I2C 总线上的设备
 * 
 * @param daddr I2C 设备地址
 * @param trail 测试次数
 * @param callBack 测试成功 / 失败回调, 若传入 NULL 则不进行回调
 * @param timeout 等待插入任务队列的时间
 * @return osStatus_t 插入任务队列状态
 */
osStatus_t I2CTouch(uint8_t daddr, uint8_t trail, I2CNormalCallbackTypeDef callBack, uint32_t timeout);

/// @brief I2C 管理任务状态
typedef enum I2CTASKSTATE
{
    // 就绪
    I2C_TASK_READY,
    // 未初始化
    I2C_TASK_UNINIT,
    // 任务队列已满
    I2C_TASK_QUEUEFULL,
    // 外设错误
    I2C_TASK_ERROR,
    // 外设未初始化
    I2C_TASK_RESET
} I2CTaskState;

/**
 * @brief 获取当前 I2C 管理任务状态
 * 
 * @return I2CTaskState I2C 管理任务状态
 */
I2CTaskState I2CGetTaskState();

#endif