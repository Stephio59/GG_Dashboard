/*
 * sd_lvgl_fs.h - Driver LVGL filesystem pour la carte SD
 * 
 * Permet a LVGL de lire des fichiers depuis SD via la lettre 'S'
 * Exemple usage :
 *   lv_img_set_src(img, "S:/images/france_map.bmp");
 */
#pragma once
#include <lvgl.h>

// A appeler une fois apres lv_init() et apres sd_card_init()
void sd_lvgl_fs_init();

// Decodeur BMP : lit un fichier BMP depuis SD et alloue un lv_img_dsc_t
// IMPORTANT: l'image est allouee en PSRAM, ne pas oublier de liberer
// path : chemin sur la SD (ex: "/icons/home.bmp")
// Retourne nullptr si erreur
lv_img_dsc_t* sd_load_bmp(const char* path);

// Liberer une image chargee par sd_load_bmp
void sd_free_bmp(lv_img_dsc_t* img);
