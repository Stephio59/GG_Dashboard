#pragma once
/*
 * jkbms_ble.h — Client BLE GATT JK-BMS ON-DEMAND
 *
 * Mode de fonctionnement :
 *   - Connexion UNIQUEMENT quand on appelle jkbms_request_activate()
 *   - Se déconnecte automatiquement quand jkbms_request_deactivate()
 *   - Les pages UI appellent activate en entrant et deactivate en sortant
 *
 * Avantages :
 *   - Pas de bruit radio en permanence
 *   - Pas d'impact sur l'UI hors de la page Énergie
 *   - Économie d'énergie batterie
 */
#include <Arduino.h>
#include <stdint.h>

struct JkBmsData {
    bool     valid;
    uint32_t last_update_ms;

    int      cell_count;
    float    cells_v[32];
    float    cell_v_avg;
    float    cell_v_diff;
    int      cell_v_max_idx;
    int      cell_v_min_idx;

    float    voltage;
    float    current;
    float    power;
    float    soc_pct;
    float    capacity_total_ah;
    float    capacity_remain_ah;
    uint32_t cycle_count;
    float    cycle_capacity_ah;

    float    temp_mos_c;
    float    temp_t1_c;
    float    temp_t2_c;

    bool     charging_on;
    bool     discharging_on;
    bool     balancing_on;
    float    balance_current;

    uint32_t uptime_s;
};

extern JkBmsData jkbmsData;

// API
void jkbms_init();
void jkbms_update();           // appelé par ble_task sur Core 0

// Demande active/deactive depuis l'UI (Core 1)
void jkbms_request_activate();    // = je veux des données
void jkbms_request_deactivate();  // = plus besoin, déconnecte

// Status
bool jkbms_is_active_requested();
bool jkbms_is_connected();
bool jkbms_is_available();
const char* jkbms_state_str();    // pour debug UI
