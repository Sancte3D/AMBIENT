/**
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_HwSemaphoreCoreSync/CM4/Src/main.c
  * @author  MCD Application Team
  *          This is the main program for Cortex-M4 
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
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os2.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Definitions for osSemaphore */
osSemaphoreId_t osSemaphoreHandle;
const osSemaphoreAttr_t osSemaphore_attributes = {
  .name = "osSemaphore"
};

osThreadId_t SEM_ThreadHandle;
const osThreadAttr_t SEM_Thread_attributes = {
  .name = "SEM_Thread",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static void CORE2_SemaphoreCoreSync(void *argument);
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{

  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();

  /* Activate HSEM notification for Cortex-M4*/
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

  /*
  Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
  perform system initialization (system clock config, external memory configuration.. )
  */
  HAL_PWREx_ClearPendingEvent();
  HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);

  /* Clear HSEM flag */
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

  /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
  */
  HAL_Init();
  /* Add Cortex-M4 user application code here */
  /* Configure LED3 */
  BSP_LED_Init(LED3);

  /* Enable HSEM Interrupt */
  HAL_NVIC_SetPriority(HSEM2_IRQn,10, 0);
  HAL_NVIC_EnableIRQ(HSEM2_IRQn);

  /* Init scheduler */
  osKernelInitialize();

  /* Create the semaphores(s) */
  /* creation of osSemaphore */
  osSemaphoreHandle = osSemaphoreNew(1, 1, &osSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  if(osSemaphoreHandle == NULL)
  {
    Error_Handler();
  }

  /* Create the thread(s) */
  /* creation of SEM_Thread */
  SEM_ThreadHandle = osThreadNew(CORE2_SemaphoreCoreSync, NULL, &SEM_Thread_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  if(SEM_ThreadHandle == NULL)
  {
    Error_Handler();
  }
  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  for (;;);

}

/**
  * @brief  Semaphore Test.
  * @param  argument: Not used
  * @retval None
  */
static void CORE2_SemaphoreCoreSync(void *argument)
{
  for(;;)
  {
    if (osSemaphoreHandle != NULL)
    {
      /* HSEM Activate Notification*/
      HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));
      /* Try to obtain the semaphore */
      if(osSemaphoreAcquire(osSemaphoreHandle , 0) == osOK)
      {
        BSP_LED_Toggle(LED3);
      }
    }
  }
}

/**
  * @brief Semaphore Released Callback.
  * @param SemMask: Mask of Released semaphores
  * @retval None
  */
void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
  if (osSemaphoreHandle != NULL)
  {
    osSemaphoreRelease (osSemaphoreHandle);
  }

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  /* Infinite loop */
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
