#pragma once
/*
 * pir_sensor.h — Détection présence PIR + extinction écran
 * 
 * GPIO 6 (connecteur ADC/Sensor PH2.0 du Waveshare)
 * Le PIR est en entrée digitale (HIGH = mouvement détecté)
 * 
 * Logique :
 *  - Si mouvement → rétro-éclairage ON, reset timer
 *  - Si pas de mouvement pendant DISPLAY_TIMEOUT_MS → éteindre
 *
 * ⚠️  DESACTIVE : GPIO 6 est maintenant utilise par le GPS (UART RX).
 * A reactiver quand le PIR sera porte sur le slave PCB via ESP-NOW.
 * Voir TODO bas de fichier.
 */
#include <stdint.h>
#include <stdbool.h>

// 1 = PIR cable physiquement sur GPIO 6 du Waveshare (incompatible avec GPS)
// 0 = PIR desactive (par defaut maintenant : GPIO 6 sert au GPS)
#define ENABLE_PIR  0

#define PIR_GPIO                 6
#define DISPLAY_TIMEOUT_MS       30000UL    // 30s pour test, mettre 300000 (5 min) en prod

extern bool pirLastState;       // true = mouvement détecté maintenant
extern bool displayOn;          // true = rétro-éclairage ON
extern uint32_t lastMotionMs;   // dernier mouvement détecté

void pir_init();
void pir_update();              // À appeler dans loop()
void display_force_on();        // Rallume (appel touch/manual ; toujours actif meme si ENABLE_PIR=0)

// TODO : quand le slave PCB sera connecte en ESP-NOW, brancher le PIR la-bas
// et envoyer un MSG_MOTION au master qui appellera display_force_on().
// Pour ca : ajouter un type de message dans espnow_protocol.h et un handler
// dans espnow_master.cpp.
