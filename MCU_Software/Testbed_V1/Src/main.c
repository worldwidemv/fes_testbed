
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include "GSBP_Basic_Config.h"
#include "GSBP_Basic.h"
#include "MCU_StateMachine.h"

#include "Testbed.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
uint8_t DoMeasurments;
uint16_t MeasTestDelay;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_SPI1_Init();
  MX_SPI4_Init();
  MX_USART3_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM8_Init();
  /* USER CODE BEGIN 2 */


  /*
   * ########################################################################################
   * MCU Initialization
   * ########################################################################################
   */
  MCU_StateChange(MCU_PreMCU_Init);
  MCU_State_LowFrequncyTimerCounter = HAL_GetTick() + MCU_STATEMACHINE__LOWFREQUENCY_TIMER_PERIOD_IN_MS;

  /* Initialize variables */
  // GPIOs

  // GSBP
  uint32_t GSBP_NextCall = HAL_GetTick() + GSBP_SETUP__CALLBACK_PERIOD_IN_MS;

  // TestBed
//  memset(&ISS_SensorGroups, 0, sizeof(ISS_SensorGroups_t));

  tca_device_t stimN1, stimP1, stimP2; // TODO: in init function integrieren für variables Setup ....
  tca9535_init_device(&stimN1, &hi2c2, 1, 0xFFFF, 0);
  tca9535_init_device(&stimP1, &hi2c1, 1, 0xFFFF, 0);
  tca9535_init_device(&stimP2, &hi2c3, 1, 0xFFFF, 0);

  /* wait a bit for the external parts to initialize */
  HAL_Delay(200);

  /* Initialize the GSBP communication system */
  GSBP_Init();

  /* send an DEBUG message about the system reset ... */
  sprintf((char*)GSBP_Handles.ACK.Data, "TestBed reseted -> timer = %lu", HAL_GetTick());
  GSBP_SendDebugPackage(&GSBP_USB, GSBP_Handles.ACK.Data);


  /* Start Normal Execution */
  MCU_StateChange(MCU_PreInit_Connected);

  /*
   * ########################################################################################
   * MCU infinite loop -> normal operations
   * ########################################################################################
   */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  	  /*
	   * ########################################################################################
	   *  run the low frequency callback
	   * ########################################################################################
	   */
	  if (HAL_GetTick() >= MCU_State_LowFrequncyTimerCounter) {
		  MCU_State_LowFrequncyTimerCounter += MCU_STATEMACHINE__LOWFREQUENCY_TIMER_PERIOD_IN_MS;
		  MCU_LowFrequencyTimerCallback();
	  }

  	  /*
	   * ########################################################################################
	   *  check for new packages every GSBP_CONFIG__CALLBACK_PERIOD_IN_MS ms
	   * ########################################################################################
	   */
	  if (HAL_GetTick() >= GSBP_NextCall) {
		  GSBP_NextCall += GSBP_SETUP__CALLBACK_PERIOD_IN_MS;
		  if (GSBP_EvaluatePackage(&GSBP_USB)){
			  // run the evaluate package again, after the current package is processed
			  GSBP_NextCall = HAL_GetTick();
		  }
	  }

	  /*
	   * ########################################################################################
	   *  execute the global state machine
	   * ########################################################################################
	   */
	  switch (MCU_State_CurrentState){
	  case MCU_PreInit_NotConnected:
		  // connection is via USB
	  	  MCU_StateChange(MCU_PreInit_Connected);
		  break;

	  case MCU_PreInit_Connected:
		  // wait for the INIT cmd
		  break;

	  case MCU_DoInit:{
		  // initialise the system
		  // get the init settings struct

		  // initialize the TestBed
		  testbed_init( (testbed_initCmd_t*)GSBP_Handles.CMD.Data, (testbed_initAck_t*)GSBP_Handles.ACK.Data);

		  // send Ack
		  GSBP_Handles.ACK.CommandID = InitACK;
		  GSBP_Handles.ACK.DataSize  = sizeof(testbed_initAck_t);
		  GSBP_SendPackage(GSBP_Handles.DefaultHandle, &(GSBP_Handles.ACK));

		  // done -> goto PostInit
		  HAL_Delay(3);
		  MCU_StateChange(MCU_PostInit);
		  GSBP_NextCall = HAL_GetTick() + GSBP_SETUP__CALLBACK_PERIOD_MIN_IN_MS;
	  	  break; }

	  case MCU_PostInit:
		  // post initialization -> wait for the start measurement command
		  break;

	  case MCU_MeasurmentDoStart:{
		  // start the measurement
		   max1119x_startMeasurment(&testbed.maxAdcDevice);

		   testbed.keepAlive_packageCounter = HAL_GetTick() + TESTBED_ADC_NO_STIM_WAIT_TIME_MS;

		  // send Ack
		  GSBP_Handles.ACK.CommandID = StartMeasurmentACK;
		  GSBP_Handles.ACK.DataSize = 0;
		  GSBP_SendPackage(GSBP_Handles.DefaultHandle, &(GSBP_Handles.ACK));

		  // done
		  MCU_StateChange(MCU_MeasurmentActiv);
		  break;}

	  case MCU_MeasurmentActiv:
		  // get the data from all the discovered sensors and send it away

		  if (testbed.updateStimChannels){
			  testbed_saveChannelInfoStim_PositivAndNegative();
			  testbed.updateStimChannels = false;
		  }

		  if (testbed_sendAdcData_realData( &testbed.maxAdcDevice )) {
			  testbed.keepAlive_packageCounter = HAL_GetTick() + TESTBED_ADC_NO_STIM_WAIT_TIME_MS;
		  } else {
			  // send the dummy packet, to let the PC know, that we are still there
			  if (HAL_GetTick() >= testbed.keepAlive_packageCounter) {
				  HAL_GPIO_TogglePin(D1_GPIO_Port, D1_Pin);
				  if (testbed_sendAdcData_keepAlive( &testbed.maxAdcDevice )){
					  // increase the counter, if the dummy package was send successfully
					  testbed.keepAlive_packageCounter = HAL_GetTick() + TESTBED_ADC_NO_STIM_WAIT_TIME_MS;
				  }
			  }
		  }

		  // TODO

		  break;

	  case MCU_MeasurmentDoStop:
		  // stop sending normal measurement data
		  max1119x_stopMeasurment(&testbed.maxAdcDevice);

		  // send Ack
		  GSBP_Handles.ACK.CommandID = StopMeasurmentACK;
		  GSBP_Handles.ACK.DataSize = 0;
		  GSBP_SendPackage(GSBP_Handles.DefaultHandle, &(GSBP_Handles.ACK));
		  // done
		  MCU_StateChange(MCU_MeasurmentFinished);
		  break;

	  case MCU_MeasurmentFinished:
		  // normal measurement has stopped

		  // TODO

		  break;

	  case MCU_DoDeInit:
		  // DEinitialize the system

		  testbed_deInit();

		  // send Ack
		  GSBP_Handles.ACK.CommandID = DeInitACK;
		  GSBP_Handles.ACK.DataSize = 0;
		  GSBP_SendPackage(GSBP_Handles.DefaultHandle, &(GSBP_Handles.ACK));
		  // done

		  MCU_StateChange(MCU_PreInit_Connected);
		  break;
	  }


	  /*
	   * ########################################################################################
	   *  resent package if needed
	   * ########################################################################################
	   */
	  GSBP_ReSendPackages();




	  /*
	  		if (tca9535_read_inputs(&stimP, &input)){
	  			if (input != old_input){
	  				old_input = input;
	  				GSBP_SendDebugPackage(&GSBP_USB, "change detected\n");
	  			}
	  		} */
	  //HAL_Delay(10);
//	  HAL_GPIO_TogglePin(STATUS2_GPIO_Port, STATUS2_Pin);


  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 72;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 4;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  PeriphClkInitStruct.PLLI2S.PLLI2SQ = 3;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLI2SQ;
  PeriphClkInitStruct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 1, 1);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
