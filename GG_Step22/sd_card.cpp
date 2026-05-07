/*
 * sd_card.cpp - Carte SD via SD_MMC mode 1-bit
 */
#include "sd_card.h"
#include "FS.h"
#include "SD_MMC.h"
#include "io_extension.h"

#define SD_PIN_CLK 12
#define SD_PIN_CMD 11
#define SD_PIN_D0  13

SdCardInfo sdCard = {false, false, 0, 0, "Aucune"};

bool sd_card_init() {
    Serial0.println("[SD] Init carte SD...");
    
    // Activer le CS de la SD via le CH32V003 (IO4 = HIGH)
    IO_EXTENSION_Output(IO_EXTENSION_IO_4, 1);
    delay(50);
    
    // Configurer les pins SD_MMC
    if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
        Serial0.println("[SD] Echec configuration pins SD_MMC");
        sdCard.present = false;
        return false;
    }
    
    // Mode 1-bit (mode 4-bit serait plus rapide mais besoin de 4 data pins)
    // mount_point="/sdcard", mode_1bit=true, format_if_failed=false
    if (!SD_MMC.begin("/sdcard", true, false)) {
        Serial0.println("[SD] Echec montage SD (carte absente ou probleme FS ?)");
        sdCard.present = false;
        sdCard.mounted = false;
        strncpy(sdCard.type, "Aucune", sizeof(sdCard.type));
        return false;
    }
    
    sdCard.mounted = true;
    
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial0.println("[SD] Aucune carte SD detectee");
        sdCard.present = false;
        strncpy(sdCard.type, "Aucune", sizeof(sdCard.type));
        SD_MMC.end();
        sdCard.mounted = false;
        return false;
    }
    
    sdCard.present = true;
    
    if (cardType == CARD_MMC)        strncpy(sdCard.type, "MMC", sizeof(sdCard.type));
    else if (cardType == CARD_SD)    strncpy(sdCard.type, "SDSC", sizeof(sdCard.type));
    else if (cardType == CARD_SDHC)  strncpy(sdCard.type, "SDHC", sizeof(sdCard.type));
    else                             strncpy(sdCard.type, "INCONNU", sizeof(sdCard.type));
    
    Serial0.printf("[SD] Carte montee. Type: %s\n", sdCard.type);
    
    sd_card_refresh_stats();
    Serial0.printf("[SD] Capacite totale: %llu MB, Libre: %llu MB\n",
                   sdCard.total_mb, sdCard.available_mb);
    
    return true;
}

void sd_card_refresh_stats() {
    if (!sdCard.mounted) return;
    
    uint64_t total_bytes = SD_MMC.cardSize();
    uint64_t used_bytes = SD_MMC.usedBytes();
    
    sdCard.total_mb = total_bytes / (1024ULL * 1024ULL);
    sdCard.available_mb = (total_bytes - used_bytes) / (1024ULL * 1024ULL);
}

void sd_card_list_root() {
    if (!sdCard.mounted) {
        Serial0.println("[SD] Pas montee, impossible de lister");
        return;
    }
    
    Serial0.println("[SD] === Contenu racine ===");
    File root = SD_MMC.open("/");
    if (!root || !root.isDirectory()) {
        Serial0.println("[SD] Erreur ouverture racine");
        return;
    }
    
    File f = root.openNextFile();
    int count = 0;
    while (f && count < 30) {
        if (f.isDirectory()) {
            Serial0.printf("  [DIR]  %s\n", f.name());
        } else {
            Serial0.printf("  [FILE] %s (%u bytes)\n", f.name(), (unsigned)f.size());
        }
        f = root.openNextFile();
        count++;
    }
    if (count == 0) {
        Serial0.println("  (racine vide)");
    }
    Serial0.println("[SD] === Fin liste ===");
}

bool sd_card_write_file(const char* path, const char* content) {
    if (!sdCard.mounted) return false;
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
        Serial0.printf("[SD] Echec ouverture %s en ecriture\n", path);
        return false;
    }
    f.print(content);
    f.close();
    return true;
}

bool sd_card_append_file(const char* path, const char* content) {
    if (!sdCard.mounted) return false;
    File f = SD_MMC.open(path, FILE_APPEND);
    if (!f) {
        Serial0.printf("[SD] Echec ouverture %s en append\n", path);
        return false;
    }
    f.print(content);
    f.close();
    return true;
}

String sd_card_read_file(const char* path) {
    if (!sdCard.mounted) return String();
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial0.printf("[SD] Echec ouverture %s en lecture\n", path);
        return String();
    }
    String content;
    while (f.available()) {
        content += (char)f.read();
    }
    f.close();
    return content;
}
