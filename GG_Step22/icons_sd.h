/*
 * icons_sd.h - Pack d'icones charge depuis la SD au boot
 */
#pragma once
#include <lvgl.h>

// Charge les icones depuis /icons/*.bmp dans des descripteurs LVGL
// A appeler apres sd_lvgl_fs_init()
// Retourne true si toutes les icones essentielles sont chargees
bool icons_sd_load_all();

// Retourne le descripteur d'une icone (ou nullptr si pas chargee)
// Noms valides : "home", "energy", "heater", "lights", "settings",
//                "history", "gps", "temp", "weather", "van",
//                "water_clean", "water_dirty", "battery",
//                "wifi", "bluetooth", "solar"
const lv_img_dsc_t* icons_sd_get(const char* name);
