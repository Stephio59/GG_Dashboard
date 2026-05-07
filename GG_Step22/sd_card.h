/*
 * sd_card.h - Carte SD via SD_MMC (mode 1-bit)
 * Pins Waveshare 7B :
 *   CLK = GPIO 12
 *   CMD = GPIO 11
 *   D0  = GPIO 13
 *   CS  = IO_EXTENSION_IO_4 (CH32V003)
 */
#pragma once
#include <Arduino.h>
#include <stdint.h>

struct SdCardInfo {
    bool   present;          // SD detectee
    bool   mounted;          // Montee avec succes
    uint64_t total_mb;       // Capacite totale (MB)
    uint64_t available_mb;   // Espace libre (MB)
    char   type[16];         // "SDSC", "SDHC", "MMC", etc.
};

extern SdCardInfo sdCard;

// Init au boot - appel apres IO_EXTENSION_Init()
bool sd_card_init();

// Mise a jour des stats (espace libre) - a appeler periodiquement
void sd_card_refresh_stats();

// Lister les fichiers a la racine (pour debug serial)
void sd_card_list_root();

// Helpers haut niveau
bool sd_card_write_file(const char* path, const char* content);
bool sd_card_append_file(const char* path, const char* content);
String sd_card_read_file(const char* path);
