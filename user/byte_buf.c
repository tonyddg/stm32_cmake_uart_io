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

ConstBuf* ConstBuf_CreateByBuf(const ByteBuf* obj, uint8_t is_str)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    // 当缓冲区已经满足字符串要求时, 不再修改
    if(is_str && (obj->_buf[obj->_len - 1] == 0))
    {
        is_str = 0;
    }

    if(is_str)
    {
        res->_len = obj->_len + 1;
    }
    else
    {
        res->_len = obj->_len;
    }

    res->_buf = pvPortMalloc(res->_len);
    res->_is_real_const = 0;
    res->_sid = NULL;

    for(size_t i = 0; i < obj->_len; i++)
    {
        res->_buf[i] = obj->_buf[i];
    }

    if(is_str)
    {
        res->_buf[res->_len - 1] = 0;
    }

    return res;
}

ConstBuf* ConstBuf_CreateExtBuf(const uint8_t* buf, size_t buf_len, size_t beg, size_t end, uint8_t is_str)
{
    if(beg >= buf_len)
    {
        return NULL;
    }

    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    if(end <= beg || end > buf_len)
    {
        end = buf_len;
    }
    size_t len = end - beg;

    // 当缓冲区已经满足字符串要求时, 不再修改
    if(is_str && (buf[end - 1] == 0))
    {
        is_str = 0;
    }

    if(is_str)
    {
        res->_len = len + 1;
    }
    else
    {
        res->_len = len;
    }

    res->_buf = pvPortMalloc(len);
    res->_is_real_const = 0;
    res->_sid = NULL;

    for(size_t i = beg; i < end; i++)
    {
        res->_buf[i - beg] = buf[i];
    }

    if(is_str)
    {
        res->_buf[res->_len - 1] = 0;
    }

    return res;
}

ConstBuf* ConstBuf_CreateByByte(uint8_t byte)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));
    res->_buf = pvPortMalloc(1);
    res->_is_real_const = 0;
    res->_len = 1;
    res->_sid = NULL;

    res->_buf[0] = byte;

    return res;
}

ConstBuf* ConstBuf_CreateByConst(const uint8_t* buf, size_t len)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    res->_buf = (uint8_t*)buf;
    res->_len = len;
    res->_is_real_const = 1;
    res->_sid = NULL;

    return res;
}

ConstBuf* ConstBuf_CreateByStr(const char* obj)
{
    return ConstBuf_CreateByConst((const uint8_t*)obj, strlen(obj) + 1);
}

ConstBuf* ConstBuf_CreateEmpty(size_t len)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    res->_buf = pvPortMalloc(len);
    res->_len = len;
    res->_is_real_const = 0;
    res->_sid = NULL;

    return res;
}

void ConstBuf_Delete(ConstBuf* obj)
{
    if(!obj->_is_real_const)
    {
        vPortFree(obj->_buf);
    }

    if(obj->_sid != NULL)
    {
        osSemaphoreRelease(obj->_sid);
    }

    vPortFree(obj);
}

void ConstBuf_BindSemaphore(ConstBuf* obj, osSemaphoreId_t sid)
{
    obj->_sid = sid;
}

///////////////////////////

uint8_t CommandResolveText(const ConstBuf* str, ConstBuf** body, ConstBuf** args)
{
    uint32_t r = 0;
    while(str->_buf[r] != ' ')
    {
        r++;
        if(r >= str->_len)
        {
            return 0;
        }
    }
    *body = ConstBuf_CreateExtBuf(str->_buf, str->_len, 0, r, 1);

    while(str->_buf[r] == ' ')
    {
        r++;
        if(r >= str->_len)
        {
            ConstBuf_Delete(*body);
            return 0;
        }
    }

    ByteBuf* tmpBuf = ByteBuf_Create((str->_len - r) / 2);

    uint32_t l = r;
    uint8_t left_part = 0;
    uint8_t right_part = 0;
    while((str->_buf[r] >= 'A' && str->_buf[r] <= 'F') || (str->_buf[r] >= '0' && str->_buf[r] <= '9'))
    {
        if(str->_buf[r] >= 'A' && str->_buf[r] <= 'F')
        {
            right_part = str->_buf[r] - 'A' + 10;
        }
        else
        {
            right_part = str->_buf[r] - '0';
        }

        if((r - l) % 2 == 1)
        {
            ByteBuf_Push(tmpBuf, left_part + right_part);
        }
        else
        {
            left_part = right_part << 4;
        }

        r++;
        if(r >= str->_len)
        {
            break;
        }
    }

    *args = ConstBuf_CreateByBuf(tmpBuf, 0);
    ByteBuf_Delete(tmpBuf);

    return 1;
}

ConstBuf* ConstBuf_BufToHex(const uint8_t* buf, size_t len)
{
    ConstBuf* res = pvPortMalloc(sizeof(ConstBuf));

    res->_buf = pvPortMalloc(len * 2 + 1);
    res->_len = len * 2 + 1;
    res->_is_real_const = 0;
    res->_sid = NULL;

    for(size_t i = 0; i < len * 2; i++)
    {
        uint8_t tmp = buf[i / 2];
        if(i % 2 == 0)
        {
            tmp = tmp >> 4; 
        }
        tmp = tmp & 0b00001111;

        if(tmp >= 0xAu)
        {
            res->_buf[i] = tmp + 'A' - 0xAu;
        }
        else
        {
            res->_buf[i] = tmp + '0';
        }
    }

    res->_buf[len * 2] = '\0';
    return res;
}
