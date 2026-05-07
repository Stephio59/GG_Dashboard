/*
 * page_home.cpp - Page Accueil v2 (redesign Step 18)
 * 
 * Layout :
 *   - Carte GPS XXL a gauche (cliquable -> page detail)
 *   - Horloge + Date en haut a droite
 *   - Bloc Interieur compact (cliquable -> page detail)
 *   - Bloc Meteo 3 jours
 *   - Widget eau compact en bas
 */
#include "ui.h"
#include "lv_fonts_fr.h"
#include "icons_van.h"
#include "display_config.h"
#include "bme_wire1.h"
#include "scd41.h"
#include "weather.h"
#include "gps.h"
#include "france_map.h"
#include "modal_pages.h"
#include "sd_card.h"
#include "sd_lvgl_fs.h"
#include "icons_sd.h"
#include "weather_icons.h"
#include "water_state.h"
#include "espnow_master.h"
#include <math.h>
#include <time.h>

#ifndef NAV_HEIGHT
#define NAV_HEIGHT  80
#endif
#ifndef SB_HEIGHT
#define SB_HEIGHT   36
#endif

extern UiManager ui;

// ─── Widgets ─────────────────────────────────
static lv_obj_t* lbl_clock     = nullptr;
static lv_obj_t* lbl_date      = nullptr;
static lv_obj_t* lbl_temp      = nullptr;
static lv_obj_t* lbl_hum       = nullptr;
static lv_obj_t* lbl_pres      = nullptr;
static lv_obj_t* lbl_co2       = nullptr;
static lv_obj_t* lbl_alerts    = nullptr;

// Météo
static lv_obj_t* lbl_w_today_t = nullptr;
static lv_obj_t* lbl_w_today_d = nullptr;
static lv_obj_t* lbl_w_tom_t   = nullptr;
static lv_obj_t* lbl_w_tom_d   = nullptr;
static lv_obj_t* lbl_w_after_t = nullptr;
static lv_obj_t* lbl_w_after_d = nullptr;
static lv_obj_t* lbl_w_status  = nullptr;
// Icones meteo (BMP depuis SD)
static lv_obj_t* img_w_today   = nullptr;
static lv_obj_t* img_w_tom     = nullptr;
static lv_obj_t* img_w_after   = nullptr;

// GPS
static lv_obj_t* gps_map_widget = nullptr;
static lv_obj_t* dot_position   = nullptr;
static lv_obj_t* lbl_gps_info   = nullptr;

// Eau
static lv_obj_t* bar_clean      = nullptr;
static lv_obj_t* bar_dirty      = nullptr;
static lv_obj_t* lbl_clean_pct  = nullptr;
static lv_obj_t* lbl_dirty_pct  = nullptr;

// ─── Callbacks clic ─────────────────────────
static void gps_map_clicked(lv_event_t* e) {
    modal_gps_open();
}

static void interior_clicked(lv_event_t* e) {
    modal_interior_open();
}

static void weather_today_cb(lv_event_t* e) {
    modal_weather7_open();  // Direct sur 7 jours (Open-Meteo)
}
static void weather_tomorrow_cb(lv_event_t* e) {
    modal_weather7_open();
}
static void weather_after_cb(lv_event_t* e) {
    modal_weather7_open();
}

// Helper carte
static lv_obj_t* make_card(lv_obj_t* parent, int x, int y, int w, int h) {
    if (!parent) return nullptr;
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void page_home_build(lv_obj_t* parent) {
    if (!parent) return;
    
    ui_draw_bg(parent);

    // ─── Layout gauche : Carte GPS XXL ───
    int gps_x = 16;
    int gps_y = 50;
    int gps_w = 510;
    int gps_h = 420;   // +60px pour profiter de la carte plus grande

    lv_obj_t* gps_card = make_card(parent, gps_x, gps_y, gps_w, gps_h);
    if (!gps_card) return;
    lv_obj_add_flag(gps_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(gps_card, gps_map_clicked, LV_EVENT_CLICKED, nullptr);

    // Titre carte GPS
    lv_obj_t* lbl_gps_title = lv_label_create(gps_card);
    lv_label_set_text(lbl_gps_title, LV_SYMBOL_GPS "  Position GPS");
    lv_obj_set_style_text_font(lbl_gps_title, &FONT_18, 0);
    lv_obj_set_style_text_color(lbl_gps_title, COLOR_GRAY, 0);
    lv_obj_align(lbl_gps_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* lbl_tap = lv_label_create(gps_card);
    lv_label_set_text(lbl_tap, "(Tapez pour details)");
    lv_obj_set_style_text_font(lbl_tap, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_tap, COLOR_GRAY, 0);
    lv_obj_align(lbl_tap, LV_ALIGN_TOP_RIGHT, 0, 4);

    // Carte France réelle - on utilise PRESQUE toute la zone GPS
    int map_inner_w = gps_w - 20;     // marge plus petite
    int map_inner_h = gps_h - 60;     // garder juste assez pour titre + label
    
    // Tenter de charger la belle carte BMP depuis la SD
    gps_map_widget = nullptr;
    if (sdCard.mounted) {
        lv_img_dsc_t* france_bmp = sd_load_bmp("/images/france_map.bmp");
        if (france_bmp) {
            gps_map_widget = lv_img_create(gps_card);
            lv_img_set_src(gps_map_widget, france_bmp);
            
            // Calcul scale pour REMPLIR au max la zone
            int sx = (map_inner_w * 256) / france_bmp->header.w;
            int sy = (map_inner_h * 256) / france_bmp->header.h;
            int scale = (sx < sy) ? sx : sy;
            
            // On veut maximiser, mais sans pixelliser trop : max 200%
            if (scale > 512) scale = 512;
            
            lv_img_set_zoom(gps_map_widget, scale);
            lv_obj_align(gps_map_widget, LV_ALIGN_CENTER, 0, 6);
            lv_obj_clear_flag(gps_map_widget, LV_OBJ_FLAG_CLICKABLE);
            Serial0.printf("[Home] Carte BMP chargee depuis SD (scale=%d/256)\n", scale);
        }
    }
    
    // Fallback: ancien dessin vectoriel si BMP indisponible
    if (!gps_map_widget) {
        gps_map_widget = france_map_create(gps_card, map_inner_w, map_inner_h);
        if (gps_map_widget) {
            lv_obj_set_pos(gps_map_widget, (gps_w - map_inner_w) / 2 - 8, 32);
        }
    }

    // Dot position
    dot_position = lv_obj_create(gps_card);
    lv_obj_set_size(dot_position, 14, 14);
    lv_obj_set_style_bg_color(dot_position, COLOR_RED, 0);
    lv_obj_set_style_bg_opa(dot_position, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dot_position, lv_color_white(), 0);
    lv_obj_set_style_border_width(dot_position, 2, 0);
    lv_obj_set_style_radius(dot_position, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_flag(dot_position, LV_OBJ_FLAG_HIDDEN);

    // Info GPS en bas (lat/lon ou statut)
    lbl_gps_info = lv_label_create(gps_card);
    lv_label_set_text(lbl_gps_info, "Recherche signal GPS...");
    lv_obj_set_style_text_font(lbl_gps_info, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_gps_info, COLOR_YELLOW, 0);
    lv_obj_align(lbl_gps_info, LV_ALIGN_BOTTOM_MID, 0, 0);

    // ─── Cote droit : 4 cards verticales ───
    int right_x = gps_x + gps_w + 16;
    int right_w = SCREEN_WIDTH - right_x - 16;

    // Card 1 : Horloge + date
    int h_clk = 90;
    lv_obj_t* card_clock = make_card(parent, right_x, gps_y, right_w, h_clk);
    lv_obj_set_style_pad_all(card_clock, 12, 0);

    lbl_clock = lv_label_create(card_clock);
    lv_label_set_text(lbl_clock, "--:--");
    lv_obj_set_style_text_font(lbl_clock, &FONT_48, 0);
    lv_obj_set_style_text_color(lbl_clock, lv_color_white(), 0);
    lv_obj_align(lbl_clock, LV_ALIGN_LEFT_MID, 0, -8);

    lbl_date = lv_label_create(card_clock);
    lv_label_set_text(lbl_date, "--");
    lv_obj_set_style_text_font(lbl_date, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_date, COLOR_GRAY, 0);
    lv_obj_align(lbl_date, LV_ALIGN_RIGHT_MID, 0, 0);

    // Card 2 : Interieur (CLIQUABLE)
    int h_int = 130;
    int y_int = gps_y + h_clk + 10;
    lv_obj_t* card_int = make_card(parent, right_x, y_int, right_w, h_int);
    lv_obj_set_style_pad_all(card_int, 10, 0);
    lv_obj_add_flag(card_int, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card_int, interior_clicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_int_title = lv_label_create(card_int);
    lv_label_set_text(lbl_int_title, LV_SYMBOL_HOME "  Interieur");
    lv_obj_set_style_text_font(lbl_int_title, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_int_title, COLOR_GRAY, 0);
    lv_obj_align(lbl_int_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* lbl_arrow = lv_label_create(card_int);
    lv_label_set_text(lbl_arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(lbl_arrow, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_arrow, COLOR_GRAY, 0);
    lv_obj_align(lbl_arrow, LV_ALIGN_TOP_RIGHT, 0, 0);

    lbl_temp = lv_label_create(card_int);
    lv_label_set_text(lbl_temp, "-- °C");
    lv_obj_set_style_text_font(lbl_temp, &FONT_28, 0);
    lv_obj_set_style_text_color(lbl_temp, COLOR_ORANGE, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_LEFT_MID, 0, 4);

    lbl_hum = lv_label_create(card_int);
    lv_label_set_text(lbl_hum, "--%");
    lv_obj_set_style_text_font(lbl_hum, &FONT_18, 0);
    lv_obj_set_style_text_color(lbl_hum, COLOR_BLUE, 0);
    lv_obj_align(lbl_hum, LV_ALIGN_RIGHT_MID, 0, -4);

    lbl_co2 = lv_label_create(card_int);
    lv_label_set_text(lbl_co2, "CO2 -- ppm");
    lv_obj_set_style_text_font(lbl_co2, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_co2, COLOR_GRAY, 0);
    lv_obj_align(lbl_co2, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lbl_pres = lv_label_create(card_int);
    lv_label_set_text(lbl_pres, "-- hPa");
    lv_obj_set_style_text_font(lbl_pres, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_pres, COLOR_GRAY, 0);
    lv_obj_align(lbl_pres, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    // Card 3 : Meteo 3 jours
    int h_w = 200;   // +50 pour mieux voir icones meteo
    int y_w = y_int + h_int + 10;
    lv_obj_t* card_w = make_card(parent, right_x, y_w, right_w, h_w);
    lv_obj_set_style_pad_all(card_w, 10, 0);
    lv_obj_add_flag(card_w, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card_w, weather_today_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_w_title = lv_label_create(card_w);
    lv_label_set_text(lbl_w_title, "Meteo 3 jours " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(lbl_w_title, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_w_title, COLOR_GRAY, 0);
    lv_obj_align(lbl_w_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_w_status = lv_label_create(card_w);
    lv_label_set_text(lbl_w_status, "Hors ligne");
    lv_obj_set_style_text_font(lbl_w_status, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_w_status, COLOR_GRAY, 0);
    lv_obj_align(lbl_w_status, LV_ALIGN_TOP_RIGHT, 0, 0);

    // === 3 colonnes propres : pour chaque jour, layout vertical : ===
    //   Ligne 1 : "Auj." / "Dem." / "Apr."   (label gris, FONT_14)
    //   Ligne 2 : icone meteo 40px           (BMP)
    //   Ligne 3 : "21°"                      (FONT_22 blanc)
    //   Ligne 4 : "Nuageux"                  (FONT_14 gris)
    int col_w = (right_w - 30) / 3;
    int col_y = 26;
    int icon_size = 40;  // 64*160/256 = 40px
    
    const lv_img_dsc_t* def_w = icons_sd_get("weather_sunny");
    
    // ====== AUJOURD'HUI ======
    int x1 = 8;
    lv_obj_t* lbl_h1 = lv_label_create(card_w);
    lv_label_set_text(lbl_h1, "Aujourd'hui");
    lv_obj_set_style_text_font(lbl_h1, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_h1, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_h1, x1, col_y);
    
    img_w_today = lv_img_create(card_w);
    lv_img_set_zoom(img_w_today, 160);
    lv_obj_set_pos(img_w_today, x1 + (col_w - icon_size)/2 - 4, col_y + 18);
    lv_obj_clear_flag(img_w_today, LV_OBJ_FLAG_CLICKABLE);
    if (def_w) lv_img_set_src(img_w_today, def_w);
    
    lbl_w_today_t = lv_label_create(card_w);
    lv_label_set_text(lbl_w_today_t, "--°");
    lv_obj_set_style_text_font(lbl_w_today_t, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_w_today_t, lv_color_white(), 0);
    lv_obj_set_pos(lbl_w_today_t, x1, col_y + 64);
    
    lbl_w_today_d = lv_label_create(card_w);
    lv_label_set_text(lbl_w_today_d, "--");
    lv_obj_set_style_text_font(lbl_w_today_d, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_w_today_d, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_w_today_d, x1, col_y + 92);
    
    // ====== DEMAIN ======
    int x2 = 8 + col_w;
    lv_obj_t* lbl_h2 = lv_label_create(card_w);
    lv_label_set_text(lbl_h2, "Demain");
    lv_obj_set_style_text_font(lbl_h2, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_h2, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_h2, x2, col_y);
    
    img_w_tom = lv_img_create(card_w);
    lv_img_set_zoom(img_w_tom, 160);
    lv_obj_set_pos(img_w_tom, x2 + (col_w - icon_size)/2 - 4, col_y + 18);
    lv_obj_clear_flag(img_w_tom, LV_OBJ_FLAG_CLICKABLE);
    if (def_w) lv_img_set_src(img_w_tom, def_w);
    
    lbl_w_tom_t = lv_label_create(card_w);
    lv_label_set_text(lbl_w_tom_t, "--°");
    lv_obj_set_style_text_font(lbl_w_tom_t, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_w_tom_t, lv_color_white(), 0);
    lv_obj_set_pos(lbl_w_tom_t, x2, col_y + 64);
    
    lbl_w_tom_d = lv_label_create(card_w);
    lv_label_set_text(lbl_w_tom_d, "--");
    lv_obj_set_style_text_font(lbl_w_tom_d, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_w_tom_d, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_w_tom_d, x2, col_y + 92);
    
    // ====== APRES-DEMAIN ======
    int x3 = 8 + 2*col_w;
    lv_obj_t* lbl_h3 = lv_label_create(card_w);
    lv_label_set_text(lbl_h3, "Apres-dem.");
    lv_obj_set_style_text_font(lbl_h3, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_h3, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_h3, x3, col_y);
    
    img_w_after = lv_img_create(card_w);
    lv_img_set_zoom(img_w_after, 160);
    lv_obj_set_pos(img_w_after, x3 + (col_w - icon_size)/2 - 4, col_y + 18);
    lv_obj_clear_flag(img_w_after, LV_OBJ_FLAG_CLICKABLE);
    if (def_w) lv_img_set_src(img_w_after, def_w);
    
    lbl_w_after_t = lv_label_create(card_w);
    lv_label_set_text(lbl_w_after_t, "--°");
    lv_obj_set_style_text_font(lbl_w_after_t, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_w_after_t, lv_color_white(), 0);
    lv_obj_set_pos(lbl_w_after_t, x3, col_y + 64);
    
    lbl_w_after_d = lv_label_create(card_w);
    lv_label_set_text(lbl_w_after_d, "--");
    lv_obj_set_style_text_font(lbl_w_after_d, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_w_after_d, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_w_after_d, x3, col_y + 92);

    // ─── BAS : Widget eau compact pleine largeur ───
    int water_y = gps_y + gps_h + 10;
    int water_h = SCREEN_HEIGHT - water_y - 10 - NAV_HEIGHT;  // au dessus de la nav
    if (water_h < 110) water_h = 110;
    
    lv_obj_t* card_water = make_card(parent, gps_x, water_y, SCREEN_WIDTH - 32, water_h);
    lv_obj_set_style_pad_all(card_water, 12, 0);

    lv_obj_t* lbl_water_title = lv_label_create(card_water);
    lv_label_set_text(lbl_water_title, "Reservoirs eau");
    lv_obj_set_style_text_font(lbl_water_title, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_water_title, COLOR_GRAY, 0);
    lv_obj_align(lbl_water_title, LV_ALIGN_TOP_LEFT, 0, 0);

    // Layout 2 colonnes : eau propre + eau sale
    int half = (SCREEN_WIDTH - 60) / 2;
    
    // Eau propre (gauche)
    lv_obj_t* lbl_clean_t = lv_label_create(card_water);
    lv_label_set_text(lbl_clean_t, "Eau propre");
    lv_obj_set_style_text_font(lbl_clean_t, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_clean_t, COLOR_BLUE, 0);
    lv_obj_align(lbl_clean_t, LV_ALIGN_TOP_LEFT, 0, 26);
    
    lbl_clean_pct = lv_label_create(card_water);
    lv_label_set_text(lbl_clean_pct, "--%");
    lv_obj_set_style_text_font(lbl_clean_pct, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_clean_pct, lv_color_white(), 0);
    lv_obj_align(lbl_clean_pct, LV_ALIGN_TOP_LEFT, 0, 44);

    bar_clean = lv_bar_create(card_water);
    lv_obj_set_size(bar_clean, half - 100, 16);
    lv_obj_align(bar_clean, LV_ALIGN_TOP_LEFT, 110, 50);
    lv_bar_set_range(bar_clean, 0, 100);
    lv_bar_set_value(bar_clean, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_clean, lv_color_hex(0x21262D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_clean, COLOR_BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_clean, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_clean, 8, LV_PART_INDICATOR);

    // Eau sale (droite)
    lv_obj_t* lbl_dirty_t = lv_label_create(card_water);
    lv_label_set_text(lbl_dirty_t, "Eau sale");
    lv_obj_set_style_text_font(lbl_dirty_t, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_dirty_t, COLOR_ORANGE, 0);
    lv_obj_align(lbl_dirty_t, LV_ALIGN_TOP_LEFT, half + 30, 26);
    
    lbl_dirty_pct = lv_label_create(card_water);
    lv_label_set_text(lbl_dirty_pct, "--%");
    lv_obj_set_style_text_font(lbl_dirty_pct, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_dirty_pct, lv_color_white(), 0);
    lv_obj_align(lbl_dirty_pct, LV_ALIGN_TOP_LEFT, half + 30, 44);

    bar_dirty = lv_bar_create(card_water);
    lv_obj_set_size(bar_dirty, half - 100, 16);
    lv_obj_align(bar_dirty, LV_ALIGN_TOP_LEFT, half + 140, 50);
    lv_bar_set_range(bar_dirty, 0, 100);
    lv_bar_set_value(bar_dirty, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_dirty, lv_color_hex(0x21262D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_dirty, COLOR_ORANGE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_dirty, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_dirty, 8, LV_PART_INDICATOR);

    // Alertes (en bas du widget eau)
    lbl_alerts = lv_label_create(card_water);
    lv_label_set_text(lbl_alerts, LV_SYMBOL_OK "  Tout est OK");
    lv_obj_set_style_text_font(lbl_alerts, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_alerts, COLOR_GREEN, 0);
    lv_obj_align(lbl_alerts, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

void page_home_update() {
    if (!lbl_clock) return;
    char buf[64];

    // Horloge
    time_t now;
    time(&now);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    if (tm_local.tm_year >= (2024 - 1900)) {
        snprintf(buf, sizeof(buf), "%02d:%02d", tm_local.tm_hour, tm_local.tm_min);
        lv_label_set_text(lbl_clock, buf);
        const char* jours[] = {"Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam."};
        const char* mois[]  = {"Jan", "Fev", "Mar", "Avr", "Mai", "Juin", "Juil", "Aou", "Sep", "Oct", "Nov", "Dec"};
        snprintf(buf, sizeof(buf), "%s %d %s", jours[tm_local.tm_wday], tm_local.tm_mday, mois[tm_local.tm_mon]);
        lv_label_set_text(lbl_date, buf);
    }

    // Interieur
    if (bmeData.valid) {
        snprintf(buf, sizeof(buf), "%.1f °C", bmeData.temperature);
        lv_label_set_text(lbl_temp, buf);
        snprintf(buf, sizeof(buf), "%.0f%%", bmeData.humidity);
        lv_label_set_text(lbl_hum, buf);
        snprintf(buf, sizeof(buf), "%.0f hPa", bmeData.pressure);
        lv_label_set_text(lbl_pres, buf);
    }
    if (scd41Data.valid) {
        snprintf(buf, sizeof(buf), "CO2 %d ppm", scd41Data.co2_ppm);
        lv_label_set_text(lbl_co2, buf);
        if (scd41Data.co2_ppm < 800)
            lv_obj_set_style_text_color(lbl_co2, COLOR_GREEN, 0);
        else if (scd41Data.co2_ppm < 1500)
            lv_obj_set_style_text_color(lbl_co2, COLOR_YELLOW, 0);
        else
            lv_obj_set_style_text_color(lbl_co2, COLOR_RED, 0);
    }

    // Meteo
    if (weatherData.valid) {
        lv_label_set_text(lbl_w_status, "En ligne");
        lv_obj_set_style_text_color(lbl_w_status, COLOR_GREEN, 0);
        snprintf(buf, sizeof(buf), "%.0f°", weatherData.today.temp_c);
        lv_label_set_text(lbl_w_today_t, buf);
        lv_label_set_text(lbl_w_today_d, weatherData.today.condition);
        snprintf(buf, sizeof(buf), "%.0f°", weatherData.tomorrow.temp_c);
        lv_label_set_text(lbl_w_tom_t, buf);
        lv_label_set_text(lbl_w_tom_d, weatherData.tomorrow.condition);
        snprintf(buf, sizeof(buf), "%.0f°", weatherData.after.temp_c);
        lv_label_set_text(lbl_w_after_t, buf);
        lv_label_set_text(lbl_w_after_d, weatherData.after.condition);
        
        // Mettre a jour les icones meteo BMP
        if (img_w_today) {
            const lv_img_dsc_t* ic = icons_sd_get(owm_to_icon(weatherData.today.icon));
            if (ic) lv_img_set_src(img_w_today, ic);
        }
        if (img_w_tom) {
            const lv_img_dsc_t* ic = icons_sd_get(owm_to_icon(weatherData.tomorrow.icon));
            if (ic) lv_img_set_src(img_w_tom, ic);
        }
        if (img_w_after) {
            const lv_img_dsc_t* ic = icons_sd_get(owm_to_icon(weatherData.after.icon));
            if (ic) lv_img_set_src(img_w_after, ic);
        }
    } else {
        lv_label_set_text(lbl_w_status, "Hors ligne");
        lv_obj_set_style_text_color(lbl_w_status, COLOR_GRAY, 0);
    }

    // GPS
    if (gpsData.has_fix) {
        snprintf(buf, sizeof(buf), "Lat: %.4f°  Lon: %.4f°  Sat: %d",
                 gpsData.latitude, gpsData.longitude, gpsData.satellites);
        lv_label_set_text(lbl_gps_info, buf);
        lv_obj_set_style_text_color(lbl_gps_info, COLOR_GREEN, 0);
        
        if (gpsData.latitude >= 41.0 && gpsData.latitude <= 51.5 && 
            gpsData.longitude >= -5.0 && gpsData.longitude <= 9.5 && gps_map_widget) {
            int map_x_obj = lv_obj_get_x(gps_map_widget);
            int map_y_obj = lv_obj_get_y(gps_map_widget);
            int map_w_obj = lv_obj_get_width(gps_map_widget);
            int map_h_obj = lv_obj_get_height(gps_map_widget);
            int dx = (int)((gpsData.longitude - (-5.0)) / 14.5 * map_w_obj);
            int dy = (int)((51.5 - gpsData.latitude) / 10.5 * map_h_obj);
            lv_obj_set_pos(dot_position, map_x_obj + dx - 7, map_y_obj + dy - 7);
            lv_obj_clear_flag(dot_position, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_label_set_text(lbl_gps_info, "Recherche signal GPS...");
        lv_obj_set_style_text_color(lbl_gps_info, COLOR_YELLOW, 0);
    }

    // Eau (valeurs simulees si pas de capteur connecte)
    int clean = (int)waterState.clean_pct;
    int dirty = (int)waterState.dirty_pct;
    
    // Si on n'a pas de donnees du slave (pas connecte), simuler
    if (!slaveOnline) {
        // Simulation : niveau qui varie avec le temps pour rendre l'UI vivante
        uint32_t t_min = millis() / 60000;
        clean = 70 + (int)(20 * sin(t_min * 0.05));  // oscille 50-90%
        dirty = 35 + (int)(15 * cos(t_min * 0.03));  // oscille 20-50%
    }
    
    snprintf(buf, sizeof(buf), "%d%%", clean);
    lv_label_set_text(lbl_clean_pct, buf);
    snprintf(buf, sizeof(buf), "%d%%", dirty);
    lv_label_set_text(lbl_dirty_pct, buf);
    lv_bar_set_value(bar_clean, clean, LV_ANIM_OFF);
    lv_bar_set_value(bar_dirty, dirty, LV_ANIM_OFF);
    
    // Couleur dynamique de la barre eau propre (vert > 50%, jaune 20-50%, rouge < 20%)
    lv_color_t clean_color;
    if (clean > 50) clean_color = COLOR_GREEN;
    else if (clean > 20) clean_color = COLOR_YELLOW;
    else clean_color = COLOR_RED;
    lv_obj_set_style_bg_color(bar_clean, clean_color, LV_PART_INDICATOR);
    
    // Couleur dynamique eau sale (vert < 50%, jaune 50-80%, rouge > 80%)
    lv_color_t dirty_color;
    if (dirty < 50) dirty_color = COLOR_GREEN;
    else if (dirty < 80) dirty_color = COLOR_YELLOW;
    else dirty_color = COLOR_RED;
    lv_obj_set_style_bg_color(bar_dirty, dirty_color, LV_PART_INDICATOR);
    
    // Alertes regroupees
    if (clean < 20) {
        lv_label_set_text(lbl_alerts, LV_SYMBOL_WARNING "  Eau propre basse !");
        lv_obj_set_style_text_color(lbl_alerts, COLOR_ORANGE, 0);
    } else if (dirty > 80) {
        lv_label_set_text(lbl_alerts, LV_SYMBOL_WARNING "  Eau sale presque pleine !");
        lv_obj_set_style_text_color(lbl_alerts, COLOR_ORANGE, 0);
    } else if (scd41Data.valid && scd41Data.co2_ppm > 1500) {
        lv_label_set_text(lbl_alerts, LV_SYMBOL_WARNING "  CO2 eleve - aerez !");
        lv_obj_set_style_text_color(lbl_alerts, COLOR_ORANGE, 0);
    } else {
        lv_label_set_text(lbl_alerts, LV_SYMBOL_OK "  Tout est OK");
        lv_obj_set_style_text_color(lbl_alerts, COLOR_GREEN, 0);
    }
}
