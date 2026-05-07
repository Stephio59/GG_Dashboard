/*
 * user_config.h - Configuration utilisateur via JSON sur SD
 * 
 * Au boot, charge /config.json depuis la SD si present.
 * Sinon, utilise les valeurs par defaut (compilees) et CREE le fichier.
 * 
 * Structure JSON :
 * {
 *   "wifi_ssid_1": "TP-Link_92E9",
 *   "wifi_pass_1": "stephio59",
 *   "wifi_ssid_2": "Freebox-5C6D45",
 *   "wifi_pass_2": "...",
 *   "owm_api_key": "...",
 *   "owm_city": "Dijon,FR",
 *   "screen_dim_s": 90,
 *   "screen_off_s": 120,
 *   "ble_scan_s": 5
 * }
 */
#pragma once
#include <Arduino.h>

struct UserConfig {
    // === WiFi ===
    char wifi_ssid_1[40];
    char wifi_pass_1[40];
    char wifi_ssid_2[40];
    char wifi_pass_2[40];
    
    // === OpenWeatherMap ===
    char owm_api_key[40];
    char owm_city[40];
    
    // === Veille ecran ===
    uint32_t screen_dim_s;
    uint32_t screen_off_s;
    uint32_t ble_scan_s;
    
    // === BLE - JKBMS Batterie ===
    char jkbms_mac[20];           // ex: "C8:47:80:01:CD:31"
    
    // === BLE - Chauffage Nordkapp ===
    char heating_mac[20];         // ex: "C1:02:29:4F:FE:50"
    
    // === BLE - Victron MPPT 100/30 ===
    char victron_mppt_mac[20];
    char victron_mppt_bindkey[40]; // 32 hex chars
    
    // === BLE - Victron BMV / SmartShunt ===
    char victron_bmv_mac[20];
    char victron_bmv_bindkey[40];
    
    // === BLE - Victron BatteryProtect ===
    char victron_bp_mac[20];
    char victron_bp_bindkey[40];
    
    // === BLE - Victron Orion DC/DC ===
    char victron_orion_mac[20];
    char victron_orion_bindkey[40];
    
    // === ESP-NOW slave MAC (PCB 30 pins) ===
    char espnow_slave_mac[20];    // ex: "88:57:21:BF:33:30"
    
    bool loaded_from_sd;          // true si charge depuis SD
};

extern UserConfig userConfig;

// Charge config depuis /config.json sur SD
// Si fichier absent : utilise les defauts et CREE le fichier sur SD
// Retourne true si tout OK
bool config_load();

// Sauvegarde la config actuelle sur SD
bool config_save();
