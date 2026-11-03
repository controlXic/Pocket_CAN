/*
 * can_rx.c
 *
 *  Created on: Feb 16, 2025
 *      Author: Pep
 */
#include "can_rx.h"
#include "main_window.h"
#include "main.h"
#include "lvgl.h"

// CAN Hal //
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_RxHeaderTypeDef RxHeader1;

// CAN RX vars //
uint32_t canRxBuf[100][11];//lv_realloc: couldn't reallocate memory lv_mem.c:142
uint16_t rxBufLen = 100;
uint16_t idRxBufCnt = 0;
uint32_t nFrmCnt = 0;
char canRx2Char[8];

// CAN status vars //
uint8_t busLoadCnt = 0;
extern uint32_t txFramesCnt;
extern uint8_t strCanStatus[10];
extern float oneSeconBitsCan1;
extern uint8_t busLoadBitRate;

// LVGL vars //
lv_obj_t * canRxTable;
lv_obj_t *labelBlRw;
lv_obj_t *labelRxRw;
lv_obj_t *labelTxRw;
lv_obj_t * labelBattStatRx;
extern lv_obj_t * tab1;
extern lv_obj_t * tabview;

// debug vars //
extern uint8_t strCdc[100];

// Print vars //
uint16_t cursorPos = 0;
uint16_t firstFramePos = 0;
bool newFrameFlg = false;
bool printFullRxBufFlg = false;
bool staticMode = false;

// Config vars //
extern uint32_t rxFramesCnt;

// Flow vars //
extern bool tableToucgFlg;





//////////////////////////////////////////////////////////////////////////
// CAN RX table callbacks
//////////////////////////////////////////////////////////////////////////

void draw_canRxTable_cb(lv_event_t * e)
{
    lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * base_dsc = lv_draw_task_get_draw_dsc(draw_task);

    /*If the cells are drawn...*/
    if(base_dsc->part == LV_PART_ITEMS)
    {
		uint32_t row = base_dsc->id1;

        /*Make every 2nd row grayish*/
        if((row != 0 && row % 2) == 0)
        	;
        else
        {
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if(fill_draw_dsc)
            {
                fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), fill_draw_dsc->color, LV_OPA_10);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }
    }
}

void click_canRxTable_cb(lv_event_t * e)
{
	// Desactiva auto scroll //
	lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED)
    {
    	tableToucgFlg = false;
	}
}


//////////////////////////////////////////////////////////////////////////
// Crea CAN RX table
//////////////////////////////////////////////////////////////////////////

void init_canRxTable()
{
	lv_obj_set_style_bg_color(tab1, lv_palette_lighten(LV_PALETTE_BLUE, 3), 0);
	lv_obj_set_style_bg_opa(tab1, LV_OPA_COVER, 0);

	// First tab CAN RX //
	lv_obj_t *label = lv_label_create(tab1);
	lv_label_set_text(label, "ID                       DLC          B0      B1      B2      B3      B4      B5      B6      B7");

	// Battery Status //
	labelBattStatRx = lv_label_create(tab1);
	lv_label_set_text(labelBattStatRx, LV_SYMBOL_BATTERY_2);
	lv_obj_align(labelBattStatRx, LV_ALIGN_TOP_LEFT, -10, -15);

    // CAN RX frames //
	labelRxRw = lv_label_create(tab1);
	sprintf(strCanStatus, "RX: %d", rxFramesCnt);
	lv_label_set_text(labelRxRw, strCanStatus);
	lv_obj_set_style_text_font(labelRxRw, &lv_font_montserrat_10, 0);
    lv_obj_align(labelRxRw, LV_ALIGN_TOP_LEFT, 260, -15);

    // CAN TX frames //
	labelTxRw = lv_label_create(tab1);
	sprintf(strCanStatus, "TX: %d", txFramesCnt);
	lv_label_set_text(labelTxRw, strCanStatus);
	lv_obj_set_style_text_font(labelTxRw, &lv_font_montserrat_10, 0);
    lv_obj_align(labelTxRw, LV_ALIGN_TOP_LEFT, 340, -15);

	// Create table //
	canRxTable = lv_table_create(tab1);

	// table size and pos //
    lv_obj_set_size(canRxTable, 427, 282);//286
    lv_obj_align(canRxTable, LV_ALIGN_CENTER, 0, 18);

    // set colum padding //
    lv_obj_set_style_pad_left(canRxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(canRxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(canRxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(canRxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);

    // set colum width //
    lv_table_set_column_width(canRxTable, 0, 98);
    lv_table_set_column_width(canRxTable, 1, 53);
    lv_table_set_column_width(canRxTable, 2, 34);
    lv_table_set_column_width(canRxTable, 3, 34);
    lv_table_set_column_width(canRxTable, 4, 34);
    lv_table_set_column_width(canRxTable, 5, 34);
    lv_table_set_column_width(canRxTable, 6, 34);
    lv_table_set_column_width(canRxTable, 7, 34);
    lv_table_set_column_width(canRxTable, 8, 34);
    lv_table_set_column_width(canRxTable, 9, 45);

    // configure scroll bars properties //
	lv_obj_set_scrollbar_mode(tab1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(tab1, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_scrollbar_mode(canRxTable, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(canRxTable, LV_DIR_VER);
	lv_obj_remove_flag(canRxTable, LV_OBJ_FLAG_SCROLL_ELASTIC);

	// draw event callback //
    lv_obj_add_event_cb(canRxTable, draw_canRxTable_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(canRxTable, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    lv_obj_add_event_cb(canRxTable, click_canRxTable_cb, LV_EVENT_ALL, NULL);
}



////////////////////////////////////////////////////////////////////////
// CAN RX, Bus Load Statistics
////////////////////////////////////////////////////////////////////////


void can_bus_load()
{
	switch(busLoadBitRate)
	{
		case 10:
			busLoadCnt = ((float)oneSeconBitsCan1 * 0.0001);
		break;

		case 20:
			busLoadCnt = ((float)oneSeconBitsCan1 * 0.0002);
		break;

		case 40:
			busLoadCnt = ((float)oneSeconBitsCan1 * 0.0004);
		break;

		case 80:
			busLoadCnt = ((float)oneSeconBitsCan1 * 0.0008);
		break;

		case 100:
			busLoadCnt = ((float)oneSeconBitsCan1 * 0.001);
		break;

		case 200:
			busLoadCnt = ((float)oneSeconBitsCan1 * 0.002);
		break;
	}
	if(busLoadCnt > 100)
		busLoadCnt = 100;

	oneSeconBitsCan1 = 0;
}


//////////////////////////////////////////////////////////////////////////
// RX CAN guarda buffer mode estatic
//////////////////////////////////////////////////////////////////////////

void save_rx_buf_static(uint32_t id, uint8_t dlc, uint8_t * data)
{
	// comprova si la ID existeix o no; remplaça linea o crea nova linea//
	for(int i = 0; i < rxBufLen; i++)
	{
		if(id == canRxBuf[i][0])
		{
			canRxBuf[i][0] = id;
			canRxBuf[i][1] = dlc;
			canRxBuf[i][2] = data[0];
			canRxBuf[i][3] = data[1];
			canRxBuf[i][4] = data[2];
			canRxBuf[i][5] = data[3];
			canRxBuf[i][6] = data[4];
			canRxBuf[i][7] = data[5];
			canRxBuf[i][8] = data[6];
			canRxBuf[i][9] = data[7];

			i = rxBufLen;
			newFrameFlg = false;
		}
		else if ((id != canRxBuf[i][0]) && (canRxBuf[i][0] == 0) && (idRxBufCnt < rxBufLen))
		{
			canRxBuf[i][0] = id;
			canRxBuf[i][1] = dlc;
			canRxBuf[i][2] = data[0];
			canRxBuf[i][3] = data[1];
			canRxBuf[i][4] = data[2];
			canRxBuf[i][5] = data[3];
			canRxBuf[i][6] = data[4];
			canRxBuf[i][7] = data[5];
			canRxBuf[i][8] = data[6];
			canRxBuf[i][9] = data[7];

			i = rxBufLen;
			idRxBufCnt++;
			newFrameFlg = true;
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// RX CAN buffer, ordena i printa en mode estatic
//////////////////////////////////////////////////////////////////////////

int compare_rx_static_buf(const void *a, const void *b) {
    uint32_t *arrA = (uint32_t *)a;
    uint32_t *arrB = (uint32_t *)b;

    if (arrA[0] < arrB[0]) return -1;
    else if (arrA[0] > arrB[0]) return 1;
    return 0;
}


void update_rx_screen_static()
{
    // organitza Id's de menor a major dins del buffer //
    qsort(canRxBuf, idRxBufCnt, sizeof(canRxBuf[0]), compare_rx_static_buf);

    if(lv_table_get_row_count(canRxTable) < idRxBufCnt)
    	printFullRxBufFlg = true;

    // print all new frames on display //
    if(newFrameFlg == true || printFullRxBufFlg == true)
    {
    	for(int i = 0; i < idRxBufCnt; i++)
    	{
    		sprintf(canRx2Char, "%X", canRxBuf[i][0]);
    		lv_table_set_cell_value(canRxTable, i, 0, canRx2Char);

    		sprintf(canRx2Char, "%X", canRxBuf[i][1]);
    		lv_table_set_cell_value(canRxTable, i, 1, canRx2Char);

    		// clean data row //
    		for(int j = 0; j < 8; j++)
    			lv_table_set_cell_value(canRxTable, i, j + 2, "");

    		// print data row //
    		for(int j = 0; j < canRxBuf[i][1]; j++)
    		{
    			sprintf(canRx2Char, "%X", canRxBuf[i][j + 2]);
    			lv_table_set_cell_value(canRxTable, i, j + 2, canRx2Char);
    		}

    		// Para de printar al sortir de pestanya CAN RX //
    		if(lv_tabview_get_tab_active(tabview) != 0)
    			i = idRxBufCnt;
    	}
    	printFullRxBufFlg = false;
    }

    // print existing frames only in current page view //
    else
    {
    	// read scroll pos and convert to nº row frame//
		cursorPos = lv_obj_get_scroll_top(canRxTable);
		firstFramePos = (cursorPos / 21);

		if(firstFramePos >= rxBufLen - 16)
			firstFramePos = rxBufLen - 16;

		// print buffer on display //
		for(int i = firstFramePos; (i <= firstFramePos + 16) && (i <= idRxBufCnt - 1); i++)
		{
			sprintf(canRx2Char, "%X", canRxBuf[i][0]);
			lv_table_set_cell_value(canRxTable, i, 0, canRx2Char);

			sprintf(canRx2Char, "%X", canRxBuf[i][1]);
			lv_table_set_cell_value(canRxTable, i, 1, canRx2Char);

			// clean data row //
			for(int j = 0; j < 8; j++)
				lv_table_set_cell_value(canRxTable, i, j + 2, "");

			// print data row //
			for(int j = 0; j < canRxBuf[i][1]; j++)
			{
				sprintf(canRx2Char, "%X", canRxBuf[i][j + 2]);
				lv_table_set_cell_value(canRxTable, i, j + 2, canRx2Char);
			}

			// Para de printar al sortir de pestanya CAN RX //
			if(lv_tabview_get_tab_active(tabview) != 0)
				i = idRxBufCnt;
		}
    }
}


//////////////////////////////////////////////////////////////////////////
// RX CAN guarda bufferen mode dinamic
//////////////////////////////////////////////////////////////////////////

void save_rx_buf_dinamic(uint32_t id, uint8_t dlc, uint8_t * data)
{
	canRxBuf[idRxBufCnt][0] = id;
	canRxBuf[idRxBufCnt][1] = dlc;
	canRxBuf[idRxBufCnt][2] = data[0];
	canRxBuf[idRxBufCnt][3] = data[1];
	canRxBuf[idRxBufCnt][4] = data[2];
	canRxBuf[idRxBufCnt][5] = data[3];
	canRxBuf[idRxBufCnt][6] = data[4];
	canRxBuf[idRxBufCnt][7] = data[5];
	canRxBuf[idRxBufCnt][8] = data[6];
	canRxBuf[idRxBufCnt][9] = data[7];
	canRxBuf[idRxBufCnt][10] = nFrmCnt;

	idRxBufCnt++;
	nFrmCnt++;

	if(idRxBufCnt >= rxBufLen)
		idRxBufCnt = 0;
}


//////////////////////////////////////////////////////////////////////////
// RX CAN buffer, ordena i printa en mode dinamic
//////////////////////////////////////////////////////////////////////////

int compare_rx_dinamic_buf(const void *a, const void *b) {
    uint32_t *arrA = (uint32_t *)a;
    uint32_t *arrB = (uint32_t *)b;

    return arrA[10] - arrB[10];
}

void update_rx_screen_dinamic()
{
	if(nFrmCnt < rxBufLen)
		// organitza frames en ordre de recepció //
		qsort(canRxBuf, idRxBufCnt, sizeof(canRxBuf[0]), compare_rx_dinamic_buf);

	else if(nFrmCnt > rxBufLen)
		// organitza frames en ordre de recepció //
		qsort(canRxBuf, rxBufLen, sizeof(canRxBuf[0]), compare_rx_dinamic_buf);


    if((lv_table_get_row_count(canRxTable) < nFrmCnt) && (nFrmCnt <= rxBufLen) ||
    		(lv_table_get_row_count(canRxTable) < rxBufLen) && (nFrmCnt >= rxBufLen))
    	printFullRxBufFlg = true;


    // print all new frames on display //
    if((printFullRxBufFlg == true) && (nFrmCnt <= rxBufLen))
    {
    	for(int i = 0; i < idRxBufCnt; i++)
    	{
    		sprintf(canRx2Char, "%X", canRxBuf[i][0]);
    		lv_table_set_cell_value(canRxTable, i, 0, canRx2Char);

    		sprintf(canRx2Char, "%X", canRxBuf[i][1]);
    		lv_table_set_cell_value(canRxTable, i, 1, canRx2Char);

    		// clean data row //
    		for(int j = 0; j < 8; j++)
    			lv_table_set_cell_value(canRxTable, i, j + 2, "");

    		// print data row //
    		for(int j = 0; j < canRxBuf[i][1]; j++)
    		{
    			sprintf(canRx2Char, "%X", canRxBuf[i][j + 2]);
    			lv_table_set_cell_value(canRxTable, i, j + 2, canRx2Char);
    		}

    		// Para de printar al sortir de pestanya CAN RX //
    		if(lv_tabview_get_tab_active(tabview) != 0)
    			i = idRxBufCnt;
    	}
    	printFullRxBufFlg = false;
    }

    else if((printFullRxBufFlg == true) && (nFrmCnt > rxBufLen))
    {
    	for(int i = 0; i < rxBufLen; i++)
    	{
    		sprintf(canRx2Char, "%X", canRxBuf[i][0]);
    		lv_table_set_cell_value(canRxTable, i, 0, canRx2Char);

    		sprintf(canRx2Char, "%X", canRxBuf[i][1]);
    		lv_table_set_cell_value(canRxTable, i, 1, canRx2Char);

    		// clean data row //
    		for(int j = 0; j < 8; j++)
    			lv_table_set_cell_value(canRxTable, i, j + 2, "");

    		// print data row //
    		for(int j = 0; j < canRxBuf[i][1]; j++)
    		{
    			sprintf(canRx2Char, "%X", canRxBuf[i][j + 2]);
    			lv_table_set_cell_value(canRxTable, i, j + 2, canRx2Char);
    		}

    		// Para de printar al sortir de pestanya CAN RX //
    		if(lv_tabview_get_tab_active(tabview) != 0)
    			i = rxBufLen;
    	}
    	printFullRxBufFlg = false;
    }

    // print existing frames only in current page view //
    else if(nFrmCnt <= rxBufLen)
    {
    	// read scroll pos and convert to nº row frame//
		cursorPos = lv_obj_get_scroll_top(canRxTable);
		firstFramePos = (cursorPos / 21);

		if(firstFramePos >= rxBufLen - 16)
			firstFramePos = rxBufLen - 16;

		// print buffer on display //
		for(int i = firstFramePos; (i < firstFramePos + 16) && (i < idRxBufCnt - 1); i++)
		{
			sprintf(canRx2Char, "%X", canRxBuf[i][0]);
			lv_table_set_cell_value(canRxTable, i, 0, canRx2Char);

			sprintf(canRx2Char, "%X", canRxBuf[i][1]);
			lv_table_set_cell_value(canRxTable, i, 1, canRx2Char);

			// clean data row //
			for(int j = 0; j < 8; j++)
				lv_table_set_cell_value(canRxTable, i, j + 2, "");

			// print data row //
			for(int j = 0; j < canRxBuf[i][1]; j++)
			{
				sprintf(canRx2Char, "%X", canRxBuf[i][j + 2]);
				lv_table_set_cell_value(canRxTable, i, j + 2, canRx2Char);
			}

			// Para de printar al sortir de pestanya CAN RX //
			if(lv_tabview_get_tab_active(tabview) != 0)
			{
				i = idRxBufCnt;
				i = firstFramePos + 16;
			}
		}
    }
    // print existing frames only in current page view //
    else if(nFrmCnt > rxBufLen)
	{
		// read scroll pos and convert to nº row frame//
		cursorPos = lv_obj_get_scroll_top(canRxTable);
		firstFramePos = (cursorPos / 21);

		if(firstFramePos >= rxBufLen - 16)
			firstFramePos = rxBufLen - 16;

		// print buffer on display //
		for(int i = firstFramePos; (i < firstFramePos + 16) /*&& (i < idRxBufCnt - 1)*/; i++)
		{
			sprintf(canRx2Char, "%X", canRxBuf[i][0]);
			lv_table_set_cell_value(canRxTable, i, 0, canRx2Char);

			sprintf(canRx2Char, "%X", canRxBuf[i][1]);
			lv_table_set_cell_value(canRxTable, i, 1, canRx2Char);

			// clean data row //
			for(int j = 0; j < 8; j++)
				lv_table_set_cell_value(canRxTable, i, j + 2, "");

			// print data row //
			for(int j = 0; j < canRxBuf[i][1]; j++)
			{
				sprintf(canRx2Char, "%X", canRxBuf[i][j + 2]);
				lv_table_set_cell_value(canRxTable, i, j + 2, canRx2Char);
			}

			// Para de printar al sortir de pestanya CAN RX //
			if(lv_tabview_get_tab_active(tabview) != 0)
			{
				i = firstFramePos + 16;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// RX CAN Borra buffer i taula rx window
//////////////////////////////////////////////////////////////////////////

void clear_rxTable()
{
	// Clear buffers //
	idRxBufCnt = 0;
	nFrmCnt = 0;
	rxFramesCnt = 0;

    // Esborra buffer //
    for (int i = 0; i < rxBufLen; i++)
    {
        for (int j = 0; j < 11; j++)
        {
            canRxBuf[i][j] = 0;
        }
    }

    // clear table //
    tableToucgFlg = true;
    lv_obj_scroll_to_y(canRxTable, 0, LV_ANIM_ON);
    lv_table_set_row_count(canRxTable, 0);
    update_rx_screen_static();
    update_rx_screen_dinamic();
}




//
















