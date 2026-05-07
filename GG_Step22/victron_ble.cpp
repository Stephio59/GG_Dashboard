/*
 * victron_ble.cpp — Décodeur advertisements Victron (AES-CTR)
 * 
 * Format payload manufacturer (après le 0xE1 0x02 = Victron) :
 *   0x10 (1 byte)  - prefix Instant Readout
 *   XX XX (2 bytes) - model ID
 *   XX (1 byte)    - record type (0x01=sollar, 0x02=battery monitor, 
 *                                  0x03=charger, 0x05=DC/DC, 0x06=SmartShunt, etc.)
 *   XX XX (2)      - nonce
 *   XX (1)         - first byte of AES bindkey (validation)
 *   encrypted...   - payload AES-CTR
 * 
 * Ref : https://github.com/keshavdv/victron-ble (Python)
 * Ref : https://github.com/Fabian-Schmidt/esphome-victron_ble
 */
#include "victron_ble.h"
#include "bluetooth_config.h"
#include "user_config.h"
#include "mbedtls/aes.h"
#include <string.h>
#include <ctype.h>

VictronMpptData  victronMpptData  = {};
VictronBmvData   victronBmvData   = {};
VictronBpData    victronBpData    = {};
VictronOrionData victronOrionData = {};

// ── Helpers ────────────────────────────────────────────────
static inline int16_t rd_i16(const uint8_t* p) {
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline uint16_t rd_u16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t rd_u24(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16);
}

// Décoder hex string "abcd..." → bytes
static bool hex2bin(const char* hex, uint8_t* out, size_t outlen) {
    size_t slen = strlen(hex);
    if (slen != outlen * 2) return false;
    for (size_t i = 0; i < outlen; i++) {
        char hi = hex[i*2], lo = hex[i*2 + 1];
        uint8_t h = (hi <= '9') ? (hi - '0') : ((hi|0x20) - 'a' + 10);
        uint8_t l = (lo <= '9') ? (lo - '0') : ((lo|0x20) - 'a' + 10);
        out[i] = (h << 4) | l;
    }
    return true;
}

// Comparaison MAC insensible à la casse
static bool mac_eq(const char* a, const char* b) {
    while (*a && *b) {
        char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
        char cb = (*b >= 'a' && *b <= 'z') ? (*b - 32) : *b;
        if (ca != cb) return false;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

// ── AES-CTR decrypt ────────────────────────────────────────
static bool aes_ctr_decrypt(const uint8_t* key16, uint16_t nonce,
                            const uint8_t* in, uint8_t* out, size_t len) {
    // Counter : 16 bytes = nonce_LE (2) + 14 zéros
    uint8_t counter[16] = {0};
    counter[0] = (uint8_t)(nonce & 0xFF);
    counter[1] = (uint8_t)((nonce >> 8) & 0xFF);
    
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    if (mbedtls_aes_setkey_enc(&ctx, key16, 128) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }
    
    uint8_t stream_block[16];
    size_t nc_off = 0;
    int ret = mbedtls_aes_crypt_ctr(&ctx, len, &nc_off, counter, stream_block, in, out);
    
    mbedtls_aes_free(&ctx);
    return (ret == 0);
}

// ── Parsers par type d'appareil ────────────────────────────

// Record type 0x01 = Solar Charger (MPPT)
static void parse_solar(const uint8_t* d, size_t len) {
    if (len < 9) return;
    // Format :
    //   d[0]     : device_state (u8)
    //   d[1]     : charger_error (u8)
    //   d[2..3]  : voltage_bat i16 x0.01 V
    //   d[4..5]  : current i16 x0.1 A
    //   d[6..7]  : yield_today u16 x10 Wh
    //   d[8..9]  : pv_power u16 W
    //   d[10..11]: load_current u16 x0.1 A (-1 = N/A)
    
    victronMpptData.device_state = d[0];
    victronMpptData.charger_error = d[1];
    victronMpptData.voltage_bat = rd_i16(d + 2) * 0.01f;
    victronMpptData.current     = rd_i16(d + 4) * 0.1f;
    victronMpptData.yield_today_wh = rd_u16(d + 6) * 10.0f;
    victronMpptData.power_pv_w  = rd_u16(d + 8);
    
    // Tension PV : calculée à partir de P = V × I
    if (victronMpptData.current > 0.05f) {
        victronMpptData.voltage_pv = victronMpptData.power_pv_w / victronMpptData.current;
    } else {
        victronMpptData.voltage_pv = 0;
    }
    
    victronMpptData.valid = true;
    victronMpptData.last_update_ms = millis();
    
    Serial0.printf("[Victron MPPT] V=%.2fV I=%.2fA PV=%.0fW Today=%.0fWh state=%d\n",
        victronMpptData.voltage_bat, victronMpptData.current,
        victronMpptData.power_pv_w, victronMpptData.yield_today_wh,
        victronMpptData.device_state);
}

// Record type 0x02 = Battery Monitor (BMV) — bit-packed format !
static void parse_battery_monitor(const uint8_t* d, size_t len) {
    if (len < 14) return;
    // Format officiel :
    //   d[0..1]  : ttg minutes u16
    //   d[2..3]  : voltage i16 x0.01 V
    //   d[4..5]  : alarm reason u16
    //   d[6..7]  : aux_voltage i16 mV (or temperature dK if temp mode)
    //   d[8..14] : current(22 bits) + consumed_ah(20 bits) + soc(10 bits) = 52 bits BIT-PACKED
    
    victronBmvData.time_to_go_min = rd_u16(d + 0);
    victronBmvData.voltage = rd_i16(d + 2) * 0.01f;
    victronBmvData.aux_voltage_mv = rd_i16(d + 6);
    
    // Construire un uint64 little-endian a partir des 7 bytes a partir de d[8]
    uint64_t bits = 0;
    for (int i = 0; i < 7 && (8 + i) < (int)len; i++) {
        bits |= ((uint64_t)d[8 + i]) << (i * 8);
    }
    
    // Extraire current (22 bits LSB), signé
    int32_t current_raw = bits & 0x3FFFFF;  // 22 bits
    if (current_raw & 0x200000) current_raw -= 0x400000;  // sign extend
    
    // Sentinel : 0x200000 (= -2097152) signifie "invalid"
    if (current_raw == -2097152 || current_raw == 0x1FFFFF) {
        victronBmvData.current = 0;
    } else {
        victronBmvData.current = current_raw * 0.001f;
    }
    
    // Extraire consumed_ah (20 bits suivants)
    uint32_t consumed_raw = (bits >> 22) & 0xFFFFF;  // 20 bits
    if (consumed_raw == 0xFFFFF) {
        victronBmvData.consumed_ah = 0;
    } else {
        victronBmvData.consumed_ah = consumed_raw * 0.1f;
    }
    
    // Extraire SOC (10 bits suivants)
    uint16_t soc_raw = (bits >> 42) & 0x3FF;  // 10 bits
    if (soc_raw == 0x3FF) {
        victronBmvData.soc_pct = 0;
    } else {
        victronBmvData.soc_pct = soc_raw * 0.1f;
    }
    
    victronBmvData.power_w = victronBmvData.voltage * victronBmvData.current;
    victronBmvData.valid = true;
    victronBmvData.last_update_ms = millis();
    
    Serial0.printf("[Victron BMV] V=%.2fV I=%+.2fA SOC=%.1f%% TTG=%dmin Consum=%.1fAh\n",
        victronBmvData.voltage, victronBmvData.current,
        victronBmvData.soc_pct,
        victronBmvData.time_to_go_min == 0xFFFF ? -1 : victronBmvData.time_to_go_min,
        victronBmvData.consumed_ah);
}

// Record type 0x09 = Smart BatteryProtect
static void parse_battery_protect(const uint8_t* d, size_t len) {
    if (len < 11) return;
    // Format BP confirme :
    //   d[0]      : device_state (varie, ex: 0xF9 = état non doc mais valide)
    //   d[1]      : output_state (0x00 = OFF, 0x01 = ON)
    //   d[2]      : error_code
    //   d[3..4]   : alarm_reason u16
    //   d[5..6]   : warning_reason u16
    //   d[7..8]   : input voltage i16 x0.01 V
    //   d[9..10]  : output voltage u16 x0.01 V (sortie)
    //   d[11..14] : off_reason u32
    
    victronBpData.state        = d[0];
    victronBpData.output_state = d[1];
    
    // Tension d'entrée (pas de sortie - c'est l'entrée qui compte pour batterie)
    int16_t v_in_raw = (int16_t)((uint16_t)d[7] | ((uint16_t)d[8] << 8));
    victronBpData.voltage = v_in_raw * 0.01f;
    
    victronBpData.valid = true;
    victronBpData.last_update_ms = millis();
    
    Serial0.printf("[Victron BP] V=%.2fV state=%d out=%s\n",
        victronBpData.voltage, victronBpData.state,
        victronBpData.output_state ? "ON" : "OFF");
}

// Record type 0x04 = DC/DC converter (Orion Smart)
static void parse_dcdc(const uint8_t* d, size_t len) {
    if (len < 6) return;
    //   d[0]    : device state (0x00=OFF, 0x03=BULK, 0x04=ABSORPTION, 0x05=FLOAT)
    //   d[1]    : charger error (0x00=OK)
    //   d[2..3] : input voltage i16 x0.01 V
    //   d[4..5] : output voltage i16 x0.01 V (0x7FFF = sentinel SI OFF)
    //   d[6]    : off_reason (bit7=Engine shutdown detected)
    
    victronOrionData.device_state   = d[0];
    victronOrionData.charger_error  = d[1];
    victronOrionData.input_voltage  = rd_i16(d + 2) * 0.01f;
    
    int16_t vout_raw = rd_i16(d + 4);
    if (vout_raw == 0x7FFF) {
        victronOrionData.output_voltage = 0;  // sentinel "off"
    } else {
        victronOrionData.output_voltage = vout_raw * 0.01f;
    }
    
    victronOrionData.valid = true;
    victronOrionData.last_update_ms = millis();
    
    Serial0.printf("[Victron DCDC] In=%.2fV Out=%.2fV state=%d\n",
        victronOrionData.input_voltage, victronOrionData.output_voltage,
        victronOrionData.device_state);
}

// ── API ────────────────────────────────────────────────────
void victron_init() {
    victronMpptData = {};
    victronBmvData = {};
    victronBpData = {};
    victronOrionData = {};
    Serial0.println("[Victron] Module initialise");
}

void victron_on_advertisement(const char* mac, const uint8_t* data, size_t len) {
    if (len < 8) return;
    
    int idx = -1;
    const char* name = "?";
    const char* bindkey_hex = nullptr;
    
    if (mac_eq(mac, userConfig.victron_mppt_mac))        { idx=0; name="MPPT";  bindkey_hex = userConfig.victron_mppt_bindkey;  }
    else if (mac_eq(mac, userConfig.victron_bmv_mac))    { idx=1; name="BMV";   bindkey_hex = userConfig.victron_bmv_bindkey;   }
    else if (mac_eq(mac, userConfig.victron_bp_mac))     { idx=2; name="BP";    bindkey_hex = userConfig.victron_bp_bindkey;    }
    else if (mac_eq(mac, userConfig.victron_orion_mac))  { idx=3; name="Orion"; bindkey_hex = userConfig.victron_orion_bindkey; }
    else return;
    
    // Format Victron Instant Readout (confirmé par dump hex) :
    //   data[0..1]  : prefix 0x10 0xXX
    //   data[2..3]  : model id (LE)
    //   data[4]     : record type (0x01=solar, 0x02=BMV, 0x04=BP, 0x05=DCDC, 0x09=BMV alt)
    //   data[5..6]  : nonce (u16 LE)
    //   data[7]     : key first byte (validation)
    //   data[8..]   : encrypted payload
    
    uint8_t record_type    = data[4];
    uint16_t nonce         = (uint16_t)data[5] | ((uint16_t)data[6] << 8);
    uint8_t key_first_byte = data[7];
    const uint8_t* enc     = data + 8;
    size_t enc_len         = len - 8;
    
    if (enc_len < 1 || enc_len > 20) return;
    
    uint8_t bindkey[16];
    if (!hex2bin(bindkey_hex, bindkey, 16)) return;
    
    // Vérifier le 1er byte de la clé
    if (bindkey[0] != key_first_byte) {
        static uint32_t last_warn = 0;
        if (millis() - last_warn > 60000) {
            Serial0.printf("[Victron %s] Bindkey mismatch: attendu 0x%02X recu 0x%02X\n",
                name, bindkey[0], key_first_byte);
            last_warn = millis();
        }
        return;
    }
    
    // Décrypter
    uint8_t decrypted[24] = {0};
    if (!aes_ctr_decrypt(bindkey, nonce, enc, decrypted, enc_len)) {
        Serial0.printf("[Victron %s] AES decrypt fail\n", name);
        return;
    }
    
    // Dispatcher par type (selon doc officielle Victron Instant Readout)
    switch (record_type) {
        case 0x01: parse_solar(decrypted, enc_len);           break; // Solar Charger (MPPT)
        case 0x02: parse_battery_monitor(decrypted, enc_len); break; // Battery Monitor (BMV/SmartShunt)
        case 0x03: /* Inverter */                             break;
        case 0x04: parse_dcdc(decrypted, enc_len);            break; // DC/DC Converter (Orion)
        case 0x05: /* SmartLithium */                         break;
        case 0x06: /* Inverter RS */                          break;
        case 0x07: /* GX Device */                            break;
        case 0x08: /* AC Charger */                           break;
        case 0x09: parse_battery_protect(decrypted, enc_len); break; // Smart Battery Protect
        case 0x0A: /* Lynx Smart BMS */                       break;
        case 0x0B: /* Multi RS */                             break;
        case 0x0C: /* VE.Bus */                               break;
        case 0x0D: /* DC Energy Meter */                      break;
        case 0x0E: /* Orion XS */                             break;
        default:
            static uint32_t last_t = 0;
            if (millis() - last_t > 60000) {
                Serial0.printf("[Victron %s] Record type 0x%02X non supporte\n",
                    name, record_type);
                last_t = millis();
            }
            break;
    }
}

bool victron_mppt_available() {
    return victronMpptData.valid && (millis() - victronMpptData.last_update_ms) < 30000;
}

bool victron_bmv_available() {
    return victronBmvData.valid && (millis() - victronBmvData.last_update_ms) < 30000;
}
