/*
 * can_tx.c
 *
 *  Created on: Mar 3, 2025
 *      Author: Pep
 */


#include "can_tx.h"
#include "main_window.h"
#include "main.h"
#include "lvgl.h"
#include "stm32g4xx_hal_fdcan.h"


// CAN Hal //
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_TxHeaderTypeDef TxHeader1;

// LVGL CAN TX vars //
extern lv_obj_t * tabview;
extern lv_obj_t * tab2;
lv_obj_t * canTxTable;
lv_obj_t * labelTxTw;
lv_obj_t * labelBlTw;
lv_obj_t * labelRxTw;
lv_obj_t * labelTxTw;
lv_obj_t * labelBattStatTx;
lv_obj_t * ta_id;
lv_obj_t * ta_dlc;
lv_obj_t * ta_b0;
lv_obj_t * ta_b1;
lv_obj_t * ta_b2;
lv_obj_t * ta_b3;
lv_obj_t * ta_b4;
lv_obj_t * ta_b5;
lv_obj_t * ta_b6;
lv_obj_t * ta_b7;
lv_obj_t * msgbox;
lv_obj_t * errMsgbox;


// CAN TX vars //
uint32_t canTxBuf[50][10];
uint32_t canTempBuf[10];
uint8_t txBufLen = 50;
uint8_t idTxBufCnt = 0;
uint8_t sendBufPos = 0;
uint8_t canTxData[8];
char canTx2Char[8];

// CAN TX msg box //
uint32_t nFrame;

// msg box error //
bool msgBoxFlag = false;

// CAN Bus Statistics //
uint8_t strCanStatus[10];
extern uint8_t busLoadCnt;
extern uint32_t rxFramesCnt;
uint32_t txFramesCnt = 0;

// debug vars //
extern uint8_t strCdc[100];
extern uint32_t errorCode;



//////////////////////////////////////////////////////////////////////////
// CAN TX table callbacks
//////////////////////////////////////////////////////////////////////////

void draw_canTxTable_cb(lv_event_t * e)
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

void msgBox_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if(code == LV_EVENT_CLICKED)
    {
    	if(!strcmp(lv_label_get_text(label), "Delete"))
    	{
    		remove_tx_frame(nFrame);
    	}
    	else if(!strcmp(lv_label_get_text(label), "Cancel"));

    	lv_msgbox_close(msgbox);
	}
}


void delete_frame_msgbox(void)
{
    msgbox = lv_msgbox_create(NULL);

    lv_msgbox_add_text(msgbox, "Delete TX Frame?");

    lv_obj_t * btn;
    btn = lv_msgbox_add_footer_button(msgbox, "Delete");
    lv_obj_add_event_cb(btn, msgBox_event_cb, LV_EVENT_CLICKED, NULL);
    btn = lv_msgbox_add_footer_button(msgbox, "Cancel");
    lv_obj_add_event_cb(btn, msgBox_event_cb, LV_EVENT_CLICKED, NULL);

    return;
}


void click_canTxTable_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target_obj(e);
	uint32_t row;
	uint32_t col;

	if(code == LV_EVENT_PRESSED)
    {
		// Assigna el valor del frame tocat als text area per re-enviar //
	    lv_table_get_selected_cell(obj, &row, &col);
	    lv_textarea_set_text(ta_id, lv_table_get_cell_value(obj, row, 0));
	    lv_textarea_set_text(ta_dlc, lv_table_get_cell_value(obj, row, 1));
	    lv_textarea_set_text(ta_b0, lv_table_get_cell_value(obj, row, 2));
	    lv_textarea_set_text(ta_b1, lv_table_get_cell_value(obj, row, 3));
	    lv_textarea_set_text(ta_b2, lv_table_get_cell_value(obj, row, 4));
	    lv_textarea_set_text(ta_b3, lv_table_get_cell_value(obj, row, 5));
	    lv_textarea_set_text(ta_b4, lv_table_get_cell_value(obj, row, 6));
	    lv_textarea_set_text(ta_b5, lv_table_get_cell_value(obj, row, 7));
	    lv_textarea_set_text(ta_b6, lv_table_get_cell_value(obj, row, 8));
	    lv_textarea_set_text(ta_b7, lv_table_get_cell_value(obj, row, 9));
	}
	else if(code == LV_EVENT_LONG_PRESSED)
    {
	    lv_table_get_selected_cell(obj, &nFrame, &col);

	    // Només entra si és més gran que NULL //
	    if(lv_table_get_cell_value(obj, nFrame, 0) > 150000000)
	    	delete_frame_msgbox();
	}
}


//////////////////////////////////////////////////////////////////////////
// Crea CAN TX table
//////////////////////////////////////////////////////////////////////////

void init_canTxTable()
{
	lv_obj_set_style_bg_color(tab2, lv_palette_lighten(LV_PALETTE_GREEN, 3), 0);
	lv_obj_set_style_bg_opa(tab2, LV_OPA_COVER, 0);

	// Tab header CAN TX //
	lv_obj_t * label = lv_label_create(tab2);
	lv_label_set_text(label, "ID                       DLC          B0      B1      B2      B3      B4      B5      B6      B7");


	// Battery Status //
	labelBattStatTx = lv_label_create(tab2);
	lv_label_set_text(labelBattStatTx, LV_SYMBOL_BATTERY_2);
	lv_obj_align(labelBattStatTx, LV_ALIGN_TOP_LEFT, -10, -15);

    // CAN RX frames //
	labelRxTw = lv_label_create(tab2);
	sprintf(strCanStatus, "RX: %d", rxFramesCnt);
	lv_label_set_text(labelRxTw, strCanStatus);
	lv_obj_set_style_text_font(labelRxTw, &lv_font_montserrat_10, 0);
    lv_obj_align(labelRxTw, LV_ALIGN_TOP_LEFT, 260, -15);

    // CAN TX frames //
	labelTxTw = lv_label_create(tab2);
	sprintf(strCanStatus, "TX: %d", txFramesCnt);
	lv_label_set_text(labelTxTw, strCanStatus);
	lv_obj_set_style_text_font(labelTxTw, &lv_font_montserrat_10, 0);
    lv_obj_align(labelTxTw, LV_ALIGN_TOP_LEFT, 340, -15);

	// Create table //
	canTxTable = lv_table_create(tab2);

	// table size and pos //
    lv_obj_set_size(canTxTable, 427, 144);
    lv_obj_align(canTxTable, LV_ALIGN_CENTER, 0, -48);

    // set colum padding //
    lv_obj_set_style_pad_left(canTxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(canTxTable, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(canTxTable, 5, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(canTxTable, 5, LV_PART_ITEMS|LV_STATE_DEFAULT);

    // set colum width //
    lv_table_set_column_width(canTxTable, 0, 98);
    lv_table_set_column_width(canTxTable, 1, 53);
    lv_table_set_column_width(canTxTable, 2, 34);
    lv_table_set_column_width(canTxTable, 3, 34);
    lv_table_set_column_width(canTxTable, 4, 34);
    lv_table_set_column_width(canTxTable, 5, 34);
    lv_table_set_column_width(canTxTable, 6, 34);
    lv_table_set_column_width(canTxTable, 7, 34);
    lv_table_set_column_width(canTxTable, 8, 34);
    lv_table_set_column_width(canTxTable, 9, 45);

    // configure scroll bars properties //
	lv_obj_set_scrollbar_mode(tab2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_scrollbar_mode(canTxTable, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(canTxTable, LV_DIR_VER);
	lv_obj_remove_flag(canTxTable, LV_OBJ_FLAG_SCROLL_ELASTIC);


	// draw event callback //
    lv_obj_add_event_cb(canTxTable, draw_canTxTable_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(canTxTable, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    // click event callback //
    lv_obj_add_event_cb(canTxTable, click_canTxTable_cb, LV_EVENT_ALL, NULL);

}


//////////////////////////////////////////////////////////////////////////
// CAN TX keyboard callbacks
//////////////////////////////////////////////////////////////////////////

static void errorMsg_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if(code == LV_EVENT_CLICKED)
    {
    	if(!strcmp(lv_label_get_text(label), "EXIT"))
		{
    		lv_msgbox_close(errMsgbox);
    		msgBoxFlag = false;
		}
	}
}

void error_msgbox(const char * error)
{
	if(msgBoxFlag == false)
		errMsgbox = lv_msgbox_create(NULL);
	else
		return;

	msgBoxFlag = true;
    lv_msgbox_add_text(errMsgbox, error);

    lv_obj_t * btn;
    btn = lv_msgbox_add_footer_button(errMsgbox, "EXIT");
    lv_obj_add_event_cb(btn, errorMsg_event_cb, LV_EVENT_CLICKED, NULL);
    return;
}

void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * tar = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    // Set cursor on text area //
    if(code == LV_EVENT_FOCUSED)
    {
		lv_keyboard_set_textarea(kb, tar);
    }

    // Clear all text area //
    else if(code == LV_EVENT_CANCEL)
    {
    	lv_textarea_set_text(ta_id, "");
    	lv_textarea_set_text(ta_dlc, "");
    	lv_textarea_set_text(ta_b0, "");
    	lv_textarea_set_text(ta_b1, "");
    	lv_textarea_set_text(ta_b2, "");
    	lv_textarea_set_text(ta_b3, "");
    	lv_textarea_set_text(ta_b4, "");
    	lv_textarea_set_text(ta_b5, "");
    	lv_textarea_set_text(ta_b6, "");
    	lv_textarea_set_text(ta_b7, "");
	}

	// Check DLC max value //
	if(strtoul(lv_textarea_get_text(ta_dlc), NULL, 16) > 8)
	{
		error_msgbox("DLC not correct");
		lv_textarea_set_text(ta_dlc, "0");
		return;
	}

    // Save and send frame //
    else if(code == LV_EVENT_READY)
    {
    	// Check ID max value //
    	if((uint32_t)strtoul(lv_textarea_get_text(ta_id), NULL, 16) > 0x1FFFFFFF)
    	{
    		error_msgbox("ID not correct, max is 0x1FFFFFFF");
    		lv_textarea_set_text(ta_id, "1FFFFFFF");
    		return;
    	}
    	// Check DLC max value //
    	if(strtoul(lv_textarea_get_text(ta_dlc), NULL, 16) > 8)
    	{
    		error_msgbox("DLC not correct");
    		lv_textarea_set_text(ta_dlc, "0");
    		return;
    	}

    	// Neteja velors dels text area depenent del DLC //
    	update_text_areas();

    	// Guarda, Mostra en pantalla i envia Frame //
		save_tx_buf(lv_textarea_get_text(ta_id),
					lv_textarea_get_text(ta_dlc),
					lv_textarea_get_text(ta_b0),
					lv_textarea_get_text(ta_b1),
					lv_textarea_get_text(ta_b2),
					lv_textarea_get_text(ta_b3),
					lv_textarea_get_text(ta_b4),
					lv_textarea_get_text(ta_b5),
					lv_textarea_get_text(ta_b6),
					lv_textarea_get_text(ta_b7));
	}
}


//////////////////////////////////////////////////////////////////////////
// CAN TX. Esborra tots els textArea de TX Frame
//////////////////////////////////////////////////////////////////////////

void update_text_areas()
{
	// Esborra els text area depenent del DLC //
	switch(strtoul(lv_textarea_get_text(ta_dlc), NULL, 16))
	{
		case 0:
			lv_textarea_set_text(ta_b0, "");
			lv_textarea_set_text(ta_b1, "");
			lv_textarea_set_text(ta_b2, "");
			lv_textarea_set_text(ta_b3, "");
			lv_textarea_set_text(ta_b4, "");
			lv_textarea_set_text(ta_b5, "");
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");

		break;

		case 1:
			lv_textarea_set_text(ta_b1, "");
			lv_textarea_set_text(ta_b2, "");
			lv_textarea_set_text(ta_b3, "");
			lv_textarea_set_text(ta_b4, "");
			lv_textarea_set_text(ta_b5, "");
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");
		break;

		case 2:
			lv_textarea_set_text(ta_b2, "");
			lv_textarea_set_text(ta_b3, "");
			lv_textarea_set_text(ta_b4, "");
			lv_textarea_set_text(ta_b5, "");
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");
		break;

		case 3:
			lv_textarea_set_text(ta_b3, "");
			lv_textarea_set_text(ta_b4, "");
			lv_textarea_set_text(ta_b5, "");
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");
		break;

		case 4:
			lv_textarea_set_text(ta_b4, "");
			lv_textarea_set_text(ta_b5, "");
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");
		break;

		case 5:
			lv_textarea_set_text(ta_b5, "");
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");
		break;

		case 6:
			lv_textarea_set_text(ta_b6, "");
			lv_textarea_set_text(ta_b7, "");
		break;

		case 7:
			lv_textarea_set_text(ta_b7, "");
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
// Crea CAN TX Keyboard
//////////////////////////////////////////////////////////////////////////

void can_tx_keyboard()
{
    /*Create an CAN keyboard map*/
    static const char * kb_map[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\n",
                                    "A", "B", "C", "D", "E", "F", LV_SYMBOL_BACKSPACE, LV_SYMBOL_CLOSE, LV_SYMBOL_OK, NULL};

    /*Set the relative width of the buttons and other controls*/
    static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                                     4, 4, 4, 4, 4, 4, 5, 5, 5};

    /*Create a keyboard and add the new map as USER_1 mode*/
    lv_obj_t * kb = lv_keyboard_create(tab2);
    lv_obj_set_size(kb, 427, 100);
	lv_obj_align(kb, LV_ALIGN_CENTER, 0, 110);
	// no scroll //

	lv_obj_set_scrollbar_mode(tab2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);

    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);

    // label for text areas //
    lv_obj_t * ta_label = lv_label_create(tab2);
	lv_obj_set_size(ta_label, 427, 37);
	lv_label_set_text(ta_label, "");
	lv_obj_align(ta_label, LV_ALIGN_CENTER, 0, 41);

    // create a text areas //
    ta_id = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_id, true);
    lv_textarea_set_max_length(ta_id, 8);
	lv_obj_align(ta_id, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_event_cb(ta_id, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_id, 103, 36);
    lv_obj_set_scrollbar_mode(ta_id, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_id, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ta_id, 0, LV_PART_MAIN);

    ta_dlc = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_dlc, true);
    lv_textarea_set_max_length(ta_dlc, 1);
    lv_obj_align(ta_dlc, LV_ALIGN_BOTTOM_LEFT, 103, 0);
    lv_obj_add_event_cb(ta_dlc, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_dlc, 36, 36);
    lv_obj_set_scrollbar_mode(ta_dlc, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_dlc, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_dlc, 0, LV_PART_MAIN);

    ta_b0 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b0, true);
	lv_textarea_set_max_length(ta_b0, 2);
    lv_obj_align(ta_b0, LV_ALIGN_BOTTOM_LEFT, 139, 0);
    lv_obj_add_event_cb(ta_b0, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b0, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b0, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b0, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b0, 0, LV_PART_MAIN);

    ta_b1 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b1, true);
	lv_textarea_set_max_length(ta_b1, 2);
    lv_obj_align(ta_b1, LV_ALIGN_BOTTOM_LEFT, 175, 0);
    lv_obj_add_event_cb(ta_b1, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b1, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b1, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b1, 0, LV_PART_MAIN);

    ta_b2 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b2, true);
	lv_textarea_set_max_length(ta_b2, 2);
    lv_obj_align(ta_b2, LV_ALIGN_BOTTOM_LEFT, 211, 0);
    lv_obj_add_event_cb(ta_b2, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b2, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b2, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b2, 0, LV_PART_MAIN);

    ta_b3 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b3, true);
	lv_textarea_set_max_length(ta_b3, 2);
    lv_obj_align(ta_b3, LV_ALIGN_BOTTOM_LEFT, 247, 0);
    lv_obj_add_event_cb(ta_b3, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b3, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b3, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b3, 0, LV_PART_MAIN);

    ta_b4 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b4, true);
	lv_textarea_set_max_length(ta_b4, 2);
    lv_obj_align(ta_b4, LV_ALIGN_BOTTOM_LEFT, 283, 0);
    lv_obj_add_event_cb(ta_b4, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b4, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b4, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b4, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b4, 0, LV_PART_MAIN);

    ta_b5 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b5, true);
	lv_textarea_set_max_length(ta_b5, 2);
    lv_obj_align(ta_b5, LV_ALIGN_BOTTOM_LEFT, 319, 0);
    lv_obj_add_event_cb(ta_b5, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b5, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b5, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b5, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b5, 0, LV_PART_MAIN);

    ta_b6 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b6, true);
	lv_textarea_set_max_length(ta_b6, 2);
    lv_obj_align(ta_b6, LV_ALIGN_BOTTOM_LEFT, 355, 0);
    lv_obj_add_event_cb(ta_b6, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b6, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b6, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b6, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b6, 0, LV_PART_MAIN);

    ta_b7 = lv_textarea_create(ta_label);
    lv_textarea_set_one_line(ta_b7, true);
	lv_textarea_set_max_length(ta_b7, 2);
    lv_obj_align(ta_b7, LV_ALIGN_BOTTOM_LEFT, 391, 0);
    lv_obj_add_event_cb(ta_b7, ta_event_cb, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta_b7, 36, 36);
    lv_obj_set_scrollbar_mode(ta_b7, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ta_b7, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(ta_b7, 0, LV_PART_MAIN);

	// Set cursor on text area //
	lv_keyboard_set_textarea(kb, ta_id);

}


//////////////////////////////////////////////////////////////////////////
// CAN TX Guarda buffer nou frame
//////////////////////////////////////////////////////////////////////////

void save_tx_buf(const char *id, const char * dlc, const char * b0, const char * b1, const char * b2, const char * b3,
												   const char * b4, const char * b5, const char * b6, const char * b7)
{
	canTempBuf[0] = strtoul(id, NULL, 16);
	canTempBuf[1] = strtoul(dlc, NULL, 16);
	canTempBuf[2] = strtoul(b0, NULL, 16);
	canTempBuf[3] = strtoul(b1, NULL, 16);
	canTempBuf[4] = strtoul(b2, NULL, 16);
	canTempBuf[5] = strtoul(b3, NULL, 16);
	canTempBuf[6] = strtoul(b4, NULL, 16);
	canTempBuf[7] = strtoul(b5, NULL, 16);
	canTempBuf[8] = strtoul(b6, NULL, 16);
	canTempBuf[9] = strtoul(b7, NULL, 16);

	int res;

	for(int i = 0; i < idTxBufCnt; i++)
	{
		res = memcmp(&canTempBuf, &canTxBuf[i], sizeof(canTempBuf));
		if(res == 0)
			i = idTxBufCnt;
	}
	if(((canTempBuf[0] > 0) && (res != 0)) || ((idTxBufCnt == 0) && (canTempBuf[0] > 0)))
	{
		// Mode acumulacio d'IDs repetides amb diferent data o no //
		canTxBuf[idTxBufCnt][0] = strtoul(id, NULL, 16);
		canTxBuf[idTxBufCnt][1] = strtoul(dlc, NULL, 16);
		canTxBuf[idTxBufCnt][2] = strtoul(b0, NULL, 16);
		canTxBuf[idTxBufCnt][3] = strtoul(b1, NULL, 16);
		canTxBuf[idTxBufCnt][4] = strtoul(b2, NULL, 16);
		canTxBuf[idTxBufCnt][5] = strtoul(b3, NULL, 16);
		canTxBuf[idTxBufCnt][6] = strtoul(b4, NULL, 16);
		canTxBuf[idTxBufCnt][7] = strtoul(b5, NULL, 16);
		canTxBuf[idTxBufCnt][8] = strtoul(b6, NULL, 16);
		canTxBuf[idTxBufCnt][9] = strtoul(b7, NULL, 16);

		idTxBufCnt++;
		update_tx_screen();
	}
	if(canTempBuf[0] > 0)
	{
		send_tx_frame();
	}
}


//////////////////////////////////////////////////////////////////////////
// CAN TX buffer, ordena i printa
//////////////////////////////////////////////////////////////////////////

int compare_tx_buf(const void *a, const void *b) {
    uint32_t *arrA = (uint32_t *)a;
    uint32_t *arrB = (uint32_t *)b;

    if (arrA[0] < arrB[0]) return -1;
    else if (arrA[0] > arrB[0]) return 1;
    return 0;
}


void update_tx_screen()
{
    // organitza Id's de menor a major dins del buffer //
	qsort(canTxBuf, idTxBufCnt, sizeof(canTxBuf[0]), compare_tx_buf);

	// print buffer in display //
	for(int i = 0; i < idTxBufCnt; i++)
	{
		sprintf(canTx2Char, "%X", canTxBuf[i][0]);
		lv_table_set_cell_value(canTxTable, i, 0, canTx2Char);

		sprintf(canTx2Char, "%X", canTxBuf[i][1]);
		lv_table_set_cell_value(canTxTable, i, 1, canTx2Char);

		// clean data row //
		for(int j = 0; j < 8; j++)
			lv_table_set_cell_value(canTxTable, i, j + 2, "");

		// print data row //
		for(int j = 0; j < canTxBuf[i][1]; j++)
		{
			sprintf(canTx2Char, "%X", canTxBuf[i][j + 2]);
			lv_table_set_cell_value(canTxTable, i, j + 2, canTx2Char);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// CAN TX Envia CAN frame
//////////////////////////////////////////////////////////////////////////

void send_tx_frame()
{
	TxHeader1.Identifier = canTempBuf[0];

	if(canTempBuf[0] <= 0x7FF)
		TxHeader1.IdType = FDCAN_STANDARD_ID;
	else
		TxHeader1.IdType = FDCAN_EXTENDED_ID;

	TxHeader1.TxFrameType = FDCAN_DATA_FRAME;
	TxHeader1.DataLength = canTempBuf[1];
	TxHeader1.ErrorStateIndicator = FDCAN_ESI_ACTIVE; //FDCAN_ESI_PASSIVE;
	TxHeader1.BitRateSwitch = FDCAN_BRS_OFF;
	TxHeader1.FDFormat = FDCAN_CLASSIC_CAN;
	TxHeader1.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	TxHeader1.MessageMarker = 0;

	canTxData[0] = canTempBuf[2];
	canTxData[1] = canTempBuf[3];
	canTxData[2] = canTempBuf[4];
	canTxData[3] = canTempBuf[5];
	canTxData[4] = canTempBuf[6];
	canTxData[5] = canTempBuf[7];
	canTxData[6] = canTempBuf[8];
	canTxData[7] = canTempBuf[9];




	// Envia al buf de sortida //
	if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader1, canTxData) != HAL_OK)
	{
		//Error_Handler();
		errorCode = HAL_FDCAN_GetError(&hfdcan1);

		if(errorCode & 0X08)
		{
			error_msgbox("FDCAN_IT_BUS_OFF");
		}
	}

	txFramesCnt++;

	sprintf(strCanStatus, "TX: %d", txFramesCnt);
	lv_label_set_text(labelTxTw, strCanStatus);
}


//////////////////////////////////////////////////////////////////////////
// CAN TX esborra un frame individualment de TX table, long ress
//////////////////////////////////////////////////////////////////////////

void remove_tx_frame(uint32_t nFrame)
{
	for (int j = 0; j < 10; j++)
		canTxBuf[nFrame][j] = canTxBuf[idTxBufCnt - 1][j];

	for (int j = 0; j < 10; j++)
		canTxBuf[idTxBufCnt - 1][j] = 0;

	idTxBufCnt--;

    lv_obj_scroll_to_y(canTxTable, 0, LV_ANIM_ON);
    lv_table_set_row_count(canTxTable, 0);
	update_tx_screen();
}


//////////////////////////////////////////////////////////////////////////
// CAN TX esborra tot de TX table
//////////////////////////////////////////////////////////////////////////

void clear_txTable()
{
	// Clear buffers //
	idTxBufCnt = 0;
	txFramesCnt = 0;

    for (int i = 0; i < txBufLen; i++)
    {
        for (int j = 0; j < 11; j++)
        {
            canTxBuf[i][j] = 0;
        }
    }

    // clear table //
    lv_obj_scroll_to_y(canTxTable, 0, LV_ANIM_ON);
    lv_table_set_row_count(canTxTable, 0);
}



