#pragma once
/*
 * france_map.h — Widget carte France simplifiee pour LVGL
 * 
 * Dessine les contours simplifies de la France metropolitaine
 * + un point a la position GPS courante
 * + click -> popup avec lat/lon exactes
 */
#include <lvgl.h>

// Creer la carte France dans un parent LVGL
// Retourne le conteneur
lv_obj_t* france_map_create(lv_obj_t* parent, lv_coord_t w, lv_coord_t h);

// Mettre a jour la position GPS (lat/lon degres decimaux)
void france_map_update_position(float lat, float lon, bool has_fix);

// Afficher/masquer popup info
void france_map_show_popup(bool show);
