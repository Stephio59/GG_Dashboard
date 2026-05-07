#pragma once
/*
 * history.h — Buffers circulaires pour graphiques historiques 24h
 * 
 * 144 points = 1 mesure toutes les 10 minutes sur 24 heures
 * Stockage en RAM (perdu au reset, mais OK pour un dashboard van)
 * 
 * Series :
 *   - SOC batterie (%)
 *   - Tension batterie (V)
 *   - Production solaire (W)
 *   - Conso (W) sortie batterie
 *   - Temperature interieure (°C)
 */
#include <Arduino.h>
#include <stdint.h>

#define HIST_POINTS         48          // 24h × 2 points/h = 48 (snapshot toutes 30 min)
#define HIST_INTERVAL_MS    1800000UL   // 30 minutes

// Valeur sentinel = 0x8000 (pas de donnée)
#define HIST_NO_DATA        0x8000

struct HistorySerie {
    int16_t  data[HIST_POINTS];   // valeurs * facteur d'echelle
    uint8_t  count;               // nombre points enregistres (max HIST_POINTS)
    uint8_t  head;                // prochaine position d'ecriture
    float    scale;               // facteur de division pour reconvertir
};

extern HistorySerie hist_soc;       // % * 10  (90.5% -> 905)
extern HistorySerie hist_voltage;   // V * 100 (12.69V -> 1269)
extern HistorySerie hist_pv_power;  // W       (70W -> 70)
extern HistorySerie hist_load_power;// W       (35W -> 35)
extern HistorySerie hist_temp;      // °C * 10 (21.5°C -> 215)

void history_init();
void history_update();    // appeler dans loop()

// Recharger les donnees du jour depuis SD au boot
// Appel apres sd_card_init() pour re-remplir les graphiques
void history_load_from_sd();

// Acces aux series : reorganise les points dans l'ordre chrono croissant
//   out[0]   = plus ancien
//   out[N-1] = plus recent
// Retourne le nombre de points valides
int history_get_ordered(const HistorySerie& s, int16_t* out, int max_n);

