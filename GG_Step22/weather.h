#pragma once
/*
 * weather.h — Meteo via OpenWeatherMap (3 jours actuels) + Open-Meteo (7 jours)
 */
#include <stdint.h>

struct WeatherDay {
    char    label[8];       // "AUJ", "DEM", "APR"
    float   temp_c;         // °C (actuel ou max)
    float   temp_min;       // °C min du jour (Open-Meteo)
    float   temp_max;       // °C max du jour (Open-Meteo)
    int     humidity_pct;
    float   wind_kmh;
    char    condition[32];
    char    icon[8];        // code OWM ("01d", "02n", ...) compatible avec icones SD
    float   rain_mm;        // mm de pluie prevue (Open-Meteo)
};

struct WeatherData {
    // Court terme (OWM, garde l'ancienne logique)
    WeatherDay today;
    WeatherDay tomorrow;
    WeatherDay after;
    
    // 7 jours (Open-Meteo) - nouveau
    WeatherDay days7[7];
    bool valid7days;
    
    bool valid;
    uint32_t lastUpdate;
};

extern WeatherData weatherData;

void weather_update();          // OWM (3j)
void weather_update_7days();    // Open-Meteo (7j)
