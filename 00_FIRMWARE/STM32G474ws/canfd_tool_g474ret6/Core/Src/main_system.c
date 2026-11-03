/*
 * main_system.c
 *
 *  Created on: Jul 4, 2025
 *      Author: Pep
 */

#include "main_system.h"
#include "main.h"
#include "lvgl.h"
#include "main_window.h"
#include "can_rx.h"



// Peripheric vars //
extern ADC_HandleTypeDef hadc1;

// Time vars //
extern uint32_t millis;

uint32_t initLogoMillis = 0;
uint16_t initDisplay = 1000;
uint16_t initLogoDelay = 2500;

uint32_t rxPrintMillis = 0;
uint16_t rxPrintPeriode = 150;

uint32_t rxTimeoutMillis = 0;
uint16_t rxTimeoutPeriode = 300;

uint32_t canStatusMillis = 0;
uint16_t canStatusPeriode = 1000;

uint32_t scrolltMillis = 0;
uint16_t scrollPeriode = 100;

uint32_t sleepMillis = 0;
uint16_t sleepPeriode = 30000;

// LVGL vars //
extern lv_obj_t * tabview;
extern lv_obj_t * canRxTable;
extern lv_obj_t * uartRxTable;

extern lv_obj_t * labelBlRw;
extern lv_obj_t * labelRxRw;
extern lv_obj_t * labelTxRw;

extern lv_obj_t * labelBlTw;
extern lv_obj_t * labelRxTw;
extern lv_obj_t * labelTxTw;

extern lv_obj_t * labelBlCw;
extern lv_obj_t * labelRxCw;
extern lv_obj_t * labelTxCw;

extern lv_obj_t * labelBattStatRx;
extern lv_obj_t * labelBattStatTx;
extern lv_obj_t * labelBattStatUs;
extern lv_obj_t * labelBattStatCf;
extern lv_obj_t * bitRateMenu;

// CAN vars //
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_TxHeaderTypeDef TxHeader1;
extern FDCAN_RxHeaderTypeDef RxHeader1;
extern uint8_t can1RxData1[8];
extern bool rxCanFlg;
extern bool rxTimeoutFlg;
extern bool printFullRxBufFlg;

// Touch vars //
extern bool touchFlg;

// Flow vars //
extern bool initFlg;
bool tableToucgFlg = true;
bool uartToucgFlg =  true;

// Confgi vars //
extern bool staticMode;
extern uint8_t busLoadCnt;
extern uint32_t rxFramesCnt;
extern uint32_t txFramesCnt;
extern uint8_t strCanStatus[10];
extern lv_obj_t * labelRxCnt;
extern lv_obj_t * labelTxCnt;
bool sleepFlg = false;
bool touchSleepFlg = false;
extern bool msgRxSleepFlg;

// Battery vars //
uint16_t battRes = 0;




//////////////////////////////////////////////////////////////////////////
// Work Flow, Display print, timeouts, ...
//////////////////////////////////////////////////////////////////////////
void main_task()
{

	//////////////////////////////////////////////////////////////////////////
	// Init main window after view 2 seconds of logo cXic
	//////////////////////////////////////////////////////////////////////////
	if(initFlg == true)
	{
		if(millis >= initLogoMillis + initDisplay)
			HAL_GPIO_WritePin(TP_BL_GPIO_Port, TP_BL_Pin, SET);

		// init mainwindow //
		if(millis >= initLogoMillis + initLogoDelay)
		{
			initFlg = false;
			init_window();
			printFullRxBufFlg = true;

			// Activa auto scroll a l'inici //
			tableToucgFlg = true;

			// Inicia propetats de menu config //
			lv_dropdown_set_selected(bitRateMenu, 1);
			lv_obj_add_state(bitRateMenu, LV_STATE_DISABLED);
		}
	}


	else
	{
		//////////////////////////////////////////////////////////////////////////
		// Update CAN RX screen
		//////////////////////////////////////////////////////////////////////////
		if(lv_tabview_get_tab_active(tabview) == 0)
		{
			// update rx can screen periodic //
			if((millis >= rxPrintMillis + rxPrintPeriode) && (rxCanFlg == true) && (touchFlg == false))
			{
				rxCanFlg = false;
				rxPrintMillis = millis;

				if(staticMode)
					update_rx_screen_static();
				else
					update_rx_screen_dinamic();
			}

			// CAN RX timeout; print buffer if not read periode //
			if((millis >= rxTimeoutMillis + rxTimeoutPeriode) && (rxTimeoutFlg == true) && (touchFlg == false))
			{
				rxTimeoutFlg = false;
				printFullRxBufFlg = true;

				if(staticMode)
					update_rx_screen_static();
				else
				{
					// Comprova si hi ha frames sense assignar, assigna i pinta //
					if(HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0)
					{
						uint32_t i = HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0);

						for(int j = 0; j < i;j++)
						{
							HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &RxHeader1, can1RxData1);
							HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

							save_rx_buf_dinamic(RxHeader1.Identifier, RxHeader1.DataLength, can1RxData1);
						}
					}
						update_rx_screen_dinamic();
				}
			}
		}


		//////////////////////////////////////////////////////////////////////////
		// Auto Scroll timeout
		//////////////////////////////////////////////////////////////////////////
		if(millis >= scrolltMillis + scrollPeriode)
		{
			  scrolltMillis = millis;

			  // CAN RX Scroll auto mentre no es toqui canRxTable //
			  if((lv_tabview_get_tab_active(tabview) == 0) && (tableToucgFlg == true) && lv_obj_get_scroll_bottom(canRxTable) != 0)
			  {
				  lv_obj_scroll_to_y(canRxTable, (lv_obj_get_scroll_bottom(canRxTable) + lv_obj_get_scroll_top(canRxTable)), LV_ANIM_ON);
			  }

			  // UART Screen refresh //
			  if(lv_tabview_get_tab_active(tabview) == 2)
			  {
				  print_rx_usart();

				  if(uartToucgFlg == true && lv_obj_get_scroll_top(uartRxTable) != 0)
				  {
					  uartToucgFlg = false;
					  lv_obj_scroll_to_y(uartRxTable, 0, LV_ANIM_ON);
				  }
			  }
			  if(lv_tabview_get_tab_active(tabview) != 2)
				  uartToucgFlg = true;
		  }


		//////////////////////////////////////////////////////////////////////////
		// Sleep mode timeout
		//////////////////////////////////////////////////////////////////////////
		if((millis >= sleepMillis + sleepPeriode) && (sleepFlg == true) && (touchSleepFlg == false))
		{
			HAL_GPIO_WritePin(TP_BL_GPIO_Port, TP_BL_Pin, RESET);

//			HAL_SuspendTick();
//			HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
//			HAL_PWR_EnableSleepOnExit();
		}
		if((touchSleepFlg == true) || (msgRxSleepFlg == true))
		{
			HAL_GPIO_WritePin(TP_BL_GPIO_Port, TP_BL_Pin, SET);
			sleepMillis = millis;
			touchSleepFlg = false;
			msgRxSleepFlg = false;
		}


		//////////////////////////////////////////////////////////////////////////
		// Battery & CAN Status on Tabs
		//////////////////////////////////////////////////////////////////////////
		if((millis >= canStatusMillis + canStatusPeriode))
		{
			canStatusMillis = millis;

			uint16_t adcRes = 0;
			HAL_ADC_Start(&hadc1);
			HAL_ADC_PollForConversion(&hadc1, 1);
			adcRes = HAL_ADC_GetValue(&hadc1);
			battRes = adcRes / 0.737;

			// RX screen update //
			if(lv_tabview_get_tab_active(tabview) == 0)
			{
				// Battery Status //
				if(battRes < 4200 && battRes >= 3900)
					lv_label_set_text(labelBattStatRx, LV_SYMBOL_BATTERY_FULL);
				else if(battRes < 3900 && battRes >= 3600)
					lv_label_set_text(labelBattStatRx, LV_SYMBOL_BATTERY_3);
				else if(battRes < 3600 && battRes >= 3300)
					lv_label_set_text(labelBattStatRx, LV_SYMBOL_BATTERY_2);
				else if(battRes < 3300 && battRes >= 2800)
					lv_label_set_text(labelBattStatRx, LV_SYMBOL_BATTERY_1);
				else if(battRes < 2800 && battRes >= 2600)
					lv_label_set_text(labelBattStatRx, LV_SYMBOL_BATTERY_EMPTY);

				// CAN Bus Status //
				sprintf(strCanStatus, "RX: %d", rxFramesCnt);
				lv_label_set_text(labelRxRw, strCanStatus);

				sprintf(strCanStatus, "TX: %d", txFramesCnt);
				lv_label_set_text(labelTxRw, strCanStatus);
			}
			// TX screen update //
			if(lv_tabview_get_tab_active(tabview) == 1)
			{
				// Battery Status //
				if(battRes < 4200 && battRes >= 3900)
					lv_label_set_text(labelBattStatTx, LV_SYMBOL_BATTERY_FULL);
				else if(battRes < 3900 && battRes >= 3600)
					lv_label_set_text(labelBattStatTx, LV_SYMBOL_BATTERY_3);
				else if(battRes < 3600 && battRes >= 3300)
					lv_label_set_text(labelBattStatTx, LV_SYMBOL_BATTERY_2);
				else if(battRes < 3300 && battRes >= 2800)
					lv_label_set_text(labelBattStatTx, LV_SYMBOL_BATTERY_1);
				else if(battRes < 2800 && battRes >= 2600)
					lv_label_set_text(labelBattStatTx, LV_SYMBOL_BATTERY_EMPTY);

				// CAN Bus Status //
				sprintf(strCanStatus, "RX: %d", rxFramesCnt);
				lv_label_set_text(labelRxTw, strCanStatus);

				sprintf(strCanStatus, "TX: %d", txFramesCnt);
				lv_label_set_text(labelTxTw, strCanStatus);
			}
			// USART screen update //
			if(lv_tabview_get_tab_active(tabview) == 2)
			{
				// Battery Status //
				if(battRes < 4200 && battRes >= 3900)
					lv_label_set_text(labelBattStatUs, LV_SYMBOL_BATTERY_FULL);
				else if(battRes < 3900 && battRes >= 3600)
					lv_label_set_text(labelBattStatUs, LV_SYMBOL_BATTERY_3);
				else if(battRes < 3600 && battRes >= 3300)
					lv_label_set_text(labelBattStatUs, LV_SYMBOL_BATTERY_2);
				else if(battRes < 3300 && battRes >= 2800)
					lv_label_set_text(labelBattStatUs, LV_SYMBOL_BATTERY_1);
				else if(battRes < 2800 && battRes >= 2600)
					lv_label_set_text(labelBattStatUs, LV_SYMBOL_BATTERY_EMPTY);
			}
			// Config screen update //
			if(lv_tabview_get_tab_active(tabview) == 3)
			{
				// Battery Status //
				if(battRes < 4200 && battRes >= 3900)
					lv_label_set_text(labelBattStatCf, LV_SYMBOL_BATTERY_FULL);
				else if(battRes < 3900 && battRes >= 3600)
					lv_label_set_text(labelBattStatCf, LV_SYMBOL_BATTERY_3);
				else if(battRes < 3600 && battRes >= 3300)
					lv_label_set_text(labelBattStatCf, LV_SYMBOL_BATTERY_2);
				else if(battRes < 3300 && battRes >= 2800)
					lv_label_set_text(labelBattStatCf, LV_SYMBOL_BATTERY_1);
				else if(battRes < 2800 && battRes >= 2600)
					lv_label_set_text(labelBattStatCf, LV_SYMBOL_BATTERY_EMPTY);

				// CAN Bus Status //
				can_bus_load();

				sprintf(strCanStatus, "BL: %d %%", busLoadCnt);
				lv_label_set_text(labelBlCw, strCanStatus);

				sprintf(strCanStatus, "RX: %d", rxFramesCnt);
				lv_label_set_text(labelRxCw, strCanStatus);

				sprintf(strCanStatus, "TX: %d", txFramesCnt);
				lv_label_set_text(labelTxCw, strCanStatus);
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// Read button config
	//////////////////////////////////////////////////////////////////////////
	if(HAL_GPIO_ReadPin (BTN_CFG_GPIO_Port, BTN_CFG_Pin) == 0)
	{
		usb_print("\r\n config");
		HAL_Delay(500);
	}


}
