set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(TOOLCHAIN_PREFIX                arm-none-eabi)

set(CMAKE_C_COMPILER                $ENV{TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER              $ENV{TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER              $ENV{TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}-g++)

set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)

set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")
