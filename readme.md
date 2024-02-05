# STM32 RTOS HAL IO 示例

一个基于 CubeMX, HAL, CMSIS RTOS 与 CMake 的各种外设 IO 示例项目

## 项目环境
* STM32 CubeMX v6.6.1
* 芯片 STM32F103C8T6
* 项目构建 CMake
* 目标构建 ninja
* 烧录器 openocd 0.12.0-rc2
* 编译器 gcc-arm-none-eabi-10.3-2021.10
* Windows10, vscode

## 包含项目
* `uart_io` 基于 UART 的 IO 范例, 使用外设 UART1, DMA, GPIOC Pin13 (LED) 
* `usb_vpc` 基于 USB VPC 的 IO 范例, 使用外设 USB DEVICE, GPIOC Pin13(LED)

## 部署
### 首次部署
创建文件 `toolchain/config.cmake` 并在其中定义变量 `TOOLCHAIN_PATH` 与 `OpenOCDRoot` 以确定 openocd 与 gcc-arm-none-eabi 的路径  
可参考以下格式

```cmake
# 设置 arm-none-eabi 工具链地址
set(TOOLCHAIN_PATH "path_to/gcc-arm-none-eabi-10.3-2021.10/bin" CACHE PATH "arm-none-eabi 工具链地址")

# OpenOCD 路径
set(OpenOCDRoot "path_to/openocd 0.12.0-rc2" CACHE PATH "OpenOCD 工具链地址")
```

### 一般项目部署
将 `project` 文件夹中的 `CMakeLists_<项目名>.txt` 与 `<项目名>.ioc` 移动到根目录下  

删除 `CMakeLists_<项目名>.txt` 文件名的后缀, 留下 `CMakeLists.txt`  

打开 CubeMX 项目 `<项目名>.ioc`, 并生成代码

### usb_vpc 项目的部署
生成代码后  

在 Core/Src/main.c 中 
* `/* USER CODE BEGIN SysInit */` 下添加 `MX_USB_DEVICE_Init();`  

在 USB_DEVICE/APP/usbd_cdc_if.c 中
* `/* USER CODE BEGIN INCLUDE */` 下添加 `#include "user_usb_vpc.h"`  
* 函数 `CDC_Receive_FS` 末尾添加 `USB_VPC_ReceiveCmpltCallBack(*Len);`

使用该项目前, 请检查有关电路是否正确, 以及安装驱动 <https://www.stmcu.com.cn/Designresource/detail/software/709654>

## 编译
* 目标 `STM32_CMAKE_UART_IO.elf` 将生成用于烧录的 elf 文件
* 伪目标 `DOWNLOAD` 将通过 openOCD 将编译结果通过 DAP 烧录至单片机

## 文件说明
* `toolchain` CMake 工具链文件
* `user` 源代码
    * `byte_buf.c/h` 定义可变与常量缓冲区对象
    * `user_main.c/h` 定义主要任务函数
    * `user_uart.c/h` 定义 UART IO 函数与管理任务
* `project` 部署项目文件

## 基本原理
具体函数见源文件中的 Doxygen 注释

### UART 数据发送
使用一个发送数据暂存队列与发送管理任务实现对于数据发送的管理

在数据发送管理任务 `UART1SendTask` 中 
* 首先在任务启动时创建发送数据暂存队列与发送完成信号量, 并注册发送完成回调 (回调与信号量用于 DMA 模式下)
* 在主循环中, 首先阻塞等待发送数据暂存队列中有待发送的数据块
* 一旦有数据块进入队列, 立即读取并通过 HAL 的方法 (DMA 或阻塞) 进行发送
* 发送结束后, 由管理任务负责删除数据块对象

发送数据函数 `UART1SendData` 中
* 通过 `ConstBuf_CreateByBuf/Str` 基于缓冲区的有效内容 (复制) 或常量字符串 (直接引用) 创建常量数据块 
* 将常量数据块句柄传入发送数据暂存队列

### UART 数据接收
使用一个接收数据暂存队列与接收管理任务实现对于数据发送的管理

在数据发送管理任务 `UART1RecTask` 中 
* 首先在任务启动时创建发送数据暂存队列与发送完成信号量, 并注册发送完成回调 (回调与信号量用于 DMA 模式下)
* 在主循环中, 首先通过 HAL 的方法 (DMA 或阻塞) 将数据接收到缓冲区中, 直到 RX 空闲, 其中 DMA 将阻塞直到接收完成
* 一旦接收完成, 创建数据块并将缓冲区中的有效数据复制到数据块中, 并插入接收数据暂存队列
* 插入完成后, 开始下一次数据接收

发送数据函数 `UART1RecData` 中
* 以常量数据块句柄 `ConstBuf*` 的方式返回接收到的数据块  
* 由接收者负责销毁数据块

其余外设与 UART 基本相同

## TODO
* 关于缓冲区与常量数据块的说明
* 其他外设的 IO 示例
