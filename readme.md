# STM32 CMAKE UART IO

一个基于 CubeMX, HAL, CMSIS RTOS 与 CMake 的 UART IO 示例项目

## 项目环境
* STM32 CubeMX v6.6.1
* 芯片 STM32F103C8T6
* 项目构建 CMake
* 目标构建 ninja
* 烧录器 openocd 0.12.0-rc2
* 编译器 gcc-arm-none-eabi-10.3-2021.10
* Windows10, vscode

## 部署
首先打开 CubeMX 项目文件 `stm32_cmake_uart_io.ioc` 并生成代码

之后创建文件 `toolchain/config.cmake` 并在其中定义变量 `TOOLCHAIN_PATH` 与 `OpenOCDRoot` 以确定 openocd 与 gcc-arm-none-eabi 的路径  
可参考以下格式

```cmake
# 设置 arm-none-eabi 工具链地址
set(TOOLCHAIN_PATH "path_to/gcc-arm-none-eabi-10.3-2021.10/bin" CACHE PATH "arm-none-eabi 工具链地址")

# OpenOCD 路径
set(OpenOCDRoot "path_to/openocd 0.12.0-rc2" CACHE PATH "OpenOCD 工具链地址")
```

## 编译
* 目标 `STM32_CMAKE_UART_IO.elf` 将生成用于烧录的 elf 文件
* 伪目标 `DOWNLOAD` 将通过 openOCD 将编译结果通过 DAP 烧录至单片机

## 文件说明
* `toolchain` CMake 工具链文件
* `user` 源代码
    * `byte_buf.c/h` 定义可变与常量缓冲区对象
    * `user_main.c/h` 定义主要任务函数
    * `user_uart.c/h` 定义 UART IO 函数与管理任务

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

