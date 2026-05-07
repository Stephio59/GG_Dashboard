/*
 * history.cpp — Implementation buffers circulaires
 */
#include "history.h"
#include "victron_ble.h"
#include "bme_wire1.h"
#include "sd_card.h"
#include "SD_MMC.h"
#include <time.h>
#include <string.h>

HistorySerie hist_soc        = {};
HistorySerie hist_voltage    = {};
HistorySerie hist_pv_power   = {};
HistorySerie hist_load_power = {};
HistorySerie hist_temp       = {};

static uint32_t _last_record_ms = 0;

static void serie_init(HistorySerie& s, float scale) {
    for (int i = 0; i < HIST_POINTS; i++) {
        s.data[i] = HIST_NO_DATA;
    }
    s.count = 0;
    s.head  = 0;
    s.scale = scale;
}

static void serie_push(HistorySerie& s, float value) {
    int16_t scaled;
    if (isnan(value) || value == HIST_NO_DATA) {
        scaled = HIST_NO_DATA;
    } else {
        scaled = (int16_t)(value * s.scale);
    }
    s.data[s.head] = scaled;
    s.head = (s.head + 1) % HIST_POINTS;
    if (s.count < HIST_POINTS) s.count++;
}

void history_init() {
    serie_init(hist_soc,        10.0f);   // 0.1%
    serie_init(hist_voltage,    100.0f);  // 0.01V
    serie_init(hist_pv_power,   1.0f);    // 1W
    serie_init(hist_load_power, 1.0f);    // 1W
    serie_init(hist_temp,       10.0f);   // 0.1°C
    _last_record_ms = millis();
    Serial0.println("[History] Init OK (5 series x 144 points = 24h)");
}

void history_update() {
    uint32_t now = millis();
    if (now - _last_record_ms < HIST_INTERVAL_MS) return;
    _last_record_ms = now;
    
    // SOC
    if (victronBmvData.valid) {
        serie_push(hist_soc, victronBmvData.soc_pct);
        serie_push(hist_voltage, victronBmvData.voltage);
        // Conso : courant negatif * tension (decharge)
        float load = (victronBmvData.current < 0) 
                   ? (-victronBmvData.current * victronBmvData.voltage)
                   : 0.0f;
        serie_push(hist_load_power, load);
    } else {
        serie_push(hist_soc, NAN);
        serie_push(hist_voltage, NAN);
        serie_push(hist_load_power, NAN);
    }
    
    // PV power
    if (victronMpptData.valid) {
        serie_push(hist_pv_power, victronMpptData.power_pv_w);
    } else {
        serie_push(hist_pv_power, NAN);
    }
    
    // Temperature interieure
    if (bmeData.valid) {
        serie_push(hist_temp, bmeData.temperature);
    } else {
        serie_push(hist_temp, NAN);
    }
    
    Serial0.printf("[History] Snapshot pris (count=%d)\n", hist_soc.count);
    
    // ── Sauvegarde sur SD : 1 fichier CSV par jour ──
    if (sdCard.mounted) {
        // Recuperer la date NTP
        time_t t = time(nullptr);
        if (t > 8 * 3600) {  // si NTP synchronise
            struct tm tm_info;
            localtime_r(&t, &tm_info);
            
            char path[48];
            snprintf(path, sizeof(path), "/logs/%04d-%02d-%02d.csv",
                     tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
            
            // Creer le dossier /logs si necessaire (premiere fois)
            if (!SD_MMC.exists("/logs")) {
                SD_MMC.mkdir("/logs");
            }
            
            // Si le fichier n'existe pas, ecrire le header
            if (!SD_MMC.exists(path)) {
                sd_card_write_file(path, "time,soc,v_bat,i_bat,solar_w,temp_int\n");
            }
            
            // Ajouter la ligne
            char line[128];
            float v = victronBmvData.valid ? victronBmvData.voltage : 0;
            float soc = victronBmvData.valid ? victronBmvData.soc_pct : 0;
            float i = victronBmvData.valid ? victronBmvData.current : 0;
            float pv = victronMpptData.valid ? victronMpptData.power_pv_w : 0;
            float ti = bmeData.valid ? bmeData.temperature : 0;
            
            snprintf(line, sizeof(line), "%02d:%02d,%.0f,%.2f,%.2f,%.0f,%.1f\n",
                     tm_info.tm_hour, tm_info.tm_min, soc, v, i, pv, ti);
            
            sd_card_append_file(path, line);
            Serial0.printf("[History-SD] -> %s\n", path);
        }
    }
}

int history_get_ordered(const HistorySerie& s, int16_t* out, int max_n) {
    if (s.count == 0) return 0;
    int n = (s.count < max_n) ? s.count : max_n;
    
    // Le plus ancien est a (head - count + HIST_POINTS) % HIST_POINTS
    int start = (s.head + HIST_POINTS - s.count) % HIST_POINTS;
    for (int i = 0; i < n; i++) {
        out[i] = s.data[(start + i) % HIST_POINTS];
    }
    return n;
}

// ────────────────────────────────────────────────────────
// Charger l'historique du jour depuis la SD
// Appele au boot apres sd_card_init() pour pre-remplir les graphes
// ────────────────────────────────────────────────────────

// Helper : push une valeur (deja scaled) dans une serie
static void serie_push_scaled(HistorySerie& s, int16_t v_scaled) {
    s.data[s.head] = v_scaled;
    s.head = (s.head + 1) % HIST_POINTS;
    if (s.count < HIST_POINTS) s.count++;
}

void history_load_from_sd() {
    if (!sdCard.mounted) {
        Serial0.println("[History-SD] SD non montee, pas de chargement");
        return;
    }
    
    // Trouver le fichier du jour
    time_t t = time(nullptr);
    if (t < 8 * 3600) {
        Serial0.println("[History-SD] NTP pas encore synchro, charge plus tard");
        return;
    }
    
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    
    char path[48];
    snprintf(path, sizeof(path), "/logs/%04d-%02d-%02d.csv",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
    
    if (!SD_MMC.exists(path)) {
        Serial0.printf("[History-SD] Pas de log pour aujourd'hui (%s)\n", path);
        return;
    }
    
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial0.printf("[History-SD] Echec ouverture %s\n", path);
        return;
    }
    
    Serial0.printf("[History-SD] Chargement %s...\n", path);
    
    // Format CSV : time,soc,v_bat,i_bat,solar_w,temp_int
    // ex: "14:30,85,13.18,-2.4,180,21.5"
    
    String line;
    int line_num = 0;
    int loaded = 0;
    
    while (f.available()) {
        line = f.readStringUntil('\n');
        line.trim();
        line_num++;
        
        // Skip header
        if (line_num == 1 && line.startsWith("time,")) continue;
        if (line.length() == 0) continue;
        
        // Parser les 6 champs
        // time, soc, v_bat, i_bat, solar_w, temp_int
        char buf[128];
        line.toCharArray(buf, sizeof(buf));
        
        char* tok = strtok(buf, ",");
        if (!tok) continue;
        // tok = time, on l'ignore
        
        tok = strtok(NULL, ",");
        if (!tok) continue;
        float soc = atof(tok);
        
        tok = strtok(NULL, ",");
        if (!tok) continue;
        float v_bat = atof(tok);
        
        tok = strtok(NULL, ",");
        if (!tok) continue;
        float i_bat = atof(tok);  // negatif = decharge
        
        tok = strtok(NULL, ",");
        if (!tok) continue;
        float solar_w = atof(tok);
        
        tok = strtok(NULL, ",");
        if (!tok) continue;
        float temp_int = atof(tok);
        
        // Pousser dans les series (formats compatibles avec history_update)
        serie_push_scaled(hist_soc,        (int16_t)(soc * 10));
        serie_push_scaled(hist_voltage,    (int16_t)(v_bat * 100));
        serie_push_scaled(hist_pv_power,   (int16_t)solar_w);
        // Conso (W) ~ -i_bat * v_bat si decharge, sinon 0
        float load_w = (i_bat < 0) ? (-i_bat * v_bat) : 0;
        serie_push_scaled(hist_load_power, (int16_t)load_w);
        serie_push_scaled(hist_temp,       (int16_t)(temp_int * 10));
        loaded++;
    }
    
    f.close();
    
    Serial0.printf("[History-SD] %d snapshots charges depuis CSV (sur %d lignes)\n", loaded, line_num - 1);
}
