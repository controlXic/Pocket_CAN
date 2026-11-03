/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "ili9486.h"
#include "stdio.h"
#include "usbd_cdc_if.h"
#include "st7796s.h"
#include "lvgl.h"
#include "main_system.h"
#include "main_window.h"
#include "can_rx.h"
#include "stm32g4xx_hal_fdcan.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

FDCAN_HandleTypeDef hfdcan1;

I2C_HandleTypeDef hi2c4;
DMA_HandleTypeDef hdma_i2c4_rx;
DMA_HandleTypeDef hdma_i2c4_tx;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */


// Time vars //
extern uint32_t millis;
extern uint32_t rxTimeoutMillis;

extern uint32_t sleepMillis;

// Confgi vars //
extern bool staticMode;
bool msgRxSleepFlg = false;

// CDC vars //
uint32_t cdcRxData[30][80];
uint8_t strCdc[100];

// USART vars //
uint8_t usartLines = 30;
uint8_t rowCnt = 1;
uint32_t rowCntOld = 0;
uint32_t rowCntFull = 1;

// Touch vars //
bool touchFlg = false;

// CAN vars //
FDCAN_TxHeaderTypeDef TxHeader1;
FDCAN_RxHeaderTypeDef RxHeader1;
FDCAN_ProtocolStatusTypeDef fdcanStatus;
uint8_t can1RxData1[8];
uint32_t rxFramesCnt = 0;
bool rxCanFlg = false;
bool rxTimeoutFlg = false;

// Busload time vars //
#define STD_FIXED_BITS 55 // Reals = 47 b/f (dlc = 0)
float oneSeconBitsCan1 = 0;
uint16_t frameBitsCan1 = 0;
uint8_t busLoadBitRate = 20;

// Flow vars //
bool initFlg = true;
extern bool touchSleepFlg;

// LVGL vars //
extern lv_obj_t * tabview;
extern lv_obj_t * uartRxTable;

// test vars //
uint32_t errorCode = 0;
bool errorFlag = false;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_I2C4_Init(void);
/* USER CODE BEGIN PFP */

//extern uint8_t CDC_Transmit_FS(uint8_t*, uint16_t);
//extern int8_t CDC_Receive_FS_Global(uint8_t*, uint32_t);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



//////////////////////////////////////////////////////////////////////////
// CDC write-read
//////////////////////////////////////////////////////////////////////////

void usb_print(char mytext[])
{
	CDC_Transmit_FS((uint8_t*)mytext,strlen(mytext));
	//HAL_Delay(10);
}

void USB_CDC_RxHandler(uint8_t* cdcBuf, uint32_t len)
{

	if(lv_tabview_get_tab_active(tabview) == 2)
	{
		if(cdcBuf[0] != 0)
		{
			if(rowCnt >= usartLines)
				rowCnt = 1;

//			memcpy(cdcRxData[rowCnt], "", 80);
//			memcpy(cdcRxData[rowCnt], buf, 80);

			memset(cdcRxData[rowCnt], '\0', 80);
			strcat(cdcRxData[rowCnt], "RX:  ");
			strcat(cdcRxData[rowCnt], cdcBuf);

			cdcRxData[rowCnt][78] = rowCnt;
			cdcRxData[rowCnt][79] = rowCntFull;

			rowCnt++;
			rowCntFull++;
			msgRxSleepFlg = true;
		}
	}
	else
	{
		// Apply read cmd //
		switch(cdcBuf[0])
		{
			case '0':
				usb_print("\r\ncontrolXic");
			break;

			case '1':

			break;

			case '2':

			break;

			case '3':

			break;

			case '4':

			break;
		}
	}
	memset(cdcBuf, '\0', 80);
}


//////////////////////////////////////////////////////////////////////////
// LvGL LOG
//////////////////////////////////////////////////////////////////////////

void my_log_cb(lv_log_level_t level, const char * buf)
{
	CDC_Transmit_FS(buf, strlen(buf));
	HAL_Delay(10);
}


//////////////////////////////////////////////////////////////////////////
// CAN Read - Write Interrupts
//////////////////////////////////////////////////////////////////////////


void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{

	if(ErrorStatusITs & FDCAN_IT_ERROR_WARNING)
	{
		error_msgbox("FDCAN_IT_ERROR_WARNING");
	}

	if(ErrorStatusITs & FDCAN_IT_ERROR_PASSIVE)
	{
		error_msgbox("FDCAN_IT_ERROR_PASSIVE");
	}

	if(ErrorStatusITs & FDCAN_IT_BUS_OFF)
	{
//		sprintf(strCdc, "\n\r FDCAN_IT_BUS_OFF ERROR: %X ", ErrorStatusITs);
//		usb_print(strCdc);
	}
}


void HAL_FDCAN_TxEventFifoCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t TxEventFifoITs)
{
//	usb_print("\n\rFDCAN_IT_TX_EVT_FIFO_NEW_DATA");

//	if((TxEventFifoITs & FDCAN_IT_TX_EVT_FIFO_NEW_DATA) != RESET)
//	{
//		usb_print("\n\rFDCAN_IT_TX_EVT_FIFO_NEW_DATA");
//		HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_TX_EVT_FIFO_NEW_DATA, 0);
//	}
}

void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef *hfdcan)
{
	usb_print("\n\rFDCAN_IT_TX_FIFO_EMPTY");
}


//////////////////////////////////////////////////////////////////////////
// CAN Read Callback
//////////////////////////////////////////////////////////////////////////

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
	{
	    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader1, can1RxData1) != HAL_OK)
	    {
	    	Error_Handler();
	    }
		if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
	    {
			Error_Handler();
	    }
		// count frames //
		rxFramesCnt++;

        // Busload bit count //
        frameBitsCan1 = STD_FIXED_BITS + (RxHeader1.DataLength * 8);
        oneSeconBitsCan1 = oneSeconBitsCan1 + frameBitsCan1;


		// save on can rx buffer //
		if((initFlg == false) /*&& (lv_tabview_get_tab_active(tabview) == 0)*/)
		{
			rxCanFlg = true;
			msgRxSleepFlg = true;
			rxTimeoutFlg = true;
			rxTimeoutMillis = millis;


			  if(staticMode)
				  save_rx_buf_static(RxHeader1.Identifier, RxHeader1.DataLength, can1RxData1);
			  else
				  save_rx_buf_dinamic(RxHeader1.Identifier, RxHeader1.DataLength, can1RxData1);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// CAN bit rate config (kbit/s)
//////////////////////////////////////////////////////////////////////////

void FDCAN1_bit_rate_config(uint8_t bitRate)
{
	busLoadBitRate = bitRate;

	hfdcan1.Instance = FDCAN1;
	hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
	hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
	hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
	hfdcan1.Init.AutoRetransmission = DISABLE;
	hfdcan1.Init.TransmitPause = DISABLE;
	hfdcan1.Init.ProtocolException = DISABLE;
	hfdcan1.Init.NominalPrescaler = bitRate;
	hfdcan1.Init.NominalSyncJumpWidth = 16;
	hfdcan1.Init.NominalTimeSeg1 = 13;
	hfdcan1.Init.NominalTimeSeg2 = 2;
	hfdcan1.Init.DataPrescaler = 1;
	hfdcan1.Init.DataSyncJumpWidth = 1;
	hfdcan1.Init.DataTimeSeg1 = 1;
	hfdcan1.Init.DataTimeSeg2 = 1;
	hfdcan1.Init.StdFiltersNbr = 2;
	hfdcan1.Init.ExtFiltersNbr = 2;
	hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

	if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
	{
		Error_Handler();
	}
}


//////////////////////////////////////////////////////////////////////////
// CAN Filters config
//////////////////////////////////////////////////////////////////////////

void FDCAN_config_filters(uint32_t F1, uint32_t F2, uint32_t F3, uint32_t F4)
{
	// Apagar CAN bus //
	if (HAL_FDCAN_Stop(&hfdcan1) != HAL_OK)
	{
	  Error_Handler();
	}

	// Si un Filtre es configurat habilita filtres //
	if(F1 > 0x00 || F2 > 0x00 || F3 > 0x00 || F4 > 0x00)
	{
		// Configure Rx filter //
		FDCAN_FilterTypeDef sFilterConfig;


		sFilterConfig.IdType = FDCAN_STANDARD_ID;
		sFilterConfig.FilterIndex = 0;
		sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
		sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

		if(F1 <= 0x7FF)
			sFilterConfig.FilterID1 = F1;
		else
			sFilterConfig.FilterID1 = 0x00;

		if(F2 <= 0x7FF)
			sFilterConfig.FilterID2 = F2;
		else
			sFilterConfig.FilterID2 = 0x00;

		if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
		{
			  Error_Handler();
		}

		sFilterConfig.IdType = FDCAN_STANDARD_ID;
		sFilterConfig.FilterIndex = 1;
		sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
		sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

		if(F3 <= 0x7FF)
			sFilterConfig.FilterID1 = F3;
		else
			sFilterConfig.FilterID1 = 0x00;

		if(F4 <= 0x7FF)
			sFilterConfig.FilterID2 = F4;
		else
			sFilterConfig.FilterID2 = 0x00;

		if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
		{
			  Error_Handler();
		}

		sFilterConfig.IdType = FDCAN_EXTENDED_ID;
		sFilterConfig.FilterIndex = 0;
		sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
		sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

		if(F1 > 0x7FF)
			sFilterConfig.FilterID1 = F1;
		else
			sFilterConfig.FilterID1 = 0x00;

		if(F2 > 0x7FF)
			sFilterConfig.FilterID2 = F2;
		else
			sFilterConfig.FilterID2 = 0x00;

		if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
		{
			  Error_Handler();
		}

		sFilterConfig.IdType = FDCAN_EXTENDED_ID;
		sFilterConfig.FilterIndex = 1;
		sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
		sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

		if(F3 > 0x7FF)
			sFilterConfig.FilterID1 = F3;
		else
			sFilterConfig.FilterID1 = 0x00;

		if(F4 > 0x7FF)
			sFilterConfig.FilterID2 = F4;
		else
			sFilterConfig.FilterID2 = 0x00;

		if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
		{
			  Error_Handler();
		}


		if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK)
		{
			  Error_Handler();
		}
	}

	// Deshabilita  filtres //
	else
	{

//		usb_print( "Deshabilita  filtres");

		FDCAN_FilterTypeDef sFilterConfig;

		for(int i = 0; i < 2; i++)
		{
			sFilterConfig.IdType = FDCAN_STANDARD_ID;
			sFilterConfig.FilterIndex = i;
			sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
			sFilterConfig.FilterConfig = FDCAN_FILTER_DISABLE;
			sFilterConfig.FilterID1 = 0x00;
			sFilterConfig.FilterID2 = 0x00;

			if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
			{
				  Error_Handler();
			}

			sFilterConfig.IdType = FDCAN_EXTENDED_ID;
			sFilterConfig.FilterIndex = i;
			sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
			sFilterConfig.FilterConfig = FDCAN_FILTER_DISABLE;
			sFilterConfig.FilterID1 = 0x00;
			sFilterConfig.FilterID2 = 0x00;

			if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
			{
				  Error_Handler();
			}
		}
		if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0) != HAL_OK)
		{
			  Error_Handler();
		}
	}

	// Inicia CAN bus //
	if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
	{
		Error_Handler();
	}
}


//////////////////////////////////////////////////////////////////////////
// SPI TX Interrupt
//////////////////////////////////////////////////////////////////////////

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
}


//////////////////////////////////////////////////////////////////////////
// GPIO External Interrupt for touchscreen
//////////////////////////////////////////////////////////////////////////

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == TP_IRQ_Pin )
	{
		// Touch external interrupt //
		touchFlg = true;
	}
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USB_Device_Init();
  MX_ADC1_Init();
  MX_FDCAN1_Init();
  MX_I2C4_Init();
  /* USER CODE BEGIN 2 */

//  HAL_ADCEx_Calibration_Start(&hadc1);

  // lvgl init //
  lv_init();

  // Display init //
  tft_init();
  tft_setRotation(1);
  tp_init();

  // show init logo //
  init_logo();

//  // log lvgl debug //
//  lv_log_register_print_cb(my_log_cb);

  usb_print("\r\n ******************** Control Xic ******************** \n\r");
//  sprintf(strCdc, "\n\r %d - %d - %d", millis, tp_x, tp_y);
//  usb_print(strCdc);
//	HAL_Delay(500);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while(1)
  {
	  //////////////////////////////////////////////////////////////////////////
	  // LVG internal tasks //
	  //////////////////////////////////////////////////////////////////////////
	  lv_timer_handler();


	  //////////////////////////////////////////////////////////////////////////
	  // Work Flow, Display print, timeouts, ... //
	  //////////////////////////////////////////////////////////////////////////
	  main_task();




    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV8;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 20;
  hfdcan1.Init.NominalSyncJumpWidth = 16;
  hfdcan1.Init.NominalTimeSeg1 = 13;
  hfdcan1.Init.NominalTimeSeg2 = 2;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.StdFiltersNbr = 2;
  hfdcan1.Init.ExtFiltersNbr = 2;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */


  // START FDCAN bus //
  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
  {
	  Error_Handler();
  }
  if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
  {
	  Error_Handler();
  }
  if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_TX_EVT_FIFO_NEW_DATA /*| FDCAN_IT_TX_FIFO_EMPTY*/, 0) != HAL_OK)
  {
	  Error_Handler();
  }
  if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_ERROR_WARNING | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_BUS_OFF, 0) != HAL_OK)
  {
	  Error_Handler();
  }

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief I2C4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C4_Init(void)
{

  /* USER CODE BEGIN I2C4_Init 0 */

  /* USER CODE END I2C4_Init 0 */

  /* USER CODE BEGIN I2C4_Init 1 */

  /* USER CODE END I2C4_Init 1 */
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x30D29DE4;
  hi2c4.Init.OwnAddress1 = 0;
  hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c4.Init.OwnAddress2 = 0;
  hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C4_Init 2 */

  /* USER CODE END I2C4_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SD_CS_Pin|SD_DET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, TP_BL_Pin|TFT_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TFT_RST_Pin|TFT_DC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : NRST_Pin */
  GPIO_InitStruct.Pin = NRST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF15_EVENTOUT;
  HAL_GPIO_Init(NRST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CS_Pin SD_DET_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin|SD_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : TP_BL_Pin */
  GPIO_InitStruct.Pin = TP_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TP_BL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_CS_Pin */
  GPIO_InitStruct.Pin = TFT_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(TFT_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_RST_Pin */
  GPIO_InitStruct.Pin = TFT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TFT_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_DC_Pin */
  GPIO_InitStruct.Pin = TFT_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(TFT_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TP_IRQ_Pin */
  GPIO_InitStruct.Pin = TP_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TP_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_CFG_Pin */
  GPIO_InitStruct.Pin = BTN_CFG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_CFG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF8_I2C3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB5 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
