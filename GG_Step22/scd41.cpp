/*
 * scd41.cpp — Driver SCD41 CO2 via driver I2C Waveshare
 * 
 * Partage le bus I2C_NUM_0 (GPIO 8/9) avec GT911, CH422G et BME280
 */
#include "scd41.h"
#include <Arduino.h>
#include "i2c.h"

#define SCD41_ADDR               0x62

// Commandes SCD4x (datasheet Sensirion)
#define CMD_START_PERIODIC       0x21B1
#define CMD_STOP_PERIODIC        0x3F86
#define CMD_READ_MEASUREMENT     0xEC05
#define CMD_GET_DATA_READY       0xE4B8
#define CMD_REINIT               0x3646
#define CMD_GET_SERIAL_NUMBER    0x3682
#define CMD_WAKEUP               0x36F6

Scd41Data scd41Data = {0, 0, 0, false, 0};
bool scd41Available = false;

static i2c_master_dev_handle_t scd_dev = nullptr;
static uint32_t last_read_ms = 0;
static const uint32_t READ_INTERVAL_MS = 5000;   // SCD41 = 5s en mode périodique
static bool measurement_started = false;

// CRC-8 polynome 0x31, init 0xFF (Sensirion standard)
static uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else            crc <<= 1;
        }
    }
    return crc;
}

// Envoyer une commande 16-bit (pas de data)
static bool scd_send_cmd(uint16_t cmd) {
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return (i2c_master_transmit(scd_dev, buf, 2, 100) == ESP_OK);
}

// Lire N words (16-bit) après envoi de commande, avec vérif CRC
// Retourne true si tous les CRC sont bons
static bool scd_read_words(uint16_t cmd, uint16_t* words, size_t n_words, uint32_t delay_ms) {
    if (!scd_send_cmd(cmd)) return false;
    delay(delay_ms);
    
    size_t total = n_words * 3;  // 2 data + 1 CRC par word
    uint8_t buf[18]; // max 6 words
    if (total > sizeof(buf)) return false;
    
    if (i2c_master_receive(scd_dev, buf, total, 100) != ESP_OK) return false;
    
    for (size_t i = 0; i < n_words; i++) {
        uint8_t* p = buf + i*3;
        if (crc8(p, 2) != p[2]) {
            Serial0.printf("[SCD41] CRC fail word %u\n", (unsigned)i);
            return false;
        }
        words[i] = ((uint16_t)p[0] << 8) | p[1];
    }
    return true;
}

void scd41_init() {
    Serial0.println("[SCD41] Init sur bus I2C partage (0x62)...");
    
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = SCD41_ADDR;
    dev_cfg.scl_speed_hz = 100000;  // SCD41 max 100kHz
    
    if (i2c_master_bus_add_device(handle.bus, &dev_cfg, &scd_dev) != ESP_OK) {
        Serial0.println("[SCD41] add_device fail");
        return;
    }
    
    // Wake-up (au cas où il serait en sleep)
    scd_send_cmd(CMD_WAKEUP);
    delay(30);
    
    // Stop éventuelle mesure en cours
    scd_send_cmd(CMD_STOP_PERIODIC);
    delay(500);
    
    // Lire numéro de série pour confirmer présence
    uint16_t sn[3];
    if (!scd_read_words(CMD_GET_SERIAL_NUMBER, sn, 3, 1)) {
        Serial0.println("[SCD41] NON detecte");
        i2c_master_bus_rm_device(scd_dev);
        scd_dev = nullptr;
        return;
    }
    
    Serial0.printf("[SCD41] Serial: %04X%04X%04X\n", sn[0], sn[1], sn[2]);
    
    // Démarrer mesure périodique (5s)
    if (!scd_send_cmd(CMD_START_PERIODIC)) {
        Serial0.println("[SCD41] start_periodic fail");
        return;
    }
    
    measurement_started = true;
    scd41Available = true;
    Serial0.println("[SCD41] OK (mesure toutes les 5s)");
}

void scd41_update() {
    if (!scd41Available || !measurement_started) return;
    
    uint32_t now = millis();
    if (now - last_read_ms < READ_INTERVAL_MS) return;
    last_read_ms = now;
    
    // 1. Vérifier si données prêtes
    uint16_t ready;
    if (!scd_read_words(CMD_GET_DATA_READY, &ready, 1, 2)) return;
    if ((ready & 0x07FF) == 0) return;  // pas prêt (bits 10:0 = 0)
    
    // 2. Lire la mesure
    uint16_t words[3];
    if (!scd_read_words(CMD_READ_MEASUREMENT, words, 3, 2)) return;
    
    // Décodage selon datasheet Sensirion
    scd41Data.co2_ppm    = words[0];
    scd41Data.temperature = -45.0f + 175.0f * (float)words[1] / 65535.0f;
    scd41Data.humidity    =        100.0f * (float)words[2] / 65535.0f;
    scd41Data.valid       = true;
    scd41Data.lastUpdate  = now;
}
