#pragma once
/*
 * water_state.h — État des eaux du van
 * 
 * Les données proviendront via ESP-NOW depuis le PCB ESP32-U
 *   - Eau propre (réservoir frais) : GPIO 34 du PCB (ADC via sonde CBE)
 *   - Eau sale/grise (réservoir usées) : GPIO 35 du PCB (ADC via sonde CBE)
 *   - Pompe eau : relais 7 du PCB
 *   - Chauffe-eau : relais 8 du PCB
 * 
 * Pour l'instant : valeurs simulées (démo)
 */
#include <Arduino.h>
#include <stdint.h>

struct WaterState {
    bool     valid;
    uint32_t last_update_ms;
    
    // Niveaux en %
    float    clean_pct;       // eau propre (fresh)
    float    dirty_pct;       // eau sale (grey)
    
    // Volumes (en litres, calculés d'après capacités configurables)
    float    clean_liters;
    float    dirty_liters;
    
    // Capacités en litres (config)
    float    clean_capacity_l;
    float    dirty_capacity_l;
    
    // Pompe et chauffe-eau
    bool     pump_on;         // pompe eau (relais 7)
    bool     water_heater_on; // chauffe-eau (relais 8)
    
    // Température eau chaude
    float    water_temp_c;
};

extern WaterState waterState;

void water_init();
void water_update();              // simulation démo en attendant ESP-NOW

// Callbacks pour UI (en attendant ESP-NOW)
void water_set_pump(bool on);
void water_set_heater(bool on);
