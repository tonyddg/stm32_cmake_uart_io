# 指定编译平台/架构与语言标准, 推荐指定 Ninja 为构建工具,可以加快编译速度(相比make)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 指定工具链
set(CMAKE_C_COMPILER_FORCED TRUE) # skip compiler test
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-g++.exe)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe)
set(CMAKE_LINKER ${TOOLCHAIN_PATH}/arm-none-eabi-ld.exe) # 根据知乎介绍补充

set(CMAKE_OBJCOPY ${TOOLCHAIN_PATH}/arm-none-eabi-objcopy.exe)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PATH}/arm-none-eabi-objdump.exe)
set(SIZE ${TOOLCHAIN_PATH}/arm-none-eabi-size.exe) 
set(CMAKE_AR ${TOOLCHAIN_PATH}/arm-none-eabi-ar.exe)

# cube 自动生成的 .ld 链接脚本 
set(LINK_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F103C8Tx_FLASH.ld) 

# -mcpu= 根据芯片特点设置 (可参考自动生成的 Makefile 中的 CPU 与 MCU 配置)
# -mfloat-abi=hard -mfpu=fpv4-sp-d16 用于开启浮点计算单元 FPU 的芯片 (通常为 stm32f4xx, 可参考自动生成的 Makefile 中的 FPU 配置)
set(MPU_FLAG "-mcpu=cortex-m3 -mthumb")

# 预定义宏 (可参考自动生成的 Makefile 中的 C_DEFS 配置)
add_definitions(
    -DUSE_HAL_DRIVER
    -DSTM32F103xB
    )

# 设置特定的编译和链接标志

set(CMAKE_C_FLAGS ${MPU_FLAG})
set(CMAKE_CXX_FLAGS ${MPU_FLAG})
set(CMAKE_EXE_LINKER_FLAGS -T${LINK_SCRIPT})

# OpenOCD 设置

# 可执行文件地址
set(OpenOCDPath ${OpenOCDRoot}/bin/openocd.exe)
# 烧录器配置路径
set(OpenOCPInterface ${OpenOCDRoot}/share/openocd/scripts/interface/cmsis-dap.cfg)
# 目标芯片路径
set(OpenOCPTarget ${OpenOCDRoot}/share/openocd/scripts/target/stm32f1x.cfg)
