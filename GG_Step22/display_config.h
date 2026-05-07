#pragma once
/*
 * display_config.h - Configuration ecran Waveshare ESP32-S3-Touch-LCD-7B
 * Resolution : 1024x600 (ST7701) — Tactile capacitif GT911
 * Cible unique : DISPLAY_WS7_1024x600
 */

#define DISPLAY_WS7_1024x600

// ─── Ecran ───────────────────────────────────────────────────────────────────
#define DISPLAY_NAME          "WS7B 1024x600"
#define SCREEN_WIDTH          1024
#define SCREEN_HEIGHT         600
#define SCREEN_ROTATION       0          // Landscape natif
#define DISPLAY_PIXEL_COUNT   (1024*600) // 614 400 pixels

#define USE_WS7_ST7701_DRIVER 1
#define HAS_PSRAM             1
#define PSRAM_SIZE_KB         8192       // 8 MB PSRAM
#define HAS_CAPACITIVE_TOUCH  1          // GT911 5 points I2C
#define TOUCH_I2C_SDA         8
#define TOUCH_I2C_SCL         9

// ─── Layout ──────────────────────────────────────────────────────────────────
#define STATUS_BAR_HEIGHT     36
#define NAV_BAR_HEIGHT        0          // Dashboard unique, pas de nav bar
#define NEED_PAGE_NAVIGATION  0
#define SHOW_DASHBOARD_VIEW   1

// ─── Fonts ───────────────────────────────────────────────────────────────────
#define FONT_SIZE_TITLE       32
#define FONT_SIZE_VALUE       48
#define FONT_SIZE_UNIT        22
#define FONT_SIZE_SMALL       16
#define FONT_SIZE_TINY        12

// ─── Grille ──────────────────────────────────────────────────────────────────
#define CARD_RADIUS           20
#define CARD_PADDING          18
#define GRID_COLS             4          // 4 colonnes sur 1024px
#define GRID_GAP              16

// ─── LVGL buffer (en PSRAM) ──────────────────────────────────────────────────
#define LVGL_DRAW_BUF_LINES   80
#define LVGL_BUF_SIZE         (1024*80*2)  // ~160 KB

// ─── BME280 I2C ──────────────────────────────────────────────────────────────
#define BME280_SDA_PIN            21
#define BME280_SCL_PIN            22
#define BME280_I2C_ADDR           0x76
#define BME_FREEZE_TEMP_C         2.0f
#define BME_HIGH_TEMP_C           35.0f
#define BME_HIGH_HUMIDITY_PCT     70.0f
#define BME_LOW_PRESSURE_HPA      980.0f
#define BME_CONDENSATION_MARGIN_C 3.0f

// ─── Macros de mise a l'echelle automatique ──────────────────────────────────
#define CONTENT_WIDTH   SCREEN_WIDTH
#define CONTENT_HEIGHT  (SCREEN_HEIGHT - STATUS_BAR_HEIGHT - NAV_BAR_HEIGHT)
#define SCALE_X  (((float)SCREEN_WIDTH)  / 480.0f)   // 1024/480 = 2.133x
#define SCALE_Y  (((float)SCREEN_HEIGHT) / 320.0f)   // 600/320  = 1.875x
#define SX(val)  ((lv_coord_t)((val) * SCALE_X))
#define SY(val)  ((lv_coord_t)((val) * SCALE_Y))
#define COL_WIDTH  ((CONTENT_WIDTH - (GRID_COLS-1)*GRID_GAP) / GRID_COLS)
#define COL(idx)   ((idx) * (COL_WIDTH + GRID_GAP))