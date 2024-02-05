#ifndef USER_USB_VPC_DEF
#define USER_USB_VPC_DEF

#include <stdint.h>
#include "byte_buf.h"

void USB_VPC_ReceiveCmpltCallBack(uint32_t len);
ConstBuf* USB_VPC_ReceiveData(uint32_t timeout);

osStatus_t USB_VPC_SendData(ConstBuf* data, uint32_t timeout);

#endif
