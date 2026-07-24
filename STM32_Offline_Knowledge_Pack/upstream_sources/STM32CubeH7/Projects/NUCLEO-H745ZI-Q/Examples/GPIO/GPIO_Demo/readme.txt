/**
  @page GPIO_Demo GPIO Demo example
  
  @verbatim
  ******************************************************************************
  * @file    GPIO/GPIO_Demo/readme.txt
  * @author  MCD Application Team
  * @brief   Description of the GPIO Demo example.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim

@par Example Description

Demonstration firmware based on STM32Cube. This GPIO_Demo example helps you to
discover STM32 Cortex-M devices that are plugged onto your STM32 Nucleo board.

This example demonstrates how to use the NUCLEO-H745ZI-Q board to blink an LED at different speeds when the user button is pressed.
The LED blinks at three different speeds, and each press of the user button changes the blinking speed.

@note Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.

@Note For the Cortex-M7, if the application is using the DTCM/ITCM memories (@0x20000000/ 0x0000000: not cacheable and only accessible
      by the Cortex-M7 and the  MDMA), no need for cache maintenance when the Cortex M7 and the MDMA access these RAMs.
      If the application needs to use DMA(or other masters) based access or requires more RAM, then  the user has to:
              - Use a non TCM SRAM. (example : D1 AXI-SRAM @ 0x24000000)
              - Add a cache maintenance mechanism to ensure the cache coherence between CPU and other masters(DMAs,DMA2D,LTDC,MDMA).
              - The addresses and the size of cacheable buffers (shared between CPU and other masters)
                must be properly defined to be aligned to L1-CACHE line size (32 bytes). 
 
@Note It is recommended to enable the cache and maintain its coherence.
      Depending on the use case it is also possible to configure the cache attributes using the MPU.
      Please refer to the AN4838 "Managing memory protection unit (MPU) in STM32 MCUs"
      Please refer to the AN4839 "Level 1 cache on STM32F7 Series"

@note The application need to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.

@par Directory contents 

  - GPIO/GPIO_Demo/Common/Src/system_stm32h7xx.c   STM32H7xx system configuration source file
  - GPIO/GPIO_Demo/Common/Inc/Common.h             CM4 - CM7 common items

  - GPIO/GPIO_Demo/CM7/Inc/stm32h7xx_hal_conf.h    HAL configuration file for Cortex-M7
  - GPIO/GPIO_Demo/CM7/Inc/stm32h7xx_it.h          Interrupt handlers header file for Cortex-M7
  - GPIO/GPIO_Demo/CM7/Inc/main.h                  Header for main.c module for Cortex-M7  
  - GPIO/GPIO_Demo/CM7/Src/stm32h7xx_it.c          Interrupt handlers for Cortex-M7
  - GPIO/GPIO_Demo/CM7/Src/main.c                  Main program for Cortex-M7

  - GPIO/GPIO_Demo/CM4/Inc/stm32h7xx_hal_conf.h    HAL configuration file for Cortex-M4
  - GPIO/GPIO_Demo/CM4/Inc/stm32h7xx_it.h          Interrupt handlers header file for Cortex-M4
  - GPIO/GPIO_Demo/CM4/Inc/main.h                  Header for main.c module for Cortex-M4  
  - GPIO/GPIO_Demo/CM4/Src/stm32h7xx_it.c          Interrupt handlers for Cortex-M4
  - GPIO/GPIO_Demo/CM4/Src/main.c                  Main program for Cortex-M4

@par Hardware and Software environment

  - This example runs on STM32H745xx devices.
    
  - This example has been tested with NUCLEO-H745ZI-Q board and can be
    easily tailored to any other supported device and development board.

@par How to use it ? 

In order to make the program work, you must do the following :
 - Open your preferred toolchain
 - For each target configuration (STM32H745ZI_Nucleo_CM7 and STM32H745ZI_Nucleo_CM4) : 
     - Rebuild all files 
     - Load images into target memory
 - After loading the two images, you have to reset the board in order to boot (Cortex-M7) and CPU2 (Cortex-M4) at once.
 - Run the example



 */
