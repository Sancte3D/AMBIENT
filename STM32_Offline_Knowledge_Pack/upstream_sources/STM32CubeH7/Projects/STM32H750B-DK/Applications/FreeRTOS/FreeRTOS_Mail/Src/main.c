/**
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_Mail/Src/main.c
  * @author  MCD Application Team
  * @brief   Main program body
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
#include "FreeRTOS.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef struct 
{ /* Mail object structure */
  uint32_t var1; /* var1 is a uint32_t */
  uint32_t var2; /* var2 is a uint32_t */
  uint8_t var3; /* var3 is a uint8_t */
} Amail_TypeDef;

/* Private define ------------------------------------------------------------*/
#define blckqSTACK_SIZE  configMINIMAL_STACK_SIZE
#define MAIL_SIZE        (uint32_t) 1

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
osMessageQueueId_t mailId;

osThreadAttr_t consumer_attr = {
    .name = "QCons",
    .priority = osPriorityBelowNormal,
    .stack_size = blckqSTACK_SIZE
    };

    osThreadAttr_t producer_attr = {
        .name = "QProd",
        .priority = osPriorityBelowNormal,
        .stack_size = blckqSTACK_SIZE
    };

uint32_t ProducerValue1 = 0,ProducerValue2 = 0;
uint8_t ProducerValue3 = 0;
uint32_t ConsumerValue1 = 0,ConsumerValue2 = 0;
uint8_t ConsumerValue3 = 0;

/* Private function prototypes -----------------------------------------------*/

/* Thread function that creates a mail and posts it on a mail queue. */
static void MailQueueProducer (void *argument);

/* Thread function that receives mail , remove it  from a mail queue and checks that
it is the expected mail. */
static void MailQueueConsumer(void *argument);

static void SystemClock_Config(void);
static void Error_Handler(void);
static void CPU_CACHE_Enable(void);
/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock to 400 Mhz */
  SystemClock_Config();

  /* Initialize LED_RED LED_GREEN */
  BSP_LED_Init(LED_RED);
  BSP_LED_Init(LED_GREEN);
  /* USER CODE END SysInit */
  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */
  osKernelInitialize();
  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  /* USER CODE END RTOS_SEMAPHORES */
  
  /* USER CODE BEGIN RTOS_TIMERS */

  /* USER CODE END RTOS_TIMERS */

  /* Create the mail queue used by the two tasks to pass the struct Amail_TypeDef */
  mailId = osMessageQueueNew(MAIL_SIZE, sizeof(Amail_TypeDef), NULL);

  if (mailId == NULL)
  {
     Error_Handler();
  }

  /* Create the thread(s) */
  /* Note the producer has a lower priority than the consumer when the tasks are
     spawned. */
  osThreadNew(MailQueueConsumer, NULL, &consumer_attr);

  osThreadNew(MailQueueProducer, NULL, &producer_attr);
  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */

  /* USER CODE END RTOS_QUEUES */
  
  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */

}

/**
  * @brief  Mail Producer Thread.
  * @param  argument: Not used
  * @retval None
  */
static void MailQueueProducer(void *argument)
{
  Amail_TypeDef mail;

  for(;;)
  {

    mail.var1 = ProducerValue1; /* Set the mail content */
    mail.var2 = ProducerValue2;
    mail.var3 = ProducerValue3;

    if(osMessageQueuePut(mailId, &mail, 0, 0) != osOK) /* Send Mail */
    {
      /* LED1 is turned On to indicate error */
      BSP_LED_On(LED_GREEN);
    }
    else
    {
      /* Increment the variables we are going to post next time round. The
      consumer will expect the numbers to follow in numerical order. */
      ++ProducerValue1;
      ProducerValue2 += 2;
      ProducerValue3 += 3;

      /* Toggle LED_GREEN to indicate a correct number sent */
      BSP_LED_Toggle(LED_GREEN);

      osDelay(250);
    }
  }
}

/**
  * @brief  Mail Consumer Thread.
  * @param  argument: Not used
  * @retval None
  */
static void MailQueueConsumer (void *argument)
{

  Amail_TypeDef mail;
  uint8_t msg_prio;

  for(;;)
  {

    if(osMessageQueueGet(mailId, &mail, &msg_prio, osWaitForever) == osOK)
    {

      if((mail.var1 != ConsumerValue1) || (mail.var2 != ConsumerValue2) || (mail.var3 != ConsumerValue3))
      {
        /* Catch-up. */
        ConsumerValue1 = mail.var1;
        ConsumerValue2 = mail.var2;
        ConsumerValue3 = mail.var3;

        /* LED1 is turned On to indicate error */
        BSP_LED_On(LED_RED);
      }
      else
      {
        /* Calculate values we expect to remove from the mail queue next time
        round. */
        ++ConsumerValue1;
        ConsumerValue2 += 2;
        ConsumerValue3 += 3;

        /* Toggle LED_RED to indicate a correct number received */
        BSP_LED_Toggle(LED_RED);
        osDelay(250);
      }
    }
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet. */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
  /* USER CODE END 6 */
}
#endif

