/**
 * @file byte_buf.h
 * @author tonyddg (tonyddg@outlook.com)
 * @brief 定义内存管理类
 * @version 0.1
 * @date 2024-02-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef BYTE_BUF_DEF
#define BYTE_BUF_DEF

#include <stdint.h>
typedef unsigned int size_t;

/**
 * @brief 数据缓冲区  
 * @brief 用于保存长度不确定的可变数据
 * @attention 当数据有效长度改变时, 需要即使修改成员 _len  
 * @attention 如使用 string 的字符串处理函数后, 使用 _len 接收修改结果长度
 */
typedef struct BYTEBUF
{
    // 缓冲区指针
    uint8_t* _buf;
    // 有效内容长度
    size_t _len;
    // 缓冲区长度
    size_t _size;
}ByteBuf;

/**
 * @brief 创建数据缓冲区句柄
 * 
 * @param size 数据缓冲区的总大小
 * @return ByteBuf* 数据缓冲区对象句柄
 */
ByteBuf* ByteBuf_Create(size_t size);

/**
 * @brief 删除数据缓冲区
 * 
 * @param obj 数据缓冲区对象句柄
 */
void ByteBuf_Delete(ByteBuf* obj);

/**
 * @brief 向数据缓冲区末尾填充一个字节
 * 
 * @param obj 数据缓冲区对象句柄
 * @param byte 填充字节
 * @return uint8_t 当缓冲区已满填充失败时返回 0, 否则返回 1
 */
uint8_t ByteBuf_Push(ByteBuf* obj, uint8_t byte);

/**
 * @brief 清空缓冲区
 * 
 * @param obj 数据缓冲区对象句柄
 */
void ByteBuf_Flush(ByteBuf* obj);

/**
 * @brief 格式化输出至缓冲区
 * 
 * @param obj 数据缓冲区对象句柄
 * @param is_str 是否视为字符串, 若视为字符串, 则将在末尾添加 \0; 对于发送的数据可不视为字符串
 * @param format 格式化字符串, 同 printf
 * @param ... 格式化参数
 * @return uint8_t 当写入成功时返回 1, 否则空间不足时返回 0
 * @example ByteBuf_Printf(printBuf, 0, "[REC]%s[REC]\r\n", resBuf->_buf);
 */
uint8_t ByteBuf_Printf(ByteBuf* obj, uint8_t is_str, const char* format, ...);

///////////////////

/**
 * @brief 只读数据对象
 */
typedef struct CONSTBUF
{
    // 只读数据指针
    uint8_t* _buf;
    // 内容长度
    size_t _len;

    // 是否是真常量
    uint8_t _is_real_const;
}ConstBuf;

/**
 * @brief 通过已有的数据缓冲区创建只读数据
 * 
 * @param obj 数据缓冲区对象句柄
 * @return ConstBuf* 只读数据对象句柄
 * @note 深构造, 将复制数据缓冲区的有效内容, 并在句柄销毁时一同被销毁
 */
ConstBuf* ConstBuf_CreateByBuf(const ByteBuf* obj);

/**
 * @brief 通过已有的常量数据创建只读数据
 * 
 * @param obj 常量数据指针
 * @return ConstBuf* 只读数据对象句柄
 * @note 浅构造, 不会销毁常量数据
 */
ConstBuf* ConstBuf_CreateByStr(const char* obj);

/**
 * @brief 销毁只读数据对象
 * 
 * @param obj 只读数据对象句柄
 * @note 实际根据标识 _is_real_const 决定是否销毁数据指针的内容
 */
void ConstBuf_Delete(ConstBuf* obj);

#endif