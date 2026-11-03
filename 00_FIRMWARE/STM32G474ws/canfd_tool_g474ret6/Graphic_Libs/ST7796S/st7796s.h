
#ifndef LV_ST7796S_H
#define LV_ST7796S_H

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"


////////////////////////////////////////////////////////////////
// TFT ST7796S Registers
////////////////////////////////////////////////////////////////

#define TFT_WIDTH   480
#define TFT_HEIGHT  320

#define TFT_MAX_PIXELS_AT_ONCE  32

#define ST7796S_NOP 0x00     ///< No-op register
#define ST7796S_SWRESET 0x01 ///< Software reset register
#define ST7796S_RDDID 0x04   ///< Read display identification information
#define ST7796S_RDDST 0x09   ///< Read Display Status

#define ST7796S_SLPIN 0x10  ///< Enter Sleep Mode
#define ST7796S_SLPOUT 0x11 ///< Sleep Out
#define ST7796S_PTLON 0x12  ///< Partial Mode ON
#define ST7796S_NORON 0x13  ///< Normal Display Mode ON

#define ST7796S_RDMODE 0x0A     ///< Read Display Power Mode
#define ST7796S_RDMADCTL 0x0B   ///< Read Display MADCTL
#define ST7796S_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define ST7796S_RDIMGFMT 0x0D   ///< Read Display Image Format
#define ST7796S_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define ST7796S_INVOFF 0x20   ///< Display Inversion OFF
#define ST7796S_INVON 0x21    ///< Display Inversion ON
#define ST7796S_GAMMASET 0x26 ///< Gamma Set
#define ST7796S_DISPOFF 0x28  ///< Display OFF
#define ST7796S_DISPON 0x29   ///< Display ON

#define ST7796S_CASET 0x2A ///< Column Address Set
#define ST7796S_PASET 0x2B ///< Page Address Set
#define ST7796S_RAMWR 0x2C ///< Memory Write
#define ST7796S_RAMRD 0x2E ///< Memory Read

#define ST7796S_PTLAR 0x30    ///< Partial Area
#define ST7796S_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define ST7796S_MADCTL 0x36   ///< Memory Access Control
#define ST7796S_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define ST7796S_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left


////////////////////////////////////////////////////////////////
// TP registers
////////////////////////////////////////////////////////////////

#define FT6336_DEV_MODE 0x00      ///< 255, 255,   0
#define FT6336_THRESHOLD 0x80       ///< 255, 255, 255
#define FT6336_PRD_ATCIVE 0x88      ///< 255, 165,   0
#define FT6336_G_MODE 0xA4 ///< 173, 255,  41


////////////////////////////////////////////////////////////////
// TFT Functions
////////////////////////////////////////////////////////////////

void tft_write_8(uint16_t * word);
void tft_writePixels_16(uint16_t * data, uint32_t size);
void tft_init();
void tft_setRotation(uint8_t r);
void tft_writeCommand(uint16_t cmd);
void tft_setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void tft_writePixels(uint16_t * colors, uint32_t len);
void tft_startWrite(void);
void tft_endWrite(void);


////////////////////////////////////////////////////////////////
// Touch Panel
////////////////////////////////////////////////////////////////

typedef struct {
	int16_t tp_x;
    int16_t tp_y;
    int8_t tp_act;
} tp_status;


void tp_init();
tp_status tp_read();



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_INDEV_H*/









