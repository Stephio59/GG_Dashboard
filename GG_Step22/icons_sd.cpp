/*
 * icons_sd.cpp - Pack d'icones charge depuis la SD au boot
 */
#include "icons_sd.h"
#include "sd_card.h"
#include "sd_lvgl_fs.h"
#include <Arduino.h>
#include <string.h>

#define MAX_ICONS 32

struct IconEntry {
    char name[24];
    lv_img_dsc_t* dsc;
};

static IconEntry _icons[MAX_ICONS];
static int _icon_count = 0;

static const char* ICON_NAMES[] = {
    // Nav
    "home", "energy", "heater", "lights",
    "settings", "history", "gps", "temp",
    "weather", "van", "water_clean", "water_dirty",
    "battery", "wifi", "bluetooth", "solar",
    // Meteo (7)
    "weather_sunny", "weather_partly_cloudy", "weather_cloudy",
    "weather_rain", "weather_storm", "weather_snow", "weather_fog",
    // Lumieres (8)
    "light_salon", "light_kitchen", "light_bedroom", "light_shower",
    "light_ext", "light_tv", "light_pump", "light_hotwater"
};
static const int ICON_NAMES_COUNT = sizeof(ICON_NAMES) / sizeof(ICON_NAMES[0]);

bool icons_sd_load_all() {
    if (!sdCard.mounted) {
        Serial0.println("[Icons-SD] SD non montee, pas de chargement");
        return false;
    }
    
    Serial0.println("[Icons-SD] Chargement icones...");
    
    int loaded = 0;
    for (int i = 0; i < ICON_NAMES_COUNT && _icon_count < MAX_ICONS; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/icons/%s.bmp", ICON_NAMES[i]);
        lv_img_dsc_t* dsc = sd_load_bmp(path);
        if (dsc) {
            strncpy(_icons[_icon_count].name, ICON_NAMES[i], sizeof(_icons[_icon_count].name)-1);
            _icons[_icon_count].name[sizeof(_icons[_icon_count].name)-1] = 0;
            _icons[_icon_count].dsc = dsc;
            _icon_count++;
            loaded++;
        }
    }
    
    Serial0.printf("[Icons-SD] %d/%d icones chargees\n", loaded, ICON_NAMES_COUNT);
    return loaded > 0;
}

const lv_img_dsc_t* icons_sd_get(const char* name) {
    if (!name) return nullptr;
    for (int i = 0; i < _icon_count; i++) {
        if (strcmp(_icons[i].name, name) == 0) {
            return _icons[i].dsc;
        }
    }
    return nullptr;
}
