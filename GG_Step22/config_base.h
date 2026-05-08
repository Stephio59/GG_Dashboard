#pragma once
// config_base.h - Constantes communes GG Van Dashboard
// ⚠️  Editer ce fichier avec vos propres valeurs avant compilation

// ─── WiFi ────────────────────────────────────────────────────────────────────
#define WIFI_SSID        "Tp-Link_92E9"
#define WIFI_PASSWORD    "stephio59"

// ─── NTP ─────────────────────────────────────────────────────────────────────
#define NTP_SERVER       "pool.ntp.org"
#define NTP_GMT_OFFSET   3600      // UTC+1 (France hiver)
#define NTP_DST_OFFSET   3600      // +1h ete

// ─── OpenWeatherMap ──────────────────────────────────────────────────────────
#define OWM_API_KEY      "11370682e8ddbaefaab02d8878a744a2"
#define OWM_CITY         "Paris,FR"
#define OWM_COUNTRY      "FR"
#define OWM_LANG         "fr"
#define OWM_UNITS        "metric"

// ─── BLE ─────────────────────────────────────────────────────────────────────
#include "bluetooth_config.h"

// Nombre de cellules batterie (8, 16, 24 ou 32)
#define JKBMS_CELL_COUNT 16

// ─── Relais GPIO lumieres (actif LOW) ────────────────────────────────────────
#define RELAY_LIGHT1    26      // Salon
#define RELAY_LIGHT2    27      // Cuisine
#define RELAY_LIGHT3    14      // Chambre
#define RELAY_LIGHT4    12      // WC
#define RELAY_LIGHT5    13      // Ext. Avant
#define RELAY_LIGHT6    33      // Ext. Arriere
#define RELAY_TV        33      // Television

#define RELAY_ON    LOW
#define RELAY_OFF   HIGH

// ─── Chauffage BLE ───────────────────────────────────────────────────────────
#define HEATING_TEMP_MIN     5.0f
#define HEATING_TEMP_MAX     30.0f
#define HEATING_TEMP_STEP    0.5f
#define HEATING_TEMP_DEFAULT 20.0f

// ─── Niveau Eau ──────────────────────────────────────────────────────────────
#define WATER_FRESH_ADC_PIN   34
#define WATER_WASTE_ADC_PIN   35
#define WATER_ADC_VREF        2.5f
#define WATER_ADC_RESOLUTION  4095
#define WATER_UPDATE_MS       5000

#define RELAY_WATER_PUMP    15
#define RELAY_WATER_HEATER  25

// ─── GPS NEO-6M ──────────────────────────────────────────────────────────────
// Module GNSS multi-constellations (GPS+GLONASS) sur header "GPIO" PH2.0 :
//   3V3 + GND + GP6 = GPIO 6 directement
//   Baud par defaut du module : 115200 (et non 9600 comme un NEO-6M classique)
//   Trames recues : $GNGGA, $GNRMC (preprefixe GN = Generic Navigation, multi-systemes)
// ⚠️  Conflit avec PIR (qui etait sur GPIO 6) : voir pir_sensor.h (#define ENABLE_PIR 0)
#define GPS_RX_PIN    6     // <- TX du GPS (jaune), header GPIO PH2.0 broche 3
#define GPS_TX_PIN    -1    // pas branche (le NEO n'a pas besoin d'etre configure)
#define GPS_BAUD      115200
#define GPS_UPDATE_MS 1000

// ─── Periodes de mise a jour ─────────────────────────────────────────────────
#define UPDATE_PERIOD_BLE_MS     5000
#define UPDATE_PERIOD_WEATHER_MS 600000   // 10 min
#define UPDATE_PERIOD_NTP_MS     3600000  // 1 heure
#define UI_BLE_UPDATE_MS         2000
#define UI_CLOCK_UPDATE_MS       1000