#pragma once
/*
 * config.h - Configuration principale GG Van Dashboard
 * Cible : Waveshare ESP32-S3-Touch-LCD-7B (1024x600)
 */

// Inclure la config ecran en premier (definit SCREEN_WIDTH, etc.)
#include "display_config.h"

// Inclure les constantes communes (WiFi, BLE, relais, GPIO...)
#include "config_base.h"

// ─── Options UI LVGL ─────────────────────────────────────────────────────────
#define UI_ANIM_WIFI_PERIOD_MS    120
#define UI_ANIM_BLE_PERIOD_MS     100
#define UI_ANIM_WEATHER_PERIOD_MS 150
#define UI_ANIM_HEAT_PERIOD_MS    100