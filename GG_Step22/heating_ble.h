#pragma once
/*
 * heating_ble.h — Chauffage diesel BLE (Nordkapp/Hcalory/AirHeater/Vevor)
 * 
 * Protocoles supportés : AA55 et ABBA
 * Ces chauffages sont tous des clones chinois avec firmware proprietaire BLE.
 * 
 * Basé sur reverse-engineering :
 *   - github.com/mSoftMS/AirHeater-BLE
 *   - github.com/Xev/homeassistant-diesel-heater
 *   - github.com/warehog/esphome-diesel-heater-ble
 */
#include <Arduino.h>
#include <stdint.h>

struct HeatingData {
    bool     valid;
    uint32_t last_update_ms;
    
    // Etat
    bool     running;             // moteur en marche
    uint8_t  running_state;       // 0=off, 1=start, 2=preheat, 3=heating, 4=cooldown, ...
    uint8_t  running_step;        // sous-état
    uint8_t  running_mode;        // 0=level, 1=temperature
    uint8_t  error_code;          // 0=ok
    
    // Températures
    float    room_temp_c;         // température ambiante capteur externe
    float    case_temp_c;         // température boîtier (chambre combustion)
    
    // Réglages
    uint8_t  set_level;           // niveau de puissance 1-10
    uint8_t  set_temp_c;          // consigne °C si mode temp
    
    // Mesures
    float    supply_voltage;      // V alimentation 12V
    uint16_t altitude_m;          // altitude (certains modèles)
    uint16_t fuel_pump_hz;        // fréquence pompe carburant (x0.1 Hz)
    uint16_t fan_rpm;             // vitesse ventilateur
};

extern HeatingData heatingData;

// API
void heating_init();

// On-demand (comme JKBMS)
void heating_request_activate();
void heating_request_deactivate();

void heating_update();              // appelé par ble_task (Core 0)
bool heating_is_connected();
bool heating_is_available();
const char* heating_state_str();

// Commandes
void heating_set_power(bool on);              // ON/OFF
void heating_set_level(uint8_t level_1_10);   // niveau 1-10
void heating_set_temp(uint8_t temp_c);        // consigne 8-36°C
void heating_set_mode(uint8_t mode);          // 0=level, 1=temp
