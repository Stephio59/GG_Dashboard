#pragma once
#include "espnow_protocol.h"

// Init ESP-NOW (Waveshare = master)
void espnow_master_init();

// Envoyer la commande des relais au slave
//  bitmask : etat des 8 relais (bit 0 = relais 1, ..., bit 7 = relais 8)
bool espnow_send_relays(uint8_t bitmask);

// Envoyer un ping (keepalive)
void espnow_send_ping();

// Acces aux dernieres donnees recues
extern StatusMsg statusFromSlave;
extern bool      statusValid;
extern uint32_t  statusLastUpdateMs;
extern bool      slaveOnline;

