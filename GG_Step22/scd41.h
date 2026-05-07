#pragma once
/*
 * scd41.h — Driver SCD41 (CO2) sur bus I2C Waveshare partagé
 * 
 * Adresse I2C : 0x62
 * Mesures : CO2 (ppm), Température (°C), Humidité (%)
 * Intervalle périodique : 5 secondes
 */
#include <stdint.h>
#include <stdbool.h>

struct Scd41Data {
    uint16_t co2_ppm;       // CO2 (ppm)
    float    temperature;   // °C
    float    humidity;      // %
    bool     valid;
    uint32_t lastUpdate;
};

extern Scd41Data scd41Data;
extern bool scd41Available;

void scd41_init();       // Init + start_periodic_measurement
void scd41_update();     // À appeler dans loop() (toutes 5s)
