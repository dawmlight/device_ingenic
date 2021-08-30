
#include <common.h>
#include <command.h>
#include <environment.h>
#include <asm/arch/gpio.h>

#ifdef CONFIG_OLED_ST7539SPI

#include <bmp_logo_data.h>
#include "jwt_log_battery.h"

static int g_bUsbConnect = 0;
#define OLED_BLK CONFIG_GPIO_OLED_BKL
#define OLED_RST CONFIG_GPIO_OLED_RST
#define OLED_CS1 CONFIG_GPIO_OLED_CS1
#define OLED_SCL CONFIG_GPIO_OLED_SCL
#define OLED_SDI CONFIG_GPIO_OLED_SDI
#define OLED_A0  CONFIG_GPIO_OLED_A0

/*
#define GPIO_OLED_RESET     GPIO_PB(4)          //žŽÎ»ÒýœÅ
#define GPIO_OLED_BKL       GPIO_PC(20)         //±³¹â¿ØÖÆÒýœÅ
#define GPIO_OLED_CMDDATA   GPIO_PB(20)         //ÃüÁî/ÊýŸÝÒýœÅ: ÉèÖÃÃüÁîµÄÊ±ºòÒªÇóµÍµçÆœ; ÉèÖÃÊýŸÝµÄÊ±ºòÒªÇóžßµçÆœ

#define OLED_RST GPIO_PB(4)
#define OLED_CS1 GPIO_PD(1)
#define OLED_SCL GPIO_PD(0)
#define OLED_SDI GPIO_PD(2)
#define OLED_A0  GPIO_PB(20)
*/

#define BLK(n)                  \
	gpio_direction_output(OLED_BLK, n)

#define RES(n)					\
	gpio_direction_output(OLED_RST, n)

#define CS1(n)					\
	gpio_direction_output(OLED_CS1, n)

#define A0(n)					\
	gpio_direction_output(OLED_A0, n)

#define SCL(n)					\
	gpio_direction_output(OLED_SCL, n)

#define SDI(n)					\
	gpio_direction_output(OLED_SDI, n)


#define delay mdelay

static inline int clk_delay(void)
{
	udelay(0);		/* clock cycle 60ns */
	return 0;
}

#define DISPLAY_

//=========================================================
static unsigned char local_bmp_192x8[192*8] = {
0,
};

/* 
unsigned char bmp_logo_bitmap[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00,
	0x18, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x02, 0x30, 0x00, 0x00, 0x00,
	0x68, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x01,
	0x88, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x01,
	0x08, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x04, 0x10, 0x00, 0x00, 0x00,
	0x88, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x03, 0x08, 0x0F, 0xFF, 0xF0, 0x00,
	0x85, 0x04, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x02, 0xF0, 0x00, 0x00, 0x20, 0x30,
	0x42, 0x03, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x02, 0x00, 0x00, 0x01, 0xC6, 0x2F,
	0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x03, 0x00, 0x7F, 0xFE, 0x05, 0xA0,
	0x00, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0xC0, 0x80, 0x00, 0x04, 0x9F,
	0xE0, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x21, 0x0F, 0xF8, 0x04, 0x40,
	0x10, 0x20, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00,
	0x00, 0x79, 0x10, 0x00, 0x01, 0xE0, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x42, 0xF0, 0x06, 0x04, 0x21,
	0xE0, 0x1C, 0x00, 0xFC, 0x7C, 0x38, 0x63, 0xF3,
	0xF1, 0x85, 0x11, 0xFE, 0x0E, 0x1C, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x81, 0x00, 0x01, 0x04, 0x26,
	0x00, 0x02, 0x00, 0x44, 0x44, 0x40, 0x12, 0x12,
	0x12, 0x02, 0x22, 0x01, 0x10, 0x02, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x80, 0x1F, 0x81, 0x06, 0x28,
	0x0F, 0x81, 0x00, 0x44, 0x44, 0x83, 0x09, 0x11,
	0x14, 0x00, 0x27, 0xE1, 0x20, 0xC2, 0x00, 0x01,
	0x80, 0x00, 0x01, 0x00, 0x60, 0x81, 0x01, 0x28,
	0x30, 0x41, 0x00, 0x44, 0x45, 0x04, 0x89, 0x11,
	0x14, 0x38, 0x20, 0x11, 0x21, 0x22, 0x00, 0x01,
	0x80, 0x00, 0x02, 0x01, 0x83, 0x01, 0x0C, 0xA8,
	0x4F, 0x81, 0x00, 0x44, 0x45, 0x08, 0x89, 0x11,
	0x14, 0x44, 0x27, 0xE1, 0x42, 0x22, 0x00, 0x01,
	0x80, 0x00, 0x04, 0x02, 0x1C, 0x01, 0x0A, 0x68,
	0x30, 0x01, 0x00, 0x44, 0x45, 0x10, 0x8A, 0x21,
	0x14, 0x82, 0x28, 0x01, 0x42, 0x22, 0x00, 0x01,
	0x80, 0x00, 0x08, 0x02, 0x60, 0x01, 0x11, 0x88,
	0x00, 0x01, 0x00, 0x48, 0x85, 0x10, 0x8A, 0x23,
	0x14, 0x84, 0x48, 0x61, 0x44, 0x22, 0x00, 0x01,
	0x80, 0x00, 0x10, 0xC1, 0x80, 0xC1, 0x10, 0x48,
	0x1F, 0xC1, 0x00, 0x48, 0x85, 0x11, 0x0A, 0x22,
	0x14, 0x84, 0x50, 0x91, 0x44, 0x22, 0x00, 0x01,
	0x80, 0x00, 0x21, 0x40, 0x0F, 0x21, 0x10, 0x50,
	0x20, 0x41, 0x00, 0x45, 0x09, 0x11, 0x0A, 0x14,
	0x24, 0x48, 0x51, 0x21, 0x44, 0x42, 0x00, 0x01,
	0x80, 0x00, 0x3E, 0x40, 0x30, 0x21, 0x10, 0x50,
	0x47, 0x82, 0x00, 0x42, 0x09, 0x0E, 0x12, 0x08,
	0x24, 0x30, 0x50, 0xC1, 0x43, 0x84, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x40, 0x47, 0xC1, 0x10, 0x50,
	0x38, 0x02, 0x00, 0x40, 0x09, 0x00, 0x21, 0x00,
	0x22, 0x00, 0x50, 0x01, 0x20, 0x0C, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x40, 0xB8, 0x01, 0x0C, 0x50,
	0x00, 0x02, 0x00, 0x40, 0x10, 0xC0, 0xC1, 0x86,
	0x12, 0x0C, 0x48, 0x71, 0x18, 0x30, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x81, 0x40, 0x00, 0x82, 0x50,
	0x0E, 0x02, 0x00, 0x3C, 0x10, 0x3F, 0x00, 0xF9,
	0xF1, 0xF3, 0xE7, 0x8F, 0x87, 0xC0, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x81, 0x40, 0x00, 0x62, 0x50,
	0x71, 0x02, 0x00, 0x82, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x82, 0x83, 0xC0, 0x12, 0x90,
	0x80, 0x84, 0x00, 0xFC, 0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x85, 0x04, 0x40, 0x0E, 0x90,
	0x80, 0x84, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x85, 0x08, 0x40, 0x00, 0x90,
	0x7F, 0x04, 0x08, 0x80, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x01, 0x09, 0x08, 0x40, 0x00, 0x50,
	0x00, 0x04, 0x18, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x01, 0x09, 0x04, 0x43, 0x00, 0x28,
	0x00, 0x0C, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x01, 0x08, 0x82, 0x44, 0xC0, 0x17,
	0xFF, 0xF0, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x02, 0x10, 0x81, 0xC4, 0x38, 0x08,
	0x00, 0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x02, 0x10, 0x60, 0x04, 0x06, 0x07,
	0xFF, 0xF8, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x04, 0x20, 0x18, 0x04, 0x01, 0xC0,
	0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x04, 0x20, 0x07, 0x84, 0x00, 0x38,
	0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x07, 0xC0, 0x00, 0x7C, 0x00, 0x07,
	0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
 */

/*
static unsigned char bmp1[]={
       0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,  
    0xF8,0x48,0xC8,0x88,0x40,0xB0,0x1C,0x10,0x90,0x70,0x00,0x40,0xE0,0x58,0x44,0xC8,0x10,0x00,0xF0,0x00,0x00,0xFC,0x00,0x00,  
    0xF0,0x50,0x50,0x50,0xFC,0x50,0x50,0x50,0xF8,0x10,0x00,0x80,0x84,0x84,0x84,0x84,0xE4,0xA4,0x94,0x8C,0xC4,0x80,0x00,0x08,  
    0x88,0x48,0xE8,0x38,0x2C,0x28,0x28,0xE8,0x08,0x08,0x00,0xFC,0x04,0x64,0x9C,0x00,0xFC,0x94,0x94,0x94,0xFC,0x00,0x00,0x80,  
    0x40,0x20,0x1C,0x00,0xC0,0x0C,0x30,0x40,0x80,0x80,0x00,0x20,0x24,0xA4,0xA4,0xA4,0xA4,0xB4,0x24,0x04,0xFC,0x00,0x00,0x00,  
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0F,0x08,0x0F,0x10,0x10,0x08,0x05,0x02,0x01,0x00,0x00,0x00,0x1F,0x10,0x12,0x13,  
    0x10,0x1C,0x03,0x10,0x10,0x1F,0x00,0x00,0x07,0x02,0x02,0x02,0x0F,0x12,0x12,0x12,0x13,0x18,0x00,0x00,0x00,0x00,0x10,0x10,  
    0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x1F,0x05,0x05,0x05,0x15,0x1F,0x00,0x00,0x00,0x1F,0x02,0x02,0x03,0x00,  
    0x1F,0x10,0x0B,0x04,0x0A,0x11,0x00,0x00,0x10,0x18,0x14,0x13,0x10,0x0A,0x0C,0x18,0x00,0x00,0x00,0x00,0x00,0x07,0x04,0x04,  
    0x04,0x07,0x10,0x10,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,  
    0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xFF};
*/



//========================================================


//========================================================
static void write_com(int para)
{
	int j;
	j=8;
	CS1(0);
	A0(0);
	clk_delay();
	do
	{
		if(para&0x80)
			SDI(1);
		else
			SDI(0);
		//clk_delay();
		SCL(0);
		clk_delay();
		//delay(2);
		SCL(1);
		--j;
		para<<=1;
		clk_delay();
	}
 	while(j);
	CS1(1);
	clk_delay();
}
//========================================================
static void write_data(int para)
{
	int j;
	j=8;
	CS1(0);
	A0(1);
	clk_delay();

	do
	{
		if(para&0x80)
			SDI(1);
		else
			SDI(0);
		SCL(0);
		clk_delay();
		//delay(2);
		SCL(1);
		clk_delay();
		--j;
		para<<=1;
	}
 	while(j);
	CS1(1);
	clk_delay();
}


//========================================================
static void clear_test(void)			//ÇåÆÁ
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
		write_com(i);
		write_com(0x10);
		write_com(0x00);
		for(j=0;j<192;j++)
		{
			if (j&1)
				write_data(0x00);
			else
				write_data(0xff);
			//mdelay(5);
		}
	}
}

//========================================================
static void clealddram(void)			//ÇåÆÁ
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
		write_com(i);
		write_com(0x10);
		write_com(0x00);
		for(j=0;j<192;j++)
		{
			write_data(0x00);
		}
	}
}

//========================================================
static void font1()				//È«ÏÔ
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(0xff);
	}
	}
}
//========================================================
static void font2()				//ºáÏß
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(0x55);
	}
	}
}
//========================================================
static void font3()				//ÊúÏß
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<96;j++)
	{
	    write_data(0xff);
	    write_data(0x00);
	}
	}
}
//========================================================
static void font4()				//Ñ©»¨
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<96;j++)
	{
	    write_data(0x55);
	    write_data(0xaa);
	}
	}
}
//========================================================
static void showpic(char *k)			//ÏÔÊ¾Í¼Æ¬
{
	int i,j;
	    for(i=0xb0;i<0xb8;i++)
	{
	    write_com(0xc4);		//MX=1,MY=1
	    //write_com(0x40);		//Set Scroll Line
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(*k++);
	}
	}
}

//========================================================
static void showpic2(char *k)			//ÏÔÊ¾Í¼Æ¬
{
	int i,j;
	for(i=0xb0;i<0xb9;i++)
	{
		//write_com(0xc4);		//MX=1,MY=1
		//write_com(0x40);		//Set Scroll Line
		write_com(i);
		write_com(0x10);
		write_com(0x00);
		for(j=0;j<192;j++)
		{
			write_data(*k++);
		}
	}
}


static int oled_init(void)
{
	int i, count;
	int bUsbConnect;
    	RES(0);
    	delay(2);
    	RES(1);
    	delay(20);  	

/*	
    	write_com(0xa0);		//FrameÆµÂÊ
    	write_com(0xeb);		//1/9 Bias
    	write_com(0x81);		//Set VOP
    	write_com(0x85);		//
    	write_com(0xc2);		//MX\MY
    	write_com(0x84);
    	write_com(0xf1);		//Duty
    	write_com(0x3f);		//64
*/    	
		write_com(0xe2);
    	write_com(0xa0);		//FrameÆµÂÊ
    	write_com(0xeb);		//1/9 Bias
    	write_com(0x81);		//Set VOP
    	write_com(0x96);		//
    	write_com(0xc4);		//MX\MY
    	write_com(0x84);
    	write_com(0xf1);		//Duty
    	write_com(0x3f);		//64
    	
    	//write_com(0xf2);		//
    	//write_com(0x10);		//
    	//write_com(0xf3);
    	//write_com(0x2f);		//
    	//write_com(0x85);		//

    	clealddram();
    	write_com(0xaf);		//Display on

		//�ж���usb
		#define USB_CONNECT_TRYMAX   (10)
		g_bUsbConnect = 0;
		gpio_direction_input(CONFIG_GPIO_USB_DETECT);
		for (i = count = 0; i < USB_CONNECT_TRYMAX; i++)
		{
			bUsbConnect = !!(CONFIG_GPIO_USB_DETECT_ENLEVEL == gpio_get_value(CONFIG_GPIO_USB_DETECT));
			if (bUsbConnect)
			{
				if (++count >= (USB_CONNECT_TRYMAX/2))
				{
					g_bUsbConnect = 1;
					break;
				}
			}
		}
		printf("usb:%d\n", g_bUsbConnect);


	return 0;
}


static int oled_test(void)
{
	do {
	printf("%s %d\n", __FUNCTION__, __LINE__);

		mdelay(2000);
		//showpic(bmp1);
		printf("%s %d local_bmp_192x8\n", __FUNCTION__, __LINE__);
		showpic2(&local_bmp_192x8[0]);

		mdelay(2000);
		clear_test();

		mdelay(2000);
		printf("%s %d bmp_logo_bitmap\n", __FUNCTION__, __LINE__);
		//showpic(bmp1);
		showpic2(&bmp_logo_bitmap[0]);

		mdelay(2000);
		clear_test();
#if 0
		mdelay(2000);
		font2();

		mdelay(2000);
		font3();

		mdelay(2000);
		font4();
#endif
	} while (1);
}

static int oled_gpio_init(void)
{

	RES(1);
	CS1(1);

	BLK(1); //close backlight

	return 0;
}

/*
  translate bmp logo (192/8)x64 to 192x(64/8)
*/
static int translate_bmp_buffer(void)
{
	unsigned char s[8];	/* src pixel */
	unsigned char d0;	/* dst pixel */
	unsigned char b[8];	/* tmp bit */
	int dh, dw, i;

	for (dh=0; dh<8; dh++) {
		for (dw=0; dw<192; dw++) {
			int shift, si;

			si = dw/8;
			shift = dw%8;
			shift = 8 - shift -1;

			for (i=0;i<8; i++) {
				if (g_bUsbConnect)
				{
					s[i] = bmp_jwt_battery_logo_bitmap[(dh*8+i)*(192/8) + si];
				}
				else
				{
					s[i] = bmp_logo_bitmap[(dh*8+i)*(192/8) + si];
				}
			}

			for (i=0;i<8; i++) {
				b[i] = (s[i]>> shift) & 0x1;
			}

			d0 = 0;
			for (i=0;i<8; i++) {
				d0 <<= 1;
				//d0 |= b[i];
				d0 |= b[8-i-1];
			}

			local_bmp_192x8[dh*192+dw] = d0;
			/*
			b0 = (s0>> shift) & 0x1;
			b1 = (s1>> shift) & 0x1;
			b2 = (s2>> shift) & 0x1;
			b3 = (s3>> shift) & 0x1;
			b4 = (s4>> shift) & 0x1;
			b5 = (s5>> shift) & 0x1;
			b6 = (s6>> shift) & 0x1;
			b7 = (s7>> shift) & 0x1;

			d0 = (b7<<7) | (b6<<6) | (b5<<5) | (b4<<4) | (b3<<3) | (b2<<2) | (b1<<1) | (b0<<0);
			*/
		}
	}

	return 0;
}

static int update_boot_reset_type(void * args)
{
	char * bootargs;
	char * boot_reset_type_str;
	char * type;
	/* int index; */
	/* index =0; */

	//bootargs = BOOTARGS_COMMON;
	/* bootargs = CONFIG_BOOTARGS; */
	/* printf("%s BOOTARGS_COMMON: %p\n", __FUNCTION__, BOOTARGS_COMMON); */
	/* printf("%s CONFIG_BOOTARGS: %p\n", __FUNCTION__, CONFIG_BOOTARGS); */
	/* printf("%s bootargs: %p\n", __FUNCTION__, bootargs); */

	/* printf("%s BOOTARGS_COMMON: %s\n", __FUNCTION__, BOOTARGS_COMMON); */
	/* printf("%s CONFIG_BOOTARGS: %s\n", __FUNCTION__, CONFIG_BOOTARGS); */

	/* printf("gd->bd->bi_boot_params: %p\n", gd->bd->bi_boot_params); */
	/* printf("gd->bd->bi_boot_params: %p, %s\n", gd->bd->bi_boot_params, gd->bd->bi_boot_params); */
	//bootargs = (char *)gd->bd->bi_boot_params;
	/* if (gd->flags & GD_FLG_RELOC) */
	/* 	bootargs = env_get_char_memory(index); */
	/* else */
	/* 	bootargs = env_get_char_init(index); */

	printf("default_environment= %p, %s\n", default_environment, default_environment);
	bootargs = default_environment;
	//bootargs = env_get_addr(index);
	//bootargs = getenv("bootargs");

//BOOTARGS_COMMON
#define BOOT_RESET_TYPE_ARGS "boot_reset_type="

	boot_reset_type_str = strstr(bootargs, BOOT_RESET_TYPE_ARGS);
	//printf("boot_reset_type_str= %p\n", boot_reset_type_str);
	if(boot_reset_type_str != NULL) {
		type = boot_reset_type_str + strlen(BOOT_RESET_TYPE_ARGS);
		/* printf("type= %p\n", type); */
		//printf("type= '%c'\n", *type);
		/* printf("type= '%c'\n", *(type+1)); */
		if (g_bUsbConnect)
			*type = '1';
		else
			*type = '0';
		/* printf("type= '%c'\n", *type); */
		/* printf("type= '%c'\n", *(type+1)); */
	}

	//printf("%s BOOTARGS_COMMON: %s\n", __FUNCTION__, BOOTARGS_COMMON);
	//printf("%s CONFIG_BOOTARGS: %s\n", __FUNCTION__, CONFIG_BOOTARGS);
	printf("default_environment= %p, %s\n", default_environment, default_environment);
	return 0;
}

int do_oled_logo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	oled_gpio_init();

	//printf("%s %d\n", __FUNCTION__, __LINE__);
	oled_init();

	translate_bmp_buffer();

	showpic2(&local_bmp_192x8[0]);
	BLK(0); //open backlight

	//oled_test();

	update_boot_reset_type(NULL);
	return 0;
}


U_BOOT_CMD(
	bootlogo,	4,	1,	do_oled_logo,
	"show boot logo",
	"show boot logo"
	);

#endif /* CONFIG_OLED_ST7539SPI */
