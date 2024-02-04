#include "byte_buf.h"
#include "cmsis_os.h"

#include "string.h"
#include "stdio.h"
#include "stdarg.h"

const size_t constBufSign = 0xFFFFFFFF;

ByteBuf* ByteBuf_Create(size_t size)
{
    ByteBuf* res = pvPortMalloc(sizeof(ByteBuf));
    res->_buf = pvPortMalloc(size);
    res->_len = 0;
    res->_size = size;

    return res;
}

void ByteBuf_Delete(ByteBuf* obj)
{
    vPortFree(obj->_buf);
    vPortFree(obj);
}

uint8_t ByteBuf_Push(ByteBuf* obj, uint8_t byte)
{
    if(obj->_len >= obj->_size)
    {
        return 0;
    }
    else
    {
        obj->_buf[obj->_len] = byte;
        obj->_len++;
        return 1;
    }
}

void ByteBuf_Flush(ByteBuf* obj)
{
    obj->_len = 0;
}

uint8_t ByteBuf_Printf(ByteBuf* obj, uint8_t is_str, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int res = vsniprintf((char*)obj->_buf, obj->_size, format, args);
    va_end(args);

    if(res >= 0)
    {
        if(is_str)
        {
            obj->_len = res + 1;
        }
        else
        {
            obj->_len = res;
        }

        return 1;
    }
    else
    {
        return 0;
    }
}

//////////////////

ConstBuf* ConstBuf_CreateByBuf(const ByteBuf* obj)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    res->_buf = pvPortMalloc(obj->_len);
    res->_len = obj->_len;
    res->_is_real_const = 0;

    for(size_t i = 0; i < res->_len; i++)
    {
        res->_buf[i] = obj->_buf[i];
    }

    return res;
}

ConstBuf* ConstBuf_CreateByStr(const char* obj)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    res->_buf = (uint8_t*)obj;
    res->_len = strlen(obj);
    res->_is_real_const = 1;

    return res;
}

void ConstBuf_Delete(ConstBuf* obj)
{
    if(!obj->_is_real_const)
    {
        vPortFree(obj->_buf);
    }
    vPortFree(obj);
}
