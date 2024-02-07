#ifndef USER_UART_DEF
#define USER_UART_DEF

#include "stdint.h"
#include "cmsis_os.h"
#include "byte_buf.h"

typedef enum UARTSENDSTATE
{
    // 就绪
    UART_SEND_READY,
    // 未初始化
    UART_SEND_UNINIT,
    // 发送队列已满
    UART_SEND_QUEUEFULL,
    // 外设错误
    UART_SEND_ERROR,
    // 外设未初始化
    UART_SEND_RESET
} UARTSendState;

/**
 * @brief 通过 UART1 异步发送数据
 * 
 * @param data 常量数据对象句柄, 通过 ConstBuf_CreateBy... 创建, 且由发送任务负责销毁  
 * @param timeout 插入队列等待时间, 即 osMessageQueuePut 的 timeout 参数
 * @return osStatus_t 插入队列执行结果, 当队列未正确初始化时, 将返回 osError
 * @note 使用该函数前, 任务 `UART1SendTask` 必须运行中  
 * @note 该函数为线程安全的, 建议使用此函数发送数据, 而非 HAL_UART_Transmit 
 * @example UART1SendData(ConstBuf_CreateByStr("Hello World\r\n", 0), osWaitForever);
 */
osStatus_t UART1SendData(ConstBuf* data, uint32_t timeout);

/**
 * @brief 获取当前 UART 发送任务状态
 * 
 * @return UARTSendState 当前发送任务状态
 */
UARTSendState UART1SendGetState();

typedef enum UARTRECSTATE
{
    // 就绪
    UART_REC_READY,
    // 未初始化
    UART_REC_UNINIT,
    // 发送队列空
    UART_REC_EMPTY,
    // 外设错误
    UART_REC_ERROR,
    // 外设未初始化
    UART_REC_RESET
} UARTRecState;

/**
 * @brief 通过 UART1 等待接收数据
 * 
 * @param timeout 接收队列等待时间, 即 osMessageQueuePut 的 timeout 参数
 * @return ConstBuf* 接收到的常量数据块, 由接收者负责销毁
 * @note 使用该函数前, 任务 `UART1ReceiveTask` 必须运行中  
 * @note 该函数为线程安全的, 建议使用此函数接收数据, 而非 HAL_UART_Receive 
 * @example resBuf = UART1ReceiveData(osWaitForever);
 */
ConstBuf* UART1ReceiveData(uint32_t timeout);

/**
 * @brief 获取当前 UART 接收任务状态
 * 
 * @return UARTRecState 当前发送任务状态
 */
UARTRecState UART1ReceiveGetState();

#endif
