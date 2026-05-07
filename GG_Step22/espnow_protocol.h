#pragma once
/*
 * espnow_protocol.h - Protocole ESP-NOW partage entre :
 *   - Master  : Waveshare ESP32-S3 (dashboard)
 *   - Slave   : ESP32-U sur PCB (relais + ADC eau)
 * 
 * Ce fichier est INCLUS DES DEUX COTES — ne pas modifier l'un sans l'autre !
 *
 * Architecture :
 *   - Master envoie des commandes relais (CmdRelays)
 *   - Slave envoie son etat (StatusUpdate) a 1Hz et a chaque changement
 *   - Channel WiFi : 6 (par defaut)
 *   - Encryption : aucune (LAN local, pas critique)
 */
#include <Arduino.h>
#include <stdint.h>

// Channel WiFi par defaut
#define ESPNOW_CHANNEL 6

// MAC des deux peers (a remplir une fois flashes)
// Pour trouver les MACs : Serial.println(WiFi.macAddress()); au boot
extern uint8_t MASTER_MAC[6];   // Waveshare ESP32-S3
extern uint8_t SLAVE_MAC[6];    // ESP32-U PCB

// Types de message
#define MSG_CMD_RELAYS    0x01   // Master -> Slave : commande relais
#define MSG_STATUS        0x02   // Slave -> Master : etat capteurs
#define MSG_PING          0x03   // Master -> Slave : keepalive
#define MSG_PONG          0x04   // Slave -> Master : reponse ping

// ═══════════════════════════════════════════════════════════
//  Master -> Slave : commandes relais
// ═══════════════════════════════════════════════════════════
typedef struct __attribute__((packed)) {
    uint8_t  msg_type;        // = MSG_CMD_RELAYS
    uint8_t  relays_state;    // bitmask 8 bits (relais 1..8 = bit 0..7)
                              //   bit 0 = relais 1
                              //   bit 1 = relais 2
                              //   ...
                              //   bit 7 = relais 8
    uint8_t  reserved[6];     // pour future utilisation (PWM, durees, etc.)
    uint32_t timestamp_ms;    // millis() emetteur
} CmdRelaysMsg;

// ═══════════════════════════════════════════════════════════
//  Slave -> Master : etat
// ═══════════════════════════════════════════════════════════
typedef struct __attribute__((packed)) {
    uint8_t  msg_type;        // = MSG_STATUS
    
    // Niveaux eau (CBE Solid State sondes capacitives)
    float    water_clean_pct;  // 0-100% niveau eau propre (GPIO 34)
    float    water_dirty_pct;  // 0-100% niveau eau sale (GPIO 35)
    
    // Etat reel relais (feedback : ce que le slave a vraiment applique)
    uint8_t  relays_actual;   // bitmask
    
    // Telemetry slave
    float    voltage_in;      // tension entree PCB (12V via diviseur)
    int16_t  rssi;            // RSSI dernier message ESPNOW recu
    uint32_t uptime_s;        // secondes depuis boot du slave
    uint8_t  reserved[6];
    uint32_t timestamp_ms;
} StatusMsg;

// ═══════════════════════════════════════════════════════════
//  Master <-> Slave : ping / pong
// ═══════════════════════════════════════════════════════════
typedef struct __attribute__((packed)) {
    uint8_t  msg_type;        // = MSG_PING ou MSG_PONG
    uint32_t timestamp_ms;
} PingMsg;

