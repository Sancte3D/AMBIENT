/*
 * FreeRTOS Kernel V10.6.2
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Portions Copyright (C) 2019 StMicroelectronics, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * Adjust these definitions for your particular hardware and application requirements.
 * See http://www.freertos.org/a00110.html for detailed documentation.
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, not assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
 #include <stdint.h>
 extern uint32_t SystemCoreClock;
#endif

/* CMSIS device header for CMSIS V2 */
#ifdef USE_CMSIS_V2
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32h7xx.h"
#endif /* CMSIS_device_header */
#endif /* USE_CMSIS_V2 */

/* Common configuration */
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#if defined(__GNUC__)
 #define configTOTAL_HEAP_SIZE                  ( ( size_t ) ( 50 * 1024 ) )
#else
 #define configTOTAL_HEAP_SIZE                  ( ( size_t ) ( 40 * 1024 ) )
#endif
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_TRACE_FACILITY                1
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1
#define configGENERATE_RUN_TIME_STATS           0

/* Priority and optimization settings */
#ifdef USE_CMSIS_V2
  /*
   * CMSISRTOS-v2 requires:
   * - FreeRTOS static allocation features (configSUPPORT_STATIC_ALLOCATION == 1)
   * - Max priorities set to 56
   * - Disable optimized task selection
   */
  #define configSUPPORT_STATIC_ALLOCATION         1
  #define configMAX_PRIORITIES                    56
  #define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#else
  #define configMAX_PRIORITIES                    7
#endif

/* Co-routine definitions */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         5

/* Software timer definitions */
#ifdef USE_CMSIS_V2
  #define configTIMER_TASK_PRIORITY               48 /* osPriorityRealtime */
#else
  #define configTIMER_TASK_PRIORITY               3  /* osPriorityRealtime */
#endif
#define configUSE_TIMERS                        1
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* API function inclusion */
#ifdef USE_CMSIS_V2
  #define INCLUDE_vTaskPrioritySet                1
  #define INCLUDE_uxTaskPriorityGet               1
  #define INCLUDE_vTaskDelete                     1
  #define INCLUDE_vTaskCleanUpResources           1
  #define INCLUDE_vTaskSuspend                    1
  #define INCLUDE_vTaskDelayUntil                 1
  #define INCLUDE_vTaskDelay                      1
  #define INCLUDE_xQueueGetMutexHolder            1
  #define INCLUDE_xTaskGetSchedulerState          1
  #define INCLUDE_eTaskGetState                   1
  #define INCLUDE_xTimerPendFunctionCall          1
  #define INCLUDE_uxTaskGetStackHighWaterMark     1
  #define INCLUDE_xTaskGetCurrentTaskHandle       1
#else
  #define INCLUDE_vTaskPrioritySet                1
  #define INCLUDE_uxTaskPriorityGet               1
  #define INCLUDE_vTaskDelete                     1
  #define INCLUDE_vTaskCleanUpResources           0
  #define INCLUDE_vTaskSuspend                    0
  #define INCLUDE_vTaskDelayUntil                 0
  #define INCLUDE_vTaskDelay                      1
  #define INCLUDE_xTaskGetSchedulerState          1
#endif

/* Cortex-M specific definitions */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS                        __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS                        4  /* 15 priority levels */
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY          0xf
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5

#define configKERNEL_INTERRUPT_PRIORITY                   ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY              ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Assert macro */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Map FreeRTOS port interrupt handlers to CMSIS standard names */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

#ifndef USE_CMSIS_V2
  #define xPortSysTickHandler SysTick_Handler
#endif /* USE_CMSIS_V2 */

#endif /* FREERTOS_CONFIG_H */
