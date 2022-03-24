set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/sdcc-toolchain.cmake)

project(darkroom_timer C)
cmake_minimum_required(VERSION 3.18)

add_compile_definitions(
  __CONF_FOSC=24000000UL
  __CONF_MCU_MODEL=MCU_MODEL_STC8H1K08
  STC8_UART_BASIC_OPS=1
)

file(GLOB LIB_SRCS FwLib_STC8/src/fw_tim.c FwLib_STC8/src/fw_uart.c FwLib_STC8/src/fw_sys.c)
add_executable(firmware main.c lcd.c alarm.c input.c statemachine.c relay.c ${LIB_SRCS})
include_directories(${CMAKE_SOURCE_DIR}/FwLib_STC8/include)
add_compile_options(--code-size 8192 --iram-size 256 --xram-size 1024 --stack-auto --opt-code-size)