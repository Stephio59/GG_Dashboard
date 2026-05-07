/*
 * bme_wire1.cpp — Driver BME280 utilisant le driver I2C Waveshare
 * 
 * Partage le bus I2C_NUM_0 (GPIO 8/9) avec GT911 et CH422G
 * Pas de conflit driver_ng (tout utilise i2c_master)
 */
#include "bme_wire1.h"
#include <Arduino.h>
#include "i2c.h"

#define BME_ADDR_1 0x76
#define BME_ADDR_2 0x77

// Registres
#define REG_ID        0xD0
#define REG_RESET     0xE0
#define REG_CTRL_HUM  0xF2
#define REG_STATUS    0xF3
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_DATA      0xF7  // 8 bytes: pressure[3] temp[3] hum[2]
#define REG_CALIB_T   0x88  // 26 bytes calibration T+P
#define REG_CALIB_H1  0xA1
#define REG_CALIB_H2  0xE1  // 7 bytes calibration H

// Données globales
BmeData bmeData = {0, 0, 0, false, 0};
bool bmeAvailable = false;

static i2c_master_dev_handle_t bme_dev = nullptr;
static uint32_t last_read_ms = 0;
static const uint32_t READ_INTERVAL_MS = 2000;

// Calibration
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;
static int32_t  t_fine;

static bool bme_read_reg(uint8_t reg, uint8_t* buf, size_t len) {
    esp_err_t err = i2c_master_transmit_receive(bme_dev, &reg, 1, buf, len, 100);
    return (err == ESP_OK);
}

static bool bme_write_reg(uint8_t reg, uint8_t val) {
    uint8_t data[2] = { reg, val };
    esp_err_t err = i2c_master_transmit(bme_dev, data, 2, 100);
    return (err == ESP_OK);
}

static bool bme_try_addr(uint8_t addr) {
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = addr;
    dev_cfg.scl_speed_hz = 400000;
    
    if (i2c_master_bus_add_device(handle.bus, &dev_cfg, &bme_dev) != ESP_OK) {
        Serial0.printf("[BME280] add_device fail @0x%02X\n", addr);
        return false;
    }
    
    uint8_t id = 0;
    if (!bme_read_reg(REG_ID, &id, 1)) {
        Serial0.printf("[BME280] pas de reponse @0x%02X\n", addr);
        i2c_master_bus_rm_device(bme_dev);
        bme_dev = nullptr;
        return false;
    }
    
    if (id != 0x60) {
        Serial0.printf("[BME280] ID invalide @0x%02X = 0x%02X (attendu 0x60)\n", addr, id);
        i2c_master_bus_rm_device(bme_dev);
        bme_dev = nullptr;
        return false;
    }
    
    Serial0.printf("[BME280] Trouve @0x%02X (ID=0x60)\n", addr);
    return true;
}

static bool bme_load_calibration() {
    uint8_t cal[26];
    if (!bme_read_reg(REG_CALIB_T, cal, 26)) return false;
    dig_T1 = cal[0]  | (cal[1]  << 8);
    dig_T2 = cal[2]  | (cal[3]  << 8);
    dig_T3 = cal[4]  | (cal[5]  << 8);
    dig_P1 = cal[6]  | (cal[7]  << 8);
    dig_P2 = cal[8]  | (cal[9]  << 8);
    dig_P3 = cal[10] | (cal[11] << 8);
    dig_P4 = cal[12] | (cal[13] << 8);
    dig_P5 = cal[14] | (cal[15] << 8);
    dig_P6 = cal[16] | (cal[17] << 8);
    dig_P7 = cal[18] | (cal[19] << 8);
    dig_P8 = cal[20] | (cal[21] << 8);
    dig_P9 = cal[22] | (cal[23] << 8);
    
    if (!bme_read_reg(REG_CALIB_H1, &dig_H1, 1)) return false;
    
    uint8_t calH[7];
    if (!bme_read_reg(REG_CALIB_H2, calH, 7)) return false;
    dig_H2 = calH[0] | (calH[1] << 8);
    dig_H3 = calH[2];
    dig_H4 = ((int16_t)(int8_t)calH[3] << 4) | (calH[4] & 0x0F);
    dig_H5 = ((int16_t)(int8_t)calH[5] << 4) | (calH[4] >> 4);
    dig_H6 = (int8_t)calH[6];
    return true;
}

void bme_wire1_init() {
    Serial0.println("[BME280] Init sur bus I2C partage Waveshare (GPIO 8/9)...");
    
    // Tenter 0x76 puis 0x77
    if (!bme_try_addr(BME_ADDR_1)) {
        if (!bme_try_addr(BME_ADDR_2)) {
            Serial0.println("[BME280] NON detecte");
            bmeAvailable = false;
            return;
        }
    }
    
    // Charger calibration
    if (!bme_load_calibration()) {
        Serial0.println("[BME280] Erreur lecture calibration");
        bmeAvailable = false;
        return;
    }
    
    // Configuration : humidity oversampling x1, temp x2, pressure x16, mode normal
    bme_write_reg(REG_CTRL_HUM, 0x01);   // hum x1
    bme_write_reg(REG_CTRL_MEAS, 0x57);  // temp x2, press x16, mode normal (0b01010111)
    bme_write_reg(REG_CONFIG, 0xA0);     // standby 1000ms, filter x16
    
    bmeAvailable = true;
    Serial0.println("[BME280] OK");
}

void bme_wire1_update() {
    if (!bmeAvailable) return;
    uint32_t now = millis();
    if (now - last_read_ms < READ_INTERVAL_MS) return;
    last_read_ms = now;
    
    uint8_t data[8];
    if (!bme_read_reg(REG_DATA, data, 8)) {
        bmeData.valid = false;
        return;
    }
    
    int32_t adc_P = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | (data[5] >> 4);
    int32_t adc_H = ((uint32_t)data[6] << 8)  |  (uint32_t)data[7];
    
    // Compensation temperature (datasheet)
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    bmeData.temperature = T / 100.0f;
    
    // Compensation pression
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)dig_P6;
    v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
    v2 = v2 + (((int64_t)dig_P4) << 35);
    v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
    v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;
    if (v1 == 0) { bmeData.pressure = 0; }
    else {
        int64_t p = 1048576 - adc_P;
        p = (((p << 31) - v2) * 3125) / v1;
        v1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        v2 = (((int64_t)dig_P8) * p) >> 19;
        p = ((p + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);
        bmeData.pressure = (float)p / 25600.0f;  // Pa -> hPa
    }
    
    // Compensation humidité
    int32_t vh = (t_fine - ((int32_t)76800));
    vh = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * vh)) +
          ((int32_t)16384)) >> 15) * (((((((vh * ((int32_t)dig_H6)) >> 10) *
          (((vh * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
          ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
    vh = (vh - (((((vh >> 15) * (vh >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
    vh = (vh < 0) ? 0 : vh;
    vh = (vh > 419430400) ? 419430400 : vh;
    bmeData.humidity = (float)(vh >> 12) / 1024.0f;
    
    bmeData.valid = true;
    bmeData.lastUpdate = now;
}
