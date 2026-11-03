/*
 * usart_terminal.c
 *
 *  Created on: Jul 5, 2025
 *      Author: Pep
 */

#include "usart_terminal.h"
#include "lvgl.h"
#include "main_window.h"
#include "main.h"


// UART Debug //
extern lv_obj_t * tab3;
lv_obj_t * uartRxTable;
lv_obj_t * uartTxTa;
lv_obj_t * labelBattStatUs;

// USART data vars //
extern uint32_t cdcRxData[30][80];
extern uint8_t rowCnt;
extern uint32_t rowCntOld;
extern uint32_t rowCntFull;
extern uint8_t usartLines;

// Flow vars //
extern bool uartToucgFlg;

// Debug vars //
extern uint8_t strCdc[100];





//////////////////////////////////////////////////////////////////////////
// USART Terminal callbacks
//////////////////////////////////////////////////////////////////////////

static void ta_uart_scroll_cb(lv_event_t * e)
{
	// Desactiva auto scroll //
	lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED)
    {
//    	uartToucgFlg = false;
	}
}

static void ta_uart_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * ta = lv_event_get_target_obj(e);
	lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

	if(code == LV_EVENT_FOCUSED)
	{
		lv_keyboard_set_textarea(kb, ta);
//		uartToucgFlg = true;
	}
	if(code == LV_EVENT_READY)
	{
		// Send text //
		sprintf(strCdc, "%s",  lv_textarea_get_text(uartTxTa));
		usb_print(strCdc);

		// Print send msg //
		if(rowCnt >= usartLines)
			rowCnt = 1;
		memset(cdcRxData[rowCnt], '\0', 80);
		strcat(cdcRxData[rowCnt], "TX:  ");
		strcat(cdcRxData[rowCnt], lv_textarea_get_text(uartTxTa));

		cdcRxData[rowCnt][78] = rowCnt;
		cdcRxData[rowCnt][79] = rowCntFull;
		rowCnt++;
		rowCntFull++;

		lv_obj_scroll_to_y(uartRxTable, (0), LV_ANIM_ON);
	}
    else if(code == LV_EVENT_CANCEL)
    {
    	// CLR TX textarea //
    	lv_textarea_set_text(uartTxTa, "");

    	// CLR RX textarea //
//    	lv_textarea_set_text(uartRxTa, "");
        lv_obj_scroll_to_y(uartRxTable, 0, LV_ANIM_ON);
        lv_table_set_row_count(uartRxTable, 0);

    	rowCnt = 1;
    	rowCntOld = 1;
    	rowCntFull = 1;
//		uartToucgFlg = true;

		for(int i = 0; i < usartLines; i++)
		{
			for(int j = 0; j < 80; j++)
			{
				cdcRxData[i][j] = 0 ;
			}
		}
    }
}

void draw_rx_msg_table_cb(lv_event_t * e)
{
    lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * base_dsc = lv_draw_task_get_draw_dsc(draw_task);

    /*If the cells are drawn...*/
    if(base_dsc->part == LV_PART_ITEMS)
    {
		uint32_t row = base_dsc->id1;
    }
}


//////////////////////////////////////////////////////////////////////////
// Crea USART Terminal
//////////////////////////////////////////////////////////////////////////

void init_uartDebug()
{
	// tab3 no scrollable //
	lv_obj_set_scrollbar_mode(tab3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(tab3, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_style_bg_color(tab3, lv_color_hex(0x555555), 0);
	lv_obj_set_style_bg_opa(tab3, LV_OPA_COVER, 0);

	///////////////////////////////////////////////////////////
	// Create a keyboard to use it with an of the text areas //
	lv_obj_t * kb = lv_keyboard_create(tab3);
    lv_obj_set_size(kb, 427, 150);
	lv_obj_set_style_bg_color(kb, lv_color_hex(0x555555), 0);
	lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 16); //LV_ALIGN_CENTER

	/////////////////
	// Table Style //
	static lv_style_t styleUart;
	lv_style_init(&styleUart);
	lv_style_set_bg_color(&styleUart, lv_color_hex(0x555555));
	lv_style_set_text_color(&styleUart, lv_color_hex(0xffffff));
	lv_style_set_border_color(&styleUart, lv_color_hex(0x555555));

	// Create RX usart table //
	uartRxTable = lv_table_create(tab3);
	lv_obj_add_style(uartRxTable, &styleUart, 0);
	lv_obj_add_style(uartRxTable, &styleUart, LV_PART_ITEMS);
    lv_obj_set_size(uartRxTable, 427, 126);
    lv_obj_align(uartRxTable, LV_ALIGN_TOP_LEFT, -16, -10);

    // Custom table colum padding //
    lv_obj_set_style_pad_top(uartRxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(uartRxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);

    // Custom table colum width //
    lv_table_set_column_width(uartRxTable, 0, 423);

	// Table draw event callback //
    lv_obj_add_event_cb(uartRxTable, draw_rx_msg_table_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(uartRxTable, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    ////////////////////
	// Battery Status //
    labelBattStatUs = lv_label_create(tab3);
	lv_label_set_text(labelBattStatUs, LV_SYMBOL_BATTERY_2);
	lv_obj_align(labelBattStatUs, LV_ALIGN_TOP_LEFT, -10, -15);

    ///////////////////////
	// Uart TX text area //
	uartTxTa = lv_textarea_create(tab3);
	lv_obj_set_size(uartTxTa, 395, 38);
	lv_obj_align(uartTxTa, LV_ALIGN_TOP_LEFT, 0, 113);

    // TX properties //
	lv_textarea_set_one_line(uartTxTa, true);
    lv_textarea_set_max_length(uartTxTa, 30);
    lv_obj_set_scrollbar_mode(uartTxTa, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(uartTxTa, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(uartTxTa, ta_uart_event_cb, LV_EVENT_ALL, kb);
	lv_textarea_set_placeholder_text(uartTxTa, "TX");

	// Set cursor on text area //
	lv_keyboard_set_textarea(kb, uartTxTa);
}


//////////////////////////////////////////////////////////////////////////
// Print USART in Display Table
//////////////////////////////////////////////////////////////////////////

int sort_rx_usart_buf(const void *a, const void *b)
{
    uint32_t *arrA = (uint32_t *)a;
    uint32_t *arrB = (uint32_t *)b;

    return arrB[79] - arrA[79];
}

void print_rx_usart()
{
	if(rowCntFull != rowCntOld)
	{

		qsort(cdcRxData, usartLines, sizeof(cdcRxData[0]), sort_rx_usart_buf);

		for(int i = 0; (i < rowCnt - 1) && (rowCntFull < usartLines); i++)
		{
//			sprintf(strCdc, "RX:  %s", cdcRxData[i]);
			sprintf(strCdc, "%s", cdcRxData[i]);
			lv_table_set_cell_value(uartRxTable, i, 0, strCdc);
		}

		for(int i = 0; (i < usartLines - 1) && (rowCntFull > usartLines); i++)
		{
//			sprintf(strCdc, "RX:  %s", cdcRxData[i]);
			sprintf(strCdc, "%s", cdcRxData[i]);
			lv_table_set_cell_value(uartRxTable, i, 0, strCdc);
		}
		rowCntOld = rowCntFull;
	}
}














