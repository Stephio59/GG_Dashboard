/*
 * theme_mode.h - Gestion mode jour/nuit
 * Detecte automatiquement selon l'heure :
 *   - Mode jour : 7h00 -> 21h00 (clair)
 *   - Mode nuit : 21h00 -> 7h00 (sombre, actuel)
 */
#pragma once
#include <stdint.h>

enum ThemeMode {
    THEME_NIGHT = 0,   // sombre (actuel)
    THEME_DAY   = 1    // clair
};

extern ThemeMode current_theme;

// A appeler periodiquement (ex: toutes les minutes)
// Met a jour current_theme selon l'heure NTP
// Retourne true si le theme a change
bool theme_update_auto();

// Forcer un mode (manuel)
void theme_set(ThemeMode mode);
