/*
 * main_window.c
 *
 *  Created on: Feb 8, 2025
 *      Author: Pep
 */
#include "main_window.h"
#include "main.h"
#include "lvgl.h"
#include "logoMini.h"
#include "can_rx.h"
#include "can_tx.h"
#include "usart_terminal.h"
#include "stdio.h"


// HAL CAN vars //
extern FDCAN_HandleTypeDef hfdcan1;
extern uint32_t rxFramesCnt;
extern uint32_t rxFramesCnt;

// Debug vars //
extern uint8_t strCdc[100];

// LVGL main window objects //
lv_obj_t * labelWin;
lv_obj_t * tabview;
lv_obj_t * tab1;
lv_obj_t * tab2;
lv_obj_t * tab3;
lv_obj_t * tab4;
lv_obj_t * labelBattStatCf;


// LVGL Config obj //
lv_obj_t * labelBlCw;
lv_obj_t * labelRxCw;
lv_obj_t * labelTxCw;
lv_obj_t * btnStSp;
lv_style_t style_radio;
lv_style_t style_radio_chk;
lv_obj_t * bitRateMenu;

// Config filter Window //
lv_obj_t * filterWin;
lv_obj_t *taF1;
lv_obj_t *taF2;
lv_obj_t *taF3;
lv_obj_t *taF4;


// Config vars //
uint32_t radButtIndex = 6;
extern bool staticMode;
extern bool sleepFlg;
extern uint32_t millis;
extern uint32_t sleepMillis;
extern uint32_t rxFramesCnt;
extern uint32_t txFramesCnt;
uint8_t strFilter[10];
uint32_t F1, F2, F3, F4 = 0x00;


//////////////////////////////////////////////////////////////////////////
// Init logo
//////////////////////////////////////////////////////////////////////////

void init_logo()
{
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_t * label = lv_label_create(lv_screen_active());
	lv_label_set_text(label, "");
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *imgLogo;
	LV_IMAGE_DECLARE(logoMini);
	imgLogo = lv_image_create(lv_screen_active());
	lv_image_set_src(imgLogo, &logoMini);
	lv_obj_align(imgLogo, LV_ALIGN_CENTER, 0, -20);
}


//////////////////////////////////////////////////////////////////////////
// Main window config
//////////////////////////////////////////////////////////////////////////

void init_window()
{
	// clean window //
	lv_obj_clean(lv_screen_active());

	// Create a Tab view object //
	tabview = lv_tabview_create(lv_screen_active());
	lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);
	lv_tabview_set_tab_bar_size(tabview, 53);

	lv_obj_t * tab_buttons = lv_tabview_get_tab_bar(tabview);
	lv_obj_set_style_bg_color(tab_buttons, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
	lv_obj_set_style_text_color(tab_buttons, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
	lv_obj_set_style_border_side(tab_buttons, LV_BORDER_SIDE_RIGHT, LV_PART_ITEMS | LV_STATE_CHECKED);

	// Disable scroll between tabs //
	lv_obj_remove_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);

	/*Add 3 tabs (the tabs are page (lv_page) and can be scrolled*/
	tab1 = lv_tabview_add_tab(tabview, "CAN\n  RX");
	tab2 = lv_tabview_add_tab(tabview, "CAN\n  TX");
	tab3 = lv_tabview_add_tab(tabview, "USART");
	tab4 = lv_tabview_add_tab(tabview, "CONF");

	// tab1 //
	init_canRxTable();

	// tab2 //
	init_canTxTable();
	can_tx_keyboard();

    // tab3 //
    init_uartDebug();

    // tab4 //
    init_config();
}



//////////////////////////////////////////////////////////////////////////
// CONF callbacks
//////////////////////////////////////////////////////////////////////////

static void btnStSp_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        if(cnt % 2 == 0)
        {
			if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
				Error_Handler();

			lv_obj_set_style_bg_color(btnStSp, lv_color_hex(0xffce1c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t * label = lv_obj_get_child(btn, 0);
            lv_label_set_text(label, "STOP");

        	rxFramesCnt = 0;
        	txFramesCnt = 0;

            lv_obj_add_state(bitRateMenu, LV_STATE_DISABLED);
        }
        else
        {
        	if (HAL_FDCAN_Stop(&hfdcan1) != HAL_OK)
        					Error_Handler();

        	lv_obj_set_style_bg_color(btnStSp, lv_color_hex(0x02b202), LV_PART_MAIN | LV_STATE_DEFAULT);
        	lv_obj_t * label = lv_obj_get_child(btn, 0);
        	lv_label_set_text(label, "START");

        	lv_obj_remove_state(bitRateMenu, LV_STATE_DISABLED);
        }
    }
}

static void bit_rate_menu_cb(lv_event_t * e)
{
    lv_obj_t * dropdown = lv_event_get_target_obj(e);
    char buf[64];
    lv_dropdown_get_selected_str(dropdown, buf, sizeof(buf));


    if(strcmp(buf, "1000 kbit/s") == 0)
    	FDCAN1_bit_rate_config(10);
    else if(strcmp(buf, "500 kbit/s") == 0)
    	FDCAN1_bit_rate_config(20);
    else if(strcmp(buf, "250 kbit/s") == 0)
    	FDCAN1_bit_rate_config(40);
    else if(strcmp(buf, "125 kbit/s") == 0)
    	FDCAN1_bit_rate_config(80);
    else if(strcmp(buf, "100 kbit/s") == 0)
    	FDCAN1_bit_rate_config(100);
    else if(strcmp(buf, "50 kbit/s") == 0)
    	FDCAN1_bit_rate_config(200);
}


// CAN RX Print mode //
static void radio_event_handler(lv_event_t * e)
{
    uint32_t * active_id = (uint32_t *)lv_event_get_user_data(e);
    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = lv_event_get_target_obj(e);
    lv_obj_t * old_cb = lv_obj_get_child(cont, *active_id);

    /*Do nothing if the container was clicked*/
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);

    *active_id = lv_obj_get_index(act_cb);

    //LV_LOG_USER("Selected radio buttons: %d", (int)radButtIndex);

    if(radButtIndex == 5)
    {
    	staticMode = false;
    	clear_rxTable();
    }
    else
    {
    	staticMode = true;
    	clear_rxTable();
    }

}

static void btnFilter_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED)
    {
    	filters_window();
    }
}

static void btnClrRx_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED)
    {
    	clear_rxTable();
    }
}

static void btnClrTx_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED)
    {
    	clear_txTable();
    }
}

static void sw_sleep_event_cb(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target_obj(e);

    if(lv_obj_has_state(sw, LV_STATE_CHECKED))
    {
    	sleepFlg = true;
		sleepMillis = millis;
    }
    else
    {
    	sleepFlg = false;
    }
}


//////////////////////////////////////////////////////////////////////////
// Init CONF
//////////////////////////////////////////////////////////////////////////

void init_config()
{
	// Disable eslatic scroll //
	lv_obj_remove_flag(tab4, LV_OBJ_FLAG_SCROLL_ELASTIC);
	lv_obj_remove_flag(tab4, LV_OBJ_FLAG_SCROLLABLE);

	// Battery Status //
	labelBattStatCf = lv_label_create(tab4);
	lv_label_set_text(labelBattStatCf, LV_SYMBOL_BATTERY_2);
	lv_obj_align(labelBattStatCf, LV_ALIGN_TOP_LEFT, -10, -15);

	////////////////////////
    // Start Stop CAN Bus //
	lv_obj_t * label4 = lv_label_create(tab4);
	lv_label_set_text(label4, "START/STOP Bus");
    lv_obj_align(label4, LV_ALIGN_TOP_LEFT, 0, 0);

    btnStSp = lv_button_create(tab4);
    lv_obj_set_size(btnStSp, 70, 35);
    lv_obj_set_pos(btnStSp, 15, 35);
    lv_obj_set_style_bg_color(btnStSp, lv_color_hex(0xffce1c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btnStSp, btnStSp_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * label5 = lv_label_create(btnStSp);
    lv_label_set_text(label5, "STOP");
    lv_obj_center(label5);

    ///////////////////////
	// CAN Bit rate menu //
	lv_obj_t * labelBitR = lv_label_create(tab4);
	lv_label_set_text(labelBitR, "CAN Bus Speed");
	lv_obj_align(labelBitR, LV_ALIGN_TOP_LEFT, 145, 0);

	bitRateMenu = lv_dropdown_create(tab4);
	lv_obj_align(bitRateMenu, LV_ALIGN_TOP_LEFT, 132, 32);
	lv_dropdown_set_options(bitRateMenu,"1000 kbit/s\n"
										"500 kbit/s\n"
										"250 kbit/s\n"
										"125 kbit/s\n"
										"100 kbit/s\n"
										"50 kbit/s");

	lv_dropdown_set_selected_highlight(bitRateMenu, true);
	lv_obj_add_event_cb(bitRateMenu, bit_rate_menu_cb, LV_EVENT_VALUE_CHANGED, NULL);

	///////////////////
	// RX Print mode //
	lv_obj_t * label = lv_label_create(tab4);
	lv_label_set_text(label, "RX Print mode");
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 290, 0);

	lv_style_init(&style_radio);
	lv_style_set_radius(&style_radio, LV_RADIUS_CIRCLE);

	lv_style_init(&style_radio_chk);
	lv_style_set_bg_image_src(&style_radio_chk, NULL);

	lv_obj_t * radB;
	radB = lv_checkbox_create(tab4);
	lv_checkbox_set_text(radB, "Trace");
	lv_obj_align(radB, LV_ALIGN_TOP_LEFT, 300, 25);
	lv_obj_add_flag(radB, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_style(radB, &style_radio, LV_PART_INDICATOR);
	lv_obj_add_style(radB, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

	radB = lv_checkbox_create(tab4);
	lv_checkbox_set_text(radB, "Static");
	lv_obj_align(radB, LV_ALIGN_TOP_LEFT, 300, 60);
	lv_obj_add_flag(radB, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_style(radB, &style_radio, LV_PART_INDICATOR);
	lv_obj_add_style(radB, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

	lv_obj_add_event_cb(tab4, radio_event_handler, LV_EVENT_CLICKED, &radButtIndex);
	lv_obj_add_state(lv_obj_get_child(tab4, 6), LV_STATE_CHECKED); // static mode checked

//    ///////////////////
//    // Filter button //
//	lv_obj_t * labelFil = lv_label_create(tab4);
//	lv_label_set_text(labelFil, "CAN Filters config");
//    lv_obj_align(labelFil, LV_ALIGN_TOP_LEFT, 0, 100);
//
//    lv_obj_t * btnFilterWin = lv_button_create(tab4);
//    lv_obj_set_size(btnFilterWin, 70, 35);
//    lv_obj_set_pos(btnFilterWin, 15, 135);
//    lv_obj_add_event_cb(btnFilterWin, btnFilter_event_cb, LV_EVENT_ALL, NULL);
//    lv_obj_t * labelBtnFil = lv_label_create(btnFilterWin);
//    lv_label_set_text(labelBtnFil, "Filters");
//    lv_obj_center(labelBtnFil);

    /////////////////////////
    // Clear CAN RX screen //
	lv_obj_t * label2 = lv_label_create(tab4);
	lv_label_set_text(label2, "RX Screen CLEAR");
    lv_obj_align(label2, LV_ALIGN_TOP_LEFT, 145, 100);

    lv_obj_t * btnClrRx = lv_button_create(tab4);
    lv_obj_set_size(btnClrRx, 70, 35);
    lv_obj_set_pos(btnClrRx, 160, 135);
    lv_obj_set_style_bg_color(btnClrRx, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_add_event_cb(btnClrRx, btnClrRx_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * label3 = lv_label_create(btnClrRx);
    lv_label_set_text(label3, "CLEAR");
    lv_obj_center(label3);

    /////////////////////////
    // Clear CAN TX screen //
	lv_obj_t * label6 = lv_label_create(tab4);
	lv_label_set_text(label6, "TX Screen CLEAR");
    lv_obj_align(label6, LV_ALIGN_TOP_LEFT, 290, 100);

    lv_obj_t * btnClrTx = lv_button_create(tab4);
    lv_obj_set_size(btnClrTx, 70, 35);
    lv_obj_set_pos(btnClrTx, 305, 135);
    lv_obj_set_style_bg_color(btnClrTx, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_add_event_cb(btnClrTx, btnClrTx_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * label7 = lv_label_create(btnClrTx);
    lv_label_set_text(label7, "CLEAR");
    lv_obj_center(label7);

    ///////////////////
    // Filter button //
	lv_obj_t * labelFil = lv_label_create(tab4);
	lv_label_set_text(labelFil, "CAN Filters config");
    lv_obj_align(labelFil, LV_ALIGN_TOP_LEFT, 0, 100);

    lv_obj_t * btnFilterWin = lv_button_create(tab4);
    lv_obj_set_size(btnFilterWin, 70, 35);
    lv_obj_set_pos(btnFilterWin, 15, 135);
    lv_obj_add_event_cb(btnFilterWin, btnFilter_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * labelBtnFil = lv_label_create(btnFilterWin);
    lv_label_set_text(labelBtnFil, "Filters");
    lv_obj_center(labelBtnFil);

    ////////////////////
    // CAN bus status //
	lv_obj_t * label8 = lv_label_create(tab4);
	lv_label_set_text(label8, "CAN Bus Status");
    lv_obj_align(label8, LV_ALIGN_TOP_LEFT, 0, 200);

	labelBlCw = lv_label_create(tab4);
	lv_label_set_text(labelBlCw, "BL: 0 %");
    lv_obj_align(labelBlCw, LV_ALIGN_TOP_LEFT, 10, 225);

    labelRxCw = lv_label_create(tab4);
	lv_label_set_text(labelRxCw, "RX: 0");
    lv_obj_align(labelRxCw, LV_ALIGN_TOP_LEFT, 10, 245);

    labelTxCw = lv_label_create(tab4);
	lv_label_set_text(labelTxCw, "TX: 0");
    lv_obj_align(labelTxCw, LV_ALIGN_TOP_LEFT, 10, 265);

    ////////////////
    // Sleep mode //
    lv_obj_t * label12 = lv_label_create(tab4);
    lv_label_set_text(label12, "Sleep mode");
    lv_obj_align(label12, LV_ALIGN_TOP_LEFT, 157, 200);

    lv_obj_t * swSleep = lv_switch_create(tab4);
    lv_obj_align(swSleep, LV_ALIGN_TOP_LEFT, 167, 235);
    lv_obj_add_event_cb(swSleep, sw_sleep_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_state(swSleep, LV_STATE_DEFAULT); // LV_STATE_CHECKED


    /////////////////
    // Label C-Xic //
	lv_obj_t * labelCX = lv_label_create(tab4);
	lv_label_set_text(labelCX, "Powered by Control Xic");
	lv_obj_set_style_text_color(labelCX, lv_color_hex(0xB0BEC5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(labelCX, LV_ALIGN_BOTTOM_MID, 0, 15);


}


//////////////////////////////////////////////////////////////////////////
// CONF Filter Window callbacks
//////////////////////////////////////////////////////////////////////////

static void btnReturn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED)
    {
    	lv_obj_delete(filterWin);
    }
}


static void taFilter_event_cb(lv_event_t * e)
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
    	lv_textarea_set_text(taF1, "");
    	lv_textarea_set_text(taF2, "");
    	lv_textarea_set_text(taF3, "");
    	lv_textarea_set_text(taF4, "");

    	lv_textarea_set_placeholder_text(taF1, "CAN ID Filter 1");
    	lv_textarea_set_placeholder_text(taF2, "CAN ID Filter 2");
    	lv_textarea_set_placeholder_text(taF3, "CAN ID Filter 3");
    	lv_textarea_set_placeholder_text(taF4, "CAN ID Filter 4");
	}

    // Save and send frame //
    else if(code == LV_EVENT_READY)
    {
    	F1 = (uint32_t)strtoul(lv_textarea_get_text(taF1), NULL, 16);
    	F2 = (uint32_t)strtoul(lv_textarea_get_text(taF2), NULL, 16);
    	F3 = (uint32_t)strtoul(lv_textarea_get_text(taF3), NULL, 16);
    	F4 = (uint32_t)strtoul(lv_textarea_get_text(taF4), NULL, 16);


    	if(F1 > 0x1FFFFFFF)
    	{
    		F1 = 0x1FFFFFFF;
    		lv_textarea_set_text(taF1, "0x1FFFFFFF");
    	}
    	if(F2 > 0x1FFFFFFF)
    	{
    		F2 = 0x1FFFFFFF;
    		lv_textarea_set_text(taF2, "0x1FFFFFFF");
    	}
    	if(F3 > 0x1FFFFFFF)
    	{
    		F3 = 0x1FFFFFFF;
    		lv_textarea_set_text(taF3, "0x1FFFFFFF");
    	}
    	if(F4 > 0x1FFFFFFF)
    	{
    		F4 = 0x1FFFFFFF;
    		lv_textarea_set_text(taF4, "0x1FFFFFFF");
    	}

    	// Aplica Filtres //
    	FDCAN_config_filters(F1, F2, F3, F4);

    	// Tanca finestrea //
    	lv_obj_delete(filterWin);
	}
}

//////////////////////////////////////////////////////////////////////////
// Crea CONF Filter Window
//////////////////////////////////////////////////////////////////////////

void filters_window()
{
	// Create Window //
	filterWin = lv_obj_create(tab4);
    lv_obj_set_size(filterWin, 427, 320);
    lv_obj_align(filterWin, LV_ALIGN_TOP_LEFT, -16, -16);
    lv_obj_set_style_bg_color(filterWin, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_set_style_border_width(filterWin, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(filterWin, 0, LV_PART_MAIN);

    lv_obj_remove_flag(filterWin, LV_OBJ_FLAG_SCROLL_ELASTIC);
	lv_obj_remove_flag(filterWin, LV_OBJ_FLAG_SCROLLABLE);

	// Label del titol //
	lv_obj_t * labelFilTi = lv_label_create(filterWin);
	lv_label_set_text(labelFilTi, "CONF -> CAN Filters config");
    lv_obj_align(labelFilTi, LV_ALIGN_TOP_LEFT, 0, -6);

	// Label info id 0 //
	lv_obj_t * labelId0 = lv_label_create(filterWin);
	lv_label_set_text(labelId0, "The ID 0x00 is always\n   accepted and read");
	lv_obj_align(labelId0, LV_ALIGN_TOP_LEFT, 270, 80);

	// btnReturn //
	lv_obj_t *btnReturn = lv_button_create(filterWin);
	lv_obj_set_pos(btnReturn, 355, 0);
	lv_obj_set_size(btnReturn, 33, 33);
	lv_obj_set_style_radius(btnReturn, 35, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btnReturn, lv_color_hex(0xffce1c1c), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_event_cb(btnReturn, btnReturn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_t * labelBtnRet = lv_label_create(btnReturn);
	lv_label_set_text(labelBtnRet, LV_SYMBOL_CLOSE );
	lv_obj_center(labelBtnRet);


    // Create CAN keyboard map //
    static const char * kb_map[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\n",
                                    "A", "B", "C", "D", "E", "F", LV_SYMBOL_BACKSPACE, LV_SYMBOL_CLOSE, LV_SYMBOL_OK, NULL};

    // Set the relative width of the buttons and other controls //
    static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                                     4, 4, 4, 4, 4, 4, 5, 5, 5};

    // Create a keyboard and add the new map as USER_1 mode //
    lv_obj_t * kb = lv_keyboard_create(filterWin);
    lv_obj_set_size(kb, 427, 100);
	lv_obj_align(kb, LV_ALIGN_TOP_LEFT, -16, 203);

    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);


    // La ID de CAN mes gran es  0x1FFFFFFF //

	// Text Area Filter 1 //
	taF1 = lv_textarea_create(filterWin);
	lv_obj_set_pos(taF1, 5, 20);
	lv_obj_set_size(taF1, 250, 38);
	lv_textarea_set_max_length(taF1, 8);
	lv_textarea_set_one_line(taF1, true);
	lv_obj_add_event_cb(taF1, taFilter_event_cb, LV_EVENT_ALL, kb);
	if(F1 > 0)
	{
		sprintf(strFilter, "%X", F1);
		lv_textarea_set_text(taF1, strFilter);
	}
	else
		lv_textarea_set_placeholder_text(taF1, "CAN ID Filter 1");

	// Text Area Filter 2 //
	taF2 = lv_textarea_create(filterWin);
	lv_obj_set_pos(taF2, 5, 63);
	lv_obj_set_size(taF2, 250, 38);
	lv_textarea_set_max_length(taF2, 8);
	lv_textarea_set_one_line(taF2, true);
	lv_obj_add_event_cb(taF2, taFilter_event_cb, LV_EVENT_ALL, kb);
	if(F2 > 0)
	{
		sprintf(strFilter, "%X", F2);
		lv_textarea_set_text(taF2, strFilter);
	}
	else
		lv_textarea_set_placeholder_text(taF2, "CAN ID Filter 2");

	// Text Area Filter 3 //
	taF3 = lv_textarea_create(filterWin);
	lv_obj_set_pos(taF3, 5, 106);
	lv_obj_set_size(taF3, 250, 38);
	lv_textarea_set_max_length(taF3, 8);
	lv_textarea_set_one_line(taF3, true);
	lv_obj_add_event_cb(taF3, taFilter_event_cb, LV_EVENT_ALL, kb);
	if(F3 > 0)
	{
		sprintf(strFilter, "%X", F3);
		lv_textarea_set_text(taF3, strFilter);
	}
	else
		lv_textarea_set_placeholder_text(taF3, "CAN ID Filter 3");

	// Text Area Filter 4 //
	taF4 = lv_textarea_create(filterWin);
	lv_obj_set_pos(taF4, 5, 150);
	lv_obj_set_size(taF4, 250, 38);
	lv_textarea_set_max_length(taF4, 8);
	lv_textarea_set_one_line(taF4, true);
	lv_obj_add_event_cb(taF4, taFilter_event_cb, LV_EVENT_ALL, kb);
	if(F4 > 0)
	{
		sprintf(strFilter, "%X", F4);
		lv_textarea_set_text(taF4, strFilter);
	}
	else
		lv_textarea_set_placeholder_text(taF4, "CAN ID Filter 4");

	// Set cursor on text area //
	lv_keyboard_set_textarea(kb, taF1);

}





















