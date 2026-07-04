# arm-none-eabi-m7.cmake — cross-toolchain for the STM32H743 product build
# (r18.86, Step 13.3). Auto-included by CMakeLists.txt when FAM_TARGET=h743;
# can also be passed explicitly with -DCMAKE_TOOLCHAIN_FILE=….
#
# CPU: Cortex-M7 r1p1, double-precision FPU (FPv5-D16), hard-float ABI.
# The DSP code is float32 throughout — hard-float is what makes the
# engine_render() budget (512 frames in <11.6 ms) trivially achievable.
#
# libc: newlib-nano (--specs=nano.specs) + no-syscall stubs (nosys.specs).
# printf-float is NOT enabled — nothing in the product firmware printf()s
# floats (the LCD renders text itself); add -u _printf_float if that changes.

set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  cortex-m7)

set(CMAKE_C_COMPILER    arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER  arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_OBJCOPY       arm-none-eabi-objcopy)
set(CMAKE_SIZE          arm-none-eabi-size)

# A bare-metal link has no default entry/syscalls, so a test *executable*
# can't link during CMake's compiler sanity check — build a static lib
# instead.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(_m7_flags "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT   "${_m7_flags} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "${_m7_flags} -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS_INIT "${_m7_flags}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${_m7_flags} --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections")

# Never pick up host libraries/headers.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
