#pragma once
/*
 * bme_wire1.h — BME280 via driver I2C Waveshare (partage bus GPIO 8/9)
 * 
 * Utilise le même bus que GT911 touch et CH422G IO expander
 * Adresse BME280 : 0x76 (ou 0x77)
 */
#include <stdint.h>
#include <stdbool.h>

struct BmeData {
    float temperature;    // °C
    float humidity;       // %
    float pressure;       // hPa
    bool  valid;
    uint32_t lastUpdate;
};

extern BmeData bmeData;
extern bool bmeAvailable;

void bme_wire1_init();       // Initialise le capteur
void bme_wire1_update();     // À appeler dans loop() (lecture toutes les 2s)
