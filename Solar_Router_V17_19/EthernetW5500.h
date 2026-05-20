// ============================================
// F1ATB Maison - Ethernet W5500
// Gestion connexion + interruption INT
// ============================================

#ifndef ETHERNET_W5500_H
#define ETHERNET_W5500_H

#include <Ethernet.h>
#include "GpioPinConfig.h"

// ============================================
// CONFIGURATION RÉSEAU
// ============================================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // À personnaliser
IPAddress ip(192, 168, 1, 177);                          // IP statique (optionnel DHCP)

EthernetClient ethClient;

// Flag mis à true par l'ISR quand un paquet arrive
volatile bool w5500PacketReceived = false;

// ============================================
// ISR - Interruption W5500
// Appelée automatiquement quand INT passe LOW
// ============================================
void IRAM_ATTR onW5500Interrupt() {
    w5500PacketReceived = true;
}

// ============================================
// INIT ETHERNET
// ============================================
void initEthernet() {
    Ethernet.init(GPIO_W5500_CS);

    Serial.println("[ETH] Démarrage W5500...");

    if (Ethernet.begin(mac) == 0) {
        Serial.println("[ETH] DHCP échoué, utilisation IP statique");
        Ethernet.begin(mac, ip);
    }

    delay(500);

    Serial.print("[ETH] IP : ");
    Serial.println(Ethernet.localIP());

    // Activation de l'interruption W5500 sur GPIO 25 (INT actif bas)
    pinMode(GPIO_W5500_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(GPIO_W5500_INT), onW5500Interrupt, FALLING);

    Serial.println("[ETH] Interruption W5500 INT activée sur GPIO 25");
}

// ============================================
// LOOP - À appeler dans loop()
// Traite les paquets dès que W5500 signale
// ============================================
void handleEthernet() {
    if (w5500PacketReceived) {
        w5500PacketReceived = false;
        Ethernet.maintain();  // Renouvellement DHCP si nécessaire
        // Ici : traite