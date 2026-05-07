/*
 * modal_pages.h - Pages detail en mode "modal" (creees a la demande)
 */
#pragma once

void modal_gps_open();
void modal_interior_open();
void modal_weather_open(int day);  // 0=auj, 1=dem, 2=apr
void modal_weather7_open();        // 7 jours Open-Meteo
void modal_update();
bool modal_is_open();
