#include "st7796s.h"
#include "main.h"
#include "stm32g4xx_hal_spi.h"
#include "stm32g4xx_hal_i2c.h"
#include "lvgl.h"


// Debug vars //
extern uint8_t strCdc[100];

// SPI vars //
extern SPI_HandleTypeDef hspi1;

// TFT vars //
uint16_t _width;
uint16_t _height;
uint8_t _rotation;

// I2C vars //
extern I2C_HandleTypeDef hi2c4;
uint8_t tpTxBuf[16];
uint8_t tpRxBuf[16];

// touch vars //
tp_status tpData;
extern bool touchFlg;
extern bool touchSleepFlg;

// LVGL vars//
// TFT screen resolution //
#define MY_DISP_HOR_RES 480
#define MY_DISP_VER_RES 320
// lvgl buffers
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define BUFF_SIZE (MY_DISP_HOR_RES * 10 * BYTES_PER_PIXEL) // x2 = 9.38
static uint8_t buff1[BUFF_SIZE];
static uint8_t buff2[BUFF_SIZE];

// lvgl object vars //
extern lv_obj_t * tabview;

// static functions //
static void my_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map);
void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data);


//////////////////////////////////////////////////////////////////////////////////////
// TFT Functions
//////////////////////////////////////////////////////////////////////////////////////

void tft_init()
{

	// LVGL Initialize the display //
	lv_display_t * display = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
	lv_display_set_buffers(display, buff1, buff2, sizeof(buff1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display, my_flush_cb);

	// Init vars //
    _height = TFT_HEIGHT;
    _width = TFT_WIDTH;
    _rotation = 0;

	// Enable TFT //
	HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, SET);
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, RESET);
	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, SET);

	// TFT ST7796S init secunece //
    tft_startWrite();

		tft_writeCommand(0x11);
		HAL_Delay(150);

		tft_writeCommand(0xF0);
		tft_write_8(0xC3);

		tft_writeCommand(0xF0);
		tft_write_8(0x96);

		tft_writeCommand(0x36);
		tft_write_8(0x48);

		tft_writeCommand(0x3A);
		tft_write_8(0x05);

		tft_writeCommand(0xB6);
		tft_write_8(0x00);
		tft_write_8(0x02);

		tft_writeCommand(0xB5);
		tft_write_8(0x02);
		tft_write_8(0x03);
		tft_write_8(0x00);
		tft_write_8(0x04);

		tft_writeCommand(0xB1);
		tft_write_8(0x80);
		tft_write_8(0x10);

		tft_writeCommand(0xB4);
		tft_write_8(0x00);

		tft_writeCommand(0xB7);
		tft_write_8(0xC6);

		tft_writeCommand(0xC5);
		tft_write_8(0x1C);

		tft_writeCommand(0xE4);
		tft_write_8(0x31);

		tft_writeCommand(0xE8);
		tft_write_8(0x40);
		tft_write_8(0x8A);
		tft_write_8(0x00);
		tft_write_8(0x00);
		tft_write_8(0x29);
		tft_write_8(0x19);
		tft_write_8(0xA5);
		tft_write_8(0x33);

		tft_writeCommand(0xC2);
		tft_write_8(0xA7);

		tft_writeCommand(0xE0);
		tft_write_8(0xF0);
		tft_write_8(0x09);
		tft_write_8(0x13);
		tft_write_8(0x12);
		tft_write_8(0x12);
		tft_write_8(0x2B);
		tft_write_8(0x3C);
		tft_write_8(0x44);
		tft_write_8(0x4B);
		tft_write_8(0x1B);
		tft_write_8(0x18);
		tft_write_8(0x17);
		tft_write_8(0x1D);
		tft_write_8(0x21);

		tft_writeCommand(0xE1);
		tft_write_8(0xF0);
		tft_write_8(0x09);
		tft_write_8(0x13);
		tft_write_8(0x0C);
		tft_write_8(0x0D);
		tft_write_8(0x27);
		tft_write_8(0x3B);
		tft_write_8(0x44);
		tft_write_8(0x4D);
		tft_write_8(0x0B);
		tft_write_8(0x17);
		tft_write_8(0x17);
		tft_write_8(0x1D);
		tft_write_8(0x21);

		tft_writeCommand(0xF0);
		tft_write_8(0x3C);

		tft_writeCommand(0xF0);
		tft_write_8(0x69);

		tft_writeCommand(0x13);

		tft_writeCommand(0x11);
		HAL_Delay(150);

		tft_writeCommand(0x29); // Display ON
		HAL_Delay(150);

    tft_endWrite();
}


void my_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);

	uint16_t * buf16 = (uint16_t *)px_map;

	tft_startWrite();
	tft_setAddrWindow(area->x1, area->y1, w, h);
	tft_writePixels(buf16, w * h);
	tft_endWrite();

	lv_display_flush_ready(display);
}


void tft_startWrite(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, RESET);
}


void tft_endWrite(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, SET);
}


void tft_DC_HIGH()
{
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, SET);
}


void tft_DC_LOW()
{
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, RESET);
}


void tft_write_8(uint16_t * word)
{
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	HAL_SPI_Init(&hspi1);

	HAL_SPI_Transmit_DMA(&hspi1, (uint16_t*)&word, 1);
}


void tft_writePixels_16(uint16_t * data_in, uint32_t len)
{
	hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
	HAL_SPI_Init(&hspi1);

	HAL_SPI_Transmit_DMA(&hspi1, data_in, len);
	while (hspi1.hdmatx->State != HAL_DMA_STATE_READY);
}


void tft_writeCommand(uint16_t cmd)
{
    tft_DC_LOW();
    tft_write_8(cmd);
    tft_DC_HIGH();
}


void tft_setRotation(uint8_t m)
{
    _rotation = m % 4;

    tft_startWrite();
    tft_writeCommand(ST7796S_MADCTL);

    switch (_rotation) {
        case 0:
            tft_write_8(MADCTL_MX | MADCTL_BGR);
            _width  = TFT_WIDTH;
            _height = TFT_HEIGHT;
            break;
        case 1:
        	tft_write_8(MADCTL_MV | MADCTL_BGR);
            _width  = TFT_HEIGHT;
            _height = TFT_WIDTH;
            break;
        case 2:
        	tft_write_8(MADCTL_MY | MADCTL_BGR);
            _width  = TFT_WIDTH;
            _height = TFT_HEIGHT;
            break;
        case 3:
        	tft_write_8(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            _width  = TFT_HEIGHT;
            _height = TFT_WIDTH;
            break;
    }
    tft_endWrite();
}


void tft_setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{

	tft_writeCommand(ST7796S_CASET); // Column addr set
	tft_write_8(x >> 8);
    tft_write_8(x & 0xFF);     // XSTART
    w=x+w-1;
    tft_write_8(w >> 8);
    tft_write_8(w & 0xFF);     // XEND

    tft_writeCommand(ST7796S_PASET); // Row addr set
    tft_write_8(y >> 8);
    tft_write_8(y & 0xFF);     // YSTART
    h=y+h-1;
    tft_write_8(h >> 8);
    tft_write_8(h & 0xFF);     // YEND
    tft_writeCommand(ST7796S_RAMWR);    //new jpv

}


void tft_writePixels(uint16_t * colors, uint32_t len)
{
	tft_writePixels_16(colors , len);
}


//////////////////////////////////////////////////////////////////////////////////////
// Touch panel Functions FT6336
//////////////////////////////////////////////////////////////////////////////////////

void tp_init()
{
	// Initialize the (dummy) input device driver //
	lv_indev_t * indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, touchpad_read_cb);

	// Touch init config //
	tpTxBuf[0] = FT6336_DEV_MODE;
	tpTxBuf[1] = 0x00;
	HAL_I2C_Master_Transmit_DMA(&hi2c4, 0x70 , tpTxBuf, 2);

	tpTxBuf[0] = FT6336_THRESHOLD;
	tpTxBuf[1] = 0x14;
	HAL_I2C_Master_Transmit_DMA(&hi2c4, 0x70 , tpTxBuf, 2);

	tpTxBuf[0] = FT6336_PRD_ATCIVE;
	tpTxBuf[1] = 0x0E;
	HAL_I2C_Master_Transmit_DMA(&hi2c4, 0x70 , tpTxBuf, 2);

	tpTxBuf[0] = FT6336_G_MODE;
	tpTxBuf[1] = 0x00;
	HAL_I2C_Master_Transmit_DMA(&hi2c4, 0x70 , tpTxBuf, 2);
}

tp_status tp_read()
{
	tp_status tpData;
	uint8_t reg = 0x00;

	HAL_I2C_Mem_Read_DMA(&hi2c4, 0x70, reg, 1, &tpRxBuf, 7);

	tpData.tp_y = ((tpRxBuf[3] & 0x0F) << 8) + tpRxBuf[4];
	tpData.tp_x = ((tpRxBuf[5] & 0x0F) << 8) + tpRxBuf[6];
	tpData.tp_act = tpRxBuf[2];

	return tpData;
}

void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
	tpData = tp_read();

	if (tpData.tp_act == 1)
	{
		data->point.x = tpData.tp_x;
		data->point.y = MY_DISP_VER_RES - tpData.tp_y;

		data->state = LV_INDEV_STATE_PRESSED;

		touchSleepFlg = true;
	}
	else
	{
		data->state = LV_INDEV_STATE_RELEASED;

		// Printar nomes si esta a CAN RX //
		if(lv_tabview_get_tab_active(tabview) == 0)
			  touchFlg = false;
	}
}

