#pragma once
/*
 * ble_scanner.h — Scanner BLE simple pour GG Van Dashboard
 * 
 * Scan passif périodique et stocke les appareils détectés
 * Affiché dans page Système pour valider les MAC
 * 
 * Dépendance : NimBLE-Arduino
 */
#include <Arduino.h>
#include <stdint.h>

#define MAX_BLE_DEVICES        24
#define MAC_STR_LEN            18      // "XX:XX:XX:XX:XX:XX\0"
#define NAME_MAX_LEN           24

// Constantes de temps (ms)
#define BLE_SCAN_INTERVAL_MS   30000   // Toutes les 30s
#define BLE_SCAN_DURATION_S    5       // 5s de scan passif
#define BLE_DEVICE_TIMEOUT_MS  60000   // 60s sans signal = perdu

struct BleDeviceInfo {
    char     mac[MAC_STR_LEN];
    char     name[NAME_MAX_LEN];
    int8_t   rssi;
    bool     is_known;
    const char* known_label;
    uint32_t last_seen_ms;
};

struct BleScannerState {
    BleDeviceInfo devices[MAX_BLE_DEVICES];
    int           count;
    uint32_t      total_scans;
    uint32_t      last_scan_ms;
    bool          initialized;
};

extern BleScannerState bleScanState;

// API
void ble_scanner_init();
void ble_scanner_update();
