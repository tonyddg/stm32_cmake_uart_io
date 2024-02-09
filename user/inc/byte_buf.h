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
#include "cmsis_os.h"

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
    // 有效内容长度 (对于字符串, 将包含末尾的 \0)
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
 * @brief 常量数据块对象
 */
typedef struct CONSTBUF
{
    // 只读数据指针
    uint8_t* _buf;
    // 内容长度 (对于字符串, 将包含末尾的 \0)
    size_t _len;

    // 是否是真常量
    uint8_t _is_real_const;

    // 绑定信号量, 将在数据块销毁时释放, 不会自动创建
    osSemaphoreId_t _sid;
}ConstBuf;

/**
 * @brief 通过已有的数据缓冲区创建只读数据
 * 
 * @param obj 数据缓冲区对象句柄
 * @param is_str 是否扩展为字符串 (若末尾没有 \0, 则在末尾补充 \0)
 * @return ConstBuf* 只读数据对象句柄
 * @note 深构造, 将复制数据缓冲区的有效内容, 并在句柄销毁时一同被销毁
 */
ConstBuf* ConstBuf_CreateByBuf(const ByteBuf* obj, uint8_t is_str);

/**
 * @brief 截取已有的数据缓冲区创建只读数据
 * 
 * @param buf 被截取的缓冲区
 * @param beg 开始截取位置, 包括该位置, 当大于有效位置时, 返回 NULL
 * @param end 停止截取位置, 不包括该位置, 当小于等于 beg 或超过有效长度时则截取到有效末尾
 * @param is_str 是否扩展为字符串 (若末尾没有 \0, 则在末尾补充 \0)
 * @return ConstBuf* 只读数据对象句柄
 * @note 深构造, 将复制数据缓冲区的有效内容, 并在句柄销毁时一同被销毁
 */
ConstBuf* ConstBuf_CreateExtBuf(const uint8_t* buf, size_t buf_len, size_t beg, size_t end, uint8_t is_str);

/**
 * @brief 创建单字节的只读数据
 * 
 * @param byte 只读数据中的字节
 * @return ConstBuf* 只读数据对象句柄
 * @note 不建议使用, 效率较低
 */
ConstBuf* ConstBuf_CreateByByte(uint8_t byte);

/**
 * @brief 通过已有的常量数据创建只读数据
 * 
 * @param buf 常量数据指针
 * @param len 常量数据长度
 * @return ConstBuf* 只读数据对象句柄
 * @note 浅构造, 不会销毁常量数据
 */
ConstBuf* ConstBuf_CreateByConst(const uint8_t* buf, size_t len);

/**
 * @brief 通过已有的常量字符串创建只读数据
 * 
 * @param obj 常量字符串指针
 * @return ConstBuf* 只读数据对象句柄
 * @note 浅构造, 不会销毁常量数据
 */
ConstBuf* ConstBuf_CreateByStr(const char* obj);

/**
 * @brief 创建一个指定长度的空常量缓冲区
 * 
 * @param len 缓冲区长度
 * @return ConstBuf* 只读数据对象句柄
 * @note 用于接收定长数据, 不建议单独使用
 */
ConstBuf* ConstBuf_CreateEmpty(size_t len);

/**
 * @brief 销毁只读数据对象
 * 
 * @param obj 只读数据对象句柄
 * @note 实际根据标识 _is_real_const 决定是否销毁数据指针的内容
 */
void ConstBuf_Delete(ConstBuf* obj);

/**
 * @brief 将常量数据块与信号量绑定, 在数据对象被销毁时释放信号量
 * 
 * @param obj 被绑定的只读数据对象
 * @param sid 待绑定的信号量
 */
void ConstBuf_BindSemaphore(ConstBuf* obj, osSemaphoreId_t sid);

//////////////////////

/**
 * @brief 解析调试字符串 (命令体 + 空格 + 大写 16 进制字符串)
 * 
 * @param str 被解析的常量缓冲区 (末尾不要求有 '\0')
 * @param body 命令体 (字符串, 末尾有 '\0')
 * @param args 命令参数 (一般常量缓冲区, 末尾无 '\0')
 * @return uint8_t 当调试字符串提前结束时返回 0, 成功解析时返回 1
 */
uint8_t CommandResolveText(const ConstBuf* str, ConstBuf** body, ConstBuf** args);

/**
 * @brief 将数据缓冲区的数据转为十六进制的字符串
 * 
 * @param buf 被转换的缓冲区指针
 * @param len 缓冲区长度
 * @return ConstBuf* 转换为字符串的只读数据对象
 */
ConstBuf* ConstBuf_BufToHex(const uint8_t* buf, size_t len);

#endif