/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OUT22_Pin GPIO_PIN_13
#define OUT22_GPIO_Port GPIOC
#define OUT23_Pin GPIO_PIN_14
#define OUT23_GPIO_Port GPIOC
#define OUT0_Pin GPIO_PIN_0
#define OUT0_GPIO_Port GPIOA
#define OUT1_Pin GPIO_PIN_1
#define OUT1_GPIO_Port GPIOA
#define OUT2_Pin GPIO_PIN_2
#define OUT2_GPIO_Port GPIOA
#define OUT3_Pin GPIO_PIN_3
#define OUT3_GPIO_Port GPIOA
#define OUT4_Pin GPIO_PIN_4
#define OUT4_GPIO_Port GPIOA
#define OUT5_Pin GPIO_PIN_5
#define OUT5_GPIO_Port GPIOA
#define OUT6_Pin GPIO_PIN_6
#define OUT6_GPIO_Port GPIOA
#define OUT7_Pin GPIO_PIN_7
#define OUT7_GPIO_Port GPIOA
#define OUT8_Pin GPIO_PIN_0
#define OUT8_GPIO_Port GPIOB
#define OUT9_Pin GPIO_PIN_1
#define OUT9_GPIO_Port GPIOB
#define OUT10_Pin GPIO_PIN_2
#define OUT10_GPIO_Port GPIOB
#define OUT18_Pin GPIO_PIN_10
#define OUT18_GPIO_Port GPIOB
#define OUT19_Pin GPIO_PIN_11
#define OUT19_GPIO_Port GPIOB
#define DIB_CSA_Pin GPIO_PIN_12
#define DIB_CSA_GPIO_Port GPIOB
#define DIB_SCLK_Pin GPIO_PIN_13
#define DIB_SCLK_GPIO_Port GPIOB
#define DIB_MISO_Pin GPIO_PIN_14
#define DIB_MISO_GPIO_Port GPIOB
#define DIB_MOSI_Pin GPIO_PIN_15
#define DIB_MOSI_GPIO_Port GPIOB
#define PWM1_Pin GPIO_PIN_8
#define PWM1_GPIO_Port GPIOA
#define PWM2_Pin GPIO_PIN_11
#define PWM2_GPIO_Port GPIOA
#define DIB_IRQ_Pin GPIO_PIN_12
#define DIB_IRQ_GPIO_Port GPIOA
#define OUT20_Pin GPIO_PIN_6
#define OUT20_GPIO_Port GPIOF
#define OUT21_Pin GPIO_PIN_7
#define OUT21_GPIO_Port GPIOF
#define K_PWR_Pin GPIO_PIN_15
#define K_PWR_GPIO_Port GPIOA
#define OUT11_Pin GPIO_PIN_3
#define OUT11_GPIO_Port GPIOB
#define OUT12_Pin GPIO_PIN_4
#define OUT12_GPIO_Port GPIOB
#define OUT13_Pin GPIO_PIN_5
#define OUT13_GPIO_Port GPIOB
#define OUT14_Pin GPIO_PIN_6
#define OUT14_GPIO_Port GPIOB
#define OUT15_Pin GPIO_PIN_7
#define OUT15_GPIO_Port GPIOB
#define OUT16_Pin GPIO_PIN_8
#define OUT16_GPIO_Port GPIOB
#define OUT17_Pin GPIO_PIN_9
#define OUT17_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
