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
  * Copyright (c) 2019 STMicroelectronics.
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

This example demonstrates how to use the NUCLEO-L4R5ZI-P board to blink three LEDs at different speeds when a button is pressed.
The LEDs blink at different speeds, and each press of the user button changes the blinking speed.


On NUCLEO-H723ZG:
    - EXTI15_10 is connected to User push-button

  The SystemClock_Config() function is used to configure the system clock for STM32H723xx Devices :
  The CPU at 520 MHz.
  The HCLK for D1 Domain AXI and AHB3 peripherals , D2 Domain AHB1/AHB2 peripherals and D3 Domain AHB4  peripherals at 520 MHz/2.
  The APB clock dividers for D1 Domain APB3 peripherals, D2 Domain APB1 and APB2 peripherals and D3 Domain APB4 peripherals to run at 520 MHz/4.


@note Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.

@note The example needs to ensure that the SysTick time base is always set to 1 millisecond
to have correct HAL operation.

@par Keywords

GPIO, EXTI, LED, Button, Interrupt

@par Directory contents

  - GPIO/GPIO_Demo/Inc/stm32h7xx_hal_conf.h         HAL configuration file
  - GPIO/GPIO_Demo/Inc/stm32h7xx_it.h               Interrupt handlers header file
  - GPIO/GPIO_Demo/Inc/main.h                       Header for main.c module
  - GPIO/GPIO_Demo/Inc/stm32h7xx_nucleo_conf.h  STM32H7xx_Nucleo board configuration file
  - GPIO/GPIO_Demo/Src/stm32h7xx_it.c               Interrupt handlers
  - GPIO/GPIO_Demo/Src/main.c                       Main program
  - GPIO/GPIO_Demo/Src/system_stm32h7xx.c           STM32H7xx system source file

@par Hardware and Software environment

  - This example runs on STM32H723xx devices.
    
  - This example has been tested with NUCLEO-H723ZG board and can be
    easily tailored to any other supported device and development board.

@par How to use it ? 

In order to make the program work, you must do the following :
 - Open your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Run the example



 */
