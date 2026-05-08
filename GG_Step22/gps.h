#pragma once
/*
 * gps.h — Lecture GPS GNSS multi-constellations via UART
 * 
 * Module branche : GNSS multi-systemes (GPS + GLONASS, peut-etre Galileo)
 *                  reconnaissable aux trames $GNGGA/$GNRMC (prefixe GN = Generic Navigation).
 *                  Pas un simple NEO-6M (qui envoie $GP* a 9600 bauds).
 * 
 * Branchement (header "GPIO" PH2.0 de la Waveshare 7B) :
 *   VCC (rouge) -> 3V3
 *   GND (noir)  -> GND
 *   TX  (jaune) -> ESP32 RX (GPS_RX_PIN = GPIO 6, broche GP6 du header)
 *   RX  (blanc) -> non connecte (le module n'a pas besoin d'etre configure)
 * 
 * Protocole NMEA 0183 a 115200 bauds
 * Trames parsees : $GNGGA (position + altitude + nb satellites)
 *                  $GNRMC (position + vitesse + heure)
 *                  Le parser accepte automatiquement $GP*, $GN*, $GL*, $GA*
 */
#include <Arduino.h>
#include <stdint.h>

// ── GPIO configurables (à adapter selon ton câblage) ─────
// Sur la Waveshare ESP32-S3-Touch-LCD-7B (1024x600), beaucoup de pins
// sont occupes par l'ecran RGB et autres bus :
//   - GPIO 17/18    = bus RGB ecran (B6/B5)            -> INTERDITS
//   - GPIO 15/16    = via transceiver SP3485 (RS-485)  -> pas du TTL pur
//   - GPIO 11/12/13 = SD card                          -> INTERDITS
//   - GPIO 8/9      = I2C touch + IO expander          -> INTERDITS
//   - GPIO 19/20    = USB-OTG / CAN via TJA1051        -> pas utilisable simple
//   - GPIO 43/44    = UART debug (CH343P, port USB-C "UART1")
//
// Pin libre confirmee : GPIO 6 (header "GPIO" PH2.0 = 3V3 / GND / GP6)
// Module utilise : GNSS multi-constellations (envoie $GNGGA, $GNRMC) a 115200 bauds
//
// Les valeurs sont normalement definies dans config_base.h (qui est inclus
// AVANT gps.h dans GG_Step22.ino) ; les #ifndef ci-dessous servent juste de
// fallback si quelqu'un compile gps.cpp sans config_base.h.
#ifndef GPS_RX_PIN
  #define GPS_RX_PIN  6        // RX ESP32 <- TX du GPS (fil jaune, header GPIO PH2.0 broche GP6)
#endif
#ifndef GPS_TX_PIN
  #define GPS_TX_PIN  -1       // TX ESP32 -> RX du GPS (non utilise : module n'a pas besoin d'etre configure)
#endif
#ifndef GPS_BAUD
  #define GPS_BAUD    115200   // Module GNSS multi-constellations (pas un NEO-6M classique a 9600)
#endif

struct GpsData {
    bool     valid;                 // position 2D/3D valide
    bool     has_fix;
    uint32_t last_update_ms;
    
    float    latitude;              // degrés décimaux (positif = Nord)
    float    longitude;             // degrés décimaux (positif = Est)
    float    altitude_m;            // mètres au-dessus du niveau de la mer
    float    speed_kmh;             // vitesse au sol
    float    course_deg;            // cap (0 = nord, 90 = est)
    
    int      satellites;            // nb satellites utilisés
    float    hdop;                  // precision horizontale
    
    // Heure GPS
    int      hour_utc;
    int      minute_utc;
    int      second_utc;
    int      day;
    int      month;
    int      year;
};

extern GpsData gpsData;

void gps_init();
void gps_update();                    // à appeler dans la loop

// Utilitaires
bool gps_is_available();              // true si fix récent (<10s)
float gps_distance_km(float lat1, float lon1, float lat2, float lon2);
float gps_bearing_deg(float lat1, float lon1, float lat2, float lon2);
