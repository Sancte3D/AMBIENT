cmake_minimum_required(VERSION 3.16)
get_filename_component(FAM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${FAM_ROOT}/cmake/arm-none-eabi-m7.cmake" CACHE FILEPATH "H743 cross toolchain")
endif()
project(ambient_scene_flash_bench C ASM)
set(CMAKE_C_STANDARD 11)
set(VENDOR "${FAM_ROOT}/src/hal_h743/vendor")
set(HAL "${VENDOR}/STM32H7xx_HAL_Driver")
set(CMSIS "${VENDOR}/CMSIS")
set(HAL_SOURCES
    ${HAL}/Src/stm32h7xx_hal.c ${HAL}/Src/stm32h7xx_hal_cortex.c
    ${HAL}/Src/stm32h7xx_hal_rcc.c ${HAL}/Src/stm32h7xx_hal_rcc_ex.c
    ${HAL}/Src/stm32h7xx_hal_flash.c ${HAL}/Src/stm32h7xx_hal_flash_ex.c
    ${HAL}/Src/stm32h7xx_hal_pwr.c ${HAL}/Src/stm32h7xx_hal_pwr_ex.c)
add_executable(scene_flash_bench
    main.c bench.c interrupts.c busfault_vector.S
    ${FAM_ROOT}/src/scenes_journal.c ${FAM_ROOT}/src/scenes_journal_async.c
    ${FAM_ROOT}/src/scenes_journal_recovery.c
    ${FAM_ROOT}/src/hal_h743/scenes_flash_h743_async.c
    ${FAM_ROOT}/src/hal_h743/scenes_flash_h743_fault.c
    ${FAM_ROOT}/src/hal_h743/scenes_flash_h743_guard.S
    ${CMSIS}/Device/ST/STM32H7xx/Source/system_stm32h7xx.c
    ${CMSIS}/Device/ST/STM32H7xx/Source/startup_stm32h743xx.s
    ${HAL_SOURCES})
target_include_directories(scene_flash_bench PRIVATE
    ${FAM_ROOT}/include ${FAM_ROOT}/src/hal_h743 ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMSIS}/Include ${CMSIS}/Device/ST/STM32H7xx/Include ${HAL}/Inc)
target_compile_definitions(scene_flash_bench PRIVATE STM32H743xx USE_HAL_DRIVER)
target_compile_options(scene_flash_bench PRIVATE -O2 -g3 -Wall -Wextra -Werror -fstack-usage
    -fdebug-prefix-map=${FAM_ROOT}=firmware-c-next
    -fdebug-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=build-scene-bench)
set_source_files_properties(main.c interrupts.c
    ${FAM_ROOT}/src/hal_h743/scenes_flash_h743_async.c
    ${FAM_ROOT}/src/hal_h743/scenes_flash_h743_fault.c
    PROPERTIES COMPILE_OPTIONS "-mgeneral-regs-only")
set_source_files_properties(${HAL_SOURCES}
    ${CMSIS}/Device/ST/STM32H7xx/Source/system_stm32h7xx.c
    PROPERTIES COMPILE_OPTIONS "-w")
target_link_options(scene_flash_bench PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/bench.ld
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/scene_flash_bench.map -Wl,--print-memory-usage)
add_custom_command(TARGET scene_flash_bench POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:scene_flash_bench> ${CMAKE_CURRENT_BINARY_DIR}/scene_flash_bench.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:scene_flash_bench> ${CMAKE_CURRENT_BINARY_DIR}/scene_flash_bench.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:scene_flash_bench>)
