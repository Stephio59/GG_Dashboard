#pragma once
/*
 * screen_sleep.h — Gestion mise en veille écran (van mode économie)
 * 
 * Logique :
 *   - Pas d'activité 90s  → DIMMING (50% lumi)
 *   - Pas d'activité 120s → OFF (backlight off)
 *   - Activité (PIR ou tactile) → réveil instantané
 * 
 * L'ESP32 reste actif (BLE, WiFi, ESP-NOW continuent).
 */
#include <Arduino.h>

enum ScreenState {
    SCREEN_ON = 0,      // 100% lumi
    SCREEN_DIM,         // 50% lumi (transition)
    SCREEN_OFF          // backlight 0%, écran noir
};

// Configuration timeouts (millisecondes)
#define SLEEP_DIM_MS    90000UL    // 1m30 → dimming
#define SLEEP_OFF_MS    120000UL   // 2min → off

void screen_sleep_init();
void screen_sleep_update();         // appeler dans loop()

// À appeler quand activité détectée (PIR, tactile, etc.)
void screen_sleep_wake();

// État
ScreenState screen_get_state();
bool screen_is_awake();

// Configuration (utilisable depuis page Système)
void screen_set_user_brightness(uint8_t pct);  // 0..100
uint8_t screen_get_user_brightness();
