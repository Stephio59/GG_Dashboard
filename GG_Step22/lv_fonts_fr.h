#pragma once
/*
 * lv_fonts_fr.h — Polices Montserrat custom avec accents francais + degre + symboles
 * 
 * Contient :
 *  - ASCII 0x20-0x7F
 *  - Latin-1 0xA0-0xFF (°, é, è, à, ç, î, etc.)
 *  - Puce •
 *  - FontAwesome : OK, CLOSE, HOME, WIFI, BLUETOOTH, BELL, WARNING, CHARGE,
 *                  SETTINGS, TINT, GPS, EYE_OPEN, EYE_CLOSE, REFRESH, IMAGE, ...
 */
#include <lvgl.h>

LV_FONT_DECLARE(lv_font_fr_14);
LV_FONT_DECLARE(lv_font_fr_16);
LV_FONT_DECLARE(lv_font_fr_18);
LV_FONT_DECLARE(lv_font_fr_20);
LV_FONT_DECLARE(lv_font_fr_22);
LV_FONT_DECLARE(lv_font_fr_28);
LV_FONT_DECLARE(lv_font_fr_36);
LV_FONT_DECLARE(lv_font_fr_48);

// Alias pratiques
#define FONT_14  lv_font_fr_14
#define FONT_16  lv_font_fr_16
#define FONT_18  lv_font_fr_18
#define FONT_20  lv_font_fr_20
#define FONT_22  lv_font_fr_22
#define FONT_28  lv_font_fr_28
#define FONT_36  lv_font_fr_36
#define FONT_48  lv_font_fr_48
