/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32wbxx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

void suspend(int mode);
void sleep(uint32_t time);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BUZZER_Pin GPIO_PIN_0
#define BUZZER_GPIO_Port GPIOA
#define DOOR_Pin GPIO_PIN_2
#define DOOR_GPIO_Port GPIOC
#define DOOR_EXTI_IRQn EXTI2_IRQn
#define TEMP_VAL_Pin GPIO_PIN_0
#define TEMP_VAL_GPIO_Port GPIOC
#define TEMP_VCC_Pin GPIO_PIN_8
#define TEMP_VCC_GPIO_Port GPIOB
#define LIGHT_IRQ_Pin GPIO_PIN_5
#define LIGHT_IRQ_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOB
#define CTRL_Pin GPIO_PIN_1
#define CTRL_GPIO_Port GPIOD
#define CTRL_EXTI_IRQn EXTI1_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
