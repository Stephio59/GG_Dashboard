/*
 * icons_van.h - Police d'icones FontAwesome (taille 22 uniquement)
 * Utilisee uniquement pour les ICONES STATUS BAR (BLE, etc.)
 * Les autres icones sont charges depuis la SD (BMP)
 */
#pragma once

#include <lvgl.h>

LV_FONT_DECLARE(lv_font_icons_22);

// Macros pour facilite (police 22 seulement)
#define ICON_FONT_22  lv_font_icons_22

// Codes UTF-8 utilises (status bar)
#define ICON_BLUETOOTH    "\xEF\x8A\x93"  // bluetooth
