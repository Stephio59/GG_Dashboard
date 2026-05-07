#include "theme_mode.h"
#include <Arduino.h>
#include <time.h>

ThemeMode current_theme = THEME_NIGHT;

bool theme_update_auto() {
    time_t now = time(nullptr);
    if (now < 8 * 3600) return false;  // pas encore synchro NTP
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int hour = timeinfo.tm_hour;
    
    ThemeMode wanted = (hour >= 7 && hour < 21) ? THEME_DAY : THEME_NIGHT;
    
    if (wanted != current_theme) {
        current_theme = wanted;
        Serial0.printf("[Theme] Switch -> %s (heure: %d:%02d)\n",
                       wanted == THEME_DAY ? "JOUR" : "NUIT",
                       hour, timeinfo.tm_min);
        return true;
    }
    return false;
}

void theme_set(ThemeMode mode) {
    current_theme = mode;
}
