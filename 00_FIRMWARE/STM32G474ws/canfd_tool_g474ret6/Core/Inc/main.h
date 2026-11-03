/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32g4xx_hal.h"

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
void FDCAN1_bit_rate_config(uint8_t bitRate);
void FDCAN_config_filters(uint32_t F1, uint32_t F2, uint32_t F3, uint32_t F4);

void usb_print(char mytext[]);
void USB_CDC_RxHandler(uint8_t* buf, uint32_t len);

void print_rx_usart();
int compare_rx_usart(const void *a, const void *b);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define NRST_Pin GPIO_PIN_10
#define NRST_GPIO_Port GPIOG
#define ADC_BAT_Pin GPIO_PIN_0
#define ADC_BAT_GPIO_Port GPIOC
#define SD_CS_Pin GPIO_PIN_3
#define SD_CS_GPIO_Port GPIOA
#define SD_DET_Pin GPIO_PIN_4
#define SD_DET_GPIO_Port GPIOA
#define TP_BL_Pin GPIO_PIN_4
#define TP_BL_GPIO_Port GPIOC
#define TFT_CS_Pin GPIO_PIN_5
#define TFT_CS_GPIO_Port GPIOC
#define TFT_RST_Pin GPIO_PIN_0
#define TFT_RST_GPIO_Port GPIOB
#define TFT_DC_Pin GPIO_PIN_1
#define TFT_DC_GPIO_Port GPIOB
#define TP_IRQ_Pin GPIO_PIN_2
#define TP_IRQ_GPIO_Port GPIOB
#define TP_IRQ_EXTI_IRQn EXTI2_IRQn
#define BTN_CFG_Pin GPIO_PIN_11
#define BTN_CFG_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
