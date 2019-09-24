/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define SC_N1_Pin GPIO_PIN_13
#define SC_N1_GPIO_Port GPIOC
#define SC_N2_Pin GPIO_PIN_14
#define SC_N2_GPIO_Port GPIOC
#define SC_N3_Pin GPIO_PIN_0
#define SC_N3_GPIO_Port GPIOC
#define SC_N4_Pin GPIO_PIN_1
#define SC_N4_GPIO_Port GPIOC
#define DD_N4_Pin GPIO_PIN_2
#define DD_N4_GPIO_Port GPIOC
#define DD_N3_Pin GPIO_PIN_3
#define DD_N3_GPIO_Port GPIOC
#define Detect_V_P_Pin GPIO_PIN_0
#define Detect_V_P_GPIO_Port GPIOA
#define DD_N2_Pin GPIO_PIN_2
#define DD_N2_GPIO_Port GPIOA
#define Detect_V_N_Pin GPIO_PIN_3
#define Detect_V_N_GPIO_Port GPIOA
#define DD_N1_Pin GPIO_PIN_4
#define DD_N1_GPIO_Port GPIOA
#define SC_P2_Pin GPIO_PIN_7
#define SC_P2_GPIO_Port GPIOA
#define SC_P3_Pin GPIO_PIN_4
#define SC_P3_GPIO_Port GPIOC
#define SC_P4_Pin GPIO_PIN_5
#define SC_P4_GPIO_Port GPIOC
#define Detect_I_P_Pin GPIO_PIN_1
#define Detect_I_P_GPIO_Port GPIOB
#define Detect_I_P_EXTI_IRQn EXTI1_IRQn
#define Detect_I_N_Pin GPIO_PIN_2
#define Detect_I_N_GPIO_Port GPIOB
#define Detect_I_N_EXTI_IRQn EXTI2_IRQn
#define SC_P1_Pin GPIO_PIN_12
#define SC_P1_GPIO_Port GPIOB
#define DD_P1_Pin GPIO_PIN_14
#define DD_P1_GPIO_Port GPIOB
#define DD_P2_Pin GPIO_PIN_15
#define DD_P2_GPIO_Port GPIOB
#define DD_P3_Pin GPIO_PIN_6
#define DD_P3_GPIO_Port GPIOC
#define DD_P4_Pin GPIO_PIN_7
#define DD_P4_GPIO_Port GPIOC
#define LED_Status_Pin GPIO_PIN_15
#define LED_Status_GPIO_Port GPIOA
#define D1_Pin GPIO_PIN_12
#define D1_GPIO_Port GPIOC
#define D2_Pin GPIO_PIN_2
#define D2_GPIO_Port GPIOD
#define LED_Error_Pin GPIO_PIN_8
#define LED_Error_GPIO_Port GPIOB

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/* USER CODE BEGIN Private defines */


#define set_StateLED_ON()			HAL_GPIO_WritePin(LED_Status_GPIO_Port, LED_Status_Pin, GPIO_PIN_SET)
#define set_StateLED_OFF()			HAL_GPIO_WritePin(LED_Status_GPIO_Port, LED_Status_Pin, GPIO_PIN_RESET)

//#define set_StateLED_ON()			HAL_GPIO_WritePin(LED_Error_GPIO_Port, LED_Error_Pin, GPIO_PIN_SET)
//#define set_StateLED_OFF()			HAL_GPIO_WritePin(LED_Error_GPIO_Port, LED_Error_Pin, GPIO_PIN_RESET)

#define set_ErrorLED_ON()			HAL_GPIO_WritePin(LED_Error_GPIO_Port, LED_Error_Pin, GPIO_PIN_SET)
#define set_ErrorLED_OFF()			HAL_GPIO_WritePin(LED_Error_GPIO_Port, LED_Error_Pin, GPIO_PIN_RESET)

/* USER CODE END Private defines */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
