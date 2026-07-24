/**
  @page FreeRTOS_MPU FreeRTOS MPU application

  @verbatim
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_MPU/readme.txt
  * @author  MCD Application Team
  * @brief   Description of the FreeRTOS MPU application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim

@par Application Description

How to use the MPU feature of FreeRTOS.

This application demonstrates the use of the MPU with FreeRTOS to control memory/peripheral access for tasks.
It implements simple recovery mechanisms from prohibited memory accesses.
The application creates one Privileged mode task using xTaskCreate():

  - 'MainTask': Has higher priority than the unprivileged tasks.
                Blocks while waiting for a fault to occur.
                Displays information about the faults and takes corrective actions.
It also has User mode tasks created using the xTaskCreateRestricted() API:

  - 'Task1': Has read only access to shared memory.
             Attempts an illegal write to the shared memory only once.
             Toggles the green LED.
  - 'Task2': Has read only access to shared memory.
             Attempts an illegal write to the shared memory periodically.
             Toggles the red LED.

The application will try to recover from MemManage fault exception for each task until EXAMPLE_FAULT_COUNT_THRESHOLD is reached for that task. At that point the task will be suspended to
avoid further exceptions.

Successful operation is marked by a toggling green led light, the red LED will toggle `EXAMPLE_FAULT_COUNT_THRESHOLD` times, and detailed fault information will be printed to the serial port.

On failure, a "Critical Error occurred" message will be printed to the serial port.

@note The FreeRTOS heap size configTOTAL_HEAP_SIZE defined in FreeRTOSConfig.h is set accordingly to the
      OS resources memory requirements of the application with +10% margin and rounded to the upper Kbyte boundary.

For more details about FreeRTOS implementation on STM32Cube, please refer to UM1722 "Developing Applications
on STM32Cube with RTOS".

@Note If the  application is using the DTCM/ITCM memories (@0x20000000/ 0x0000000: not cacheable and only accessible
      by the Cortex M7 and the  MDMA), no need for cache maintenance when the Cortex M7 and the MDMA access these RAMs.
      If the application needs to use DMA(or other masters) based access or requires more RAM, then  the user has to:
      - Use a non TCM SRAM. (example : D1 AXI-SRAM @ 0x24000000)
      - Add a cache maintenance mechanism to ensure the cache coherence between CPU and other masters(DMAs,DMA2D,LTDC,MDMA).
      - The addresses and the size of cacheable buffers (shared between CPU and other masters)
        must be properly defined to be aligned to L1-CACHE line size (32 bytes).

@Note It is recommended to enable the cache and maintain its coherence.
      Depending on the use case it is also possible to configure the cache attributes using the MPU.
      Please refer to the AN4838 "Managing memory protection unit (MPU) in STM32 MCUs"
      Please refer to the AN4839 "Level 1 cache on STM32F7 Series"

@par Keywords

RTOS, FreeRTOS, MPU

@par Directory contents
    - FreeRTOS/FreeRTOS_MPU/Src/main.c                       Main program
    - FreeRTOS/FreeRTOS_MPU/Src/stm32h7xx_hal_timebase_tim.c HAL timebase file
    - FreeRTOS/FreeRTOS_MPU/Src/stm32h7xx_it.c               Interrupt handlers
    - FreeRTOS/FreeRTOS_MPU/Src/system_stm32h7xx.c           STM32H7xx system clock configuration file
    - FreeRTOS/FreeRTOS_MPU/Src/stm32l5xx_hal_msp.c          MSP Initialization and de-Initialization codes
    - FreeRTOS/FreeRTOS_MPU/Inc/main.h                       Main program header file
    - FreeRTOS/FreeRTOS_MPU/Inc/stm32h7xx_hal_conf.h         HAL Library Configuration file
    - FreeRTOS/FreeRTOS_MPU/Inc/stm32h7xx_it.h               Interrupt handlers header file
    - FreeRTOS/FreeRTOS_MPU/Inc/FreeRTOSConfig.h             FreeRTOS Configuration file

@par Hardware and Software environment

  - This example runs on STM32H743xx devices.

  - This example has been tested with NUCLEO-H743ZI board and can be
    easily tailored to any other supported device and development board.

  - This example uses USART3 to display output, the hyperterminal configuration is as follows:

    - BaudRate = 115200 baud
    - Word Length = 8 Bits
    - Stop Bit = 1
    - Parity = None
      Flow control = None

@par How to use it ?

In order to make the program work, you must do the following :
 - Open your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Run the example

*/
