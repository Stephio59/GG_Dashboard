/*
 * france_map.cpp — Carte France LVGL avec point GPS
 * 
 * Utilise lv_line pour dessiner les contours (pas de canvas, plus compatible)
 * + un point pour la position GPS + popup cliquable
 */
#include "france_map.h"
#include "ui.h"
#include <Arduino.h>
#include <stdio.h>
#include <math.h>
#include <esp_heap_caps.h>

// ── Données contour France (131 points lat, lon) - Natural Earth Data simplifié ──
static const float FRANCE_CONTOUR[][2] = {
    // Cote opale (Dunkerque -> Le Havre)
    { 51.05f,  2.55f }, { 51.00f,  2.20f }, { 50.90f,  1.85f }, { 50.65f,  1.60f },
    { 50.30f,  1.55f }, { 50.10f,  1.40f }, { 49.90f,  1.10f }, { 49.70f,  0.80f },
    { 49.65f,  0.25f }, { 49.40f, -0.10f }, { 49.30f, -0.50f }, { 49.40f, -1.10f },
    // Cotentin
    { 49.65f, -1.60f }, { 49.70f, -1.85f }, { 49.55f, -1.95f }, { 49.20f, -1.55f },
    { 48.85f, -1.40f }, { 48.65f, -1.95f }, { 48.55f, -2.50f },
    // Cotes Bretagne nord & ouest
    { 48.65f, -3.05f }, { 48.70f, -3.55f }, { 48.55f, -4.10f }, { 48.45f, -4.65f },
    { 48.35f, -4.77f }, { 48.10f, -4.55f }, { 47.95f, -4.37f }, { 47.75f, -4.05f },
    { 47.55f, -3.55f }, { 47.50f, -3.10f }, { 47.55f, -2.85f }, { 47.45f, -2.55f },
    // Loire estuaire / Vendee
    { 47.25f, -2.20f }, { 47.10f, -2.15f }, { 46.85f, -2.15f }, { 46.70f, -1.95f },
    { 46.50f, -1.80f }, { 46.25f, -1.50f }, { 46.00f, -1.20f }, { 45.85f, -1.15f },
    // La Rochelle / Royan
    { 45.65f, -1.05f }, { 45.55f, -1.15f }, { 45.40f, -1.10f }, { 45.20f, -0.95f },
    { 45.05f, -1.05f }, { 44.85f, -1.20f },
    // Bassin Arcachon / Landes
    { 44.65f, -1.25f }, { 44.50f, -1.30f }, { 44.30f, -1.30f }, { 44.05f, -1.40f },
    { 43.80f, -1.45f }, { 43.55f, -1.55f },
    // Cote basque + Pyrenees
    { 43.40f, -1.75f }, { 43.30f, -1.65f }, { 43.10f, -1.45f }, { 42.95f, -1.35f },
    { 42.85f, -0.75f }, { 42.78f,  0.30f }, { 42.70f,  0.65f }, { 42.60f,  1.05f },
    { 42.55f,  1.45f }, { 42.45f,  1.85f }, { 42.40f,  2.25f }, { 42.45f,  2.55f },
    // Cote vermeille / Languedoc
    { 42.45f,  3.05f }, { 42.55f,  3.15f }, { 42.85f,  3.05f }, { 43.10f,  3.00f },
    { 43.30f,  3.45f }, { 43.40f,  4.10f }, { 43.40f,  4.85f },
    // Marseille / Var
    { 43.35f,  5.35f }, { 43.20f,  5.55f }, { 43.10f,  5.90f }, { 43.05f,  6.20f },
    { 43.10f,  6.60f }, { 43.25f,  6.85f }, { 43.40f,  6.90f }, { 43.55f,  7.10f },
    // Nice / Mercantour
    { 43.65f,  7.30f }, { 43.75f,  7.45f }, { 44.05f,  7.20f }, { 44.20f,  7.00f },
    // Alpes (Italie -> Suisse)
    { 44.55f,  6.95f }, { 44.85f,  6.95f }, { 45.05f,  6.75f }, { 45.15f,  6.65f },
    { 45.30f,  6.90f }, { 45.40f,  6.95f }, { 45.65f,  6.95f }, { 45.85f,  6.95f },
    // Mont-Blanc / Geneve
    { 45.95f,  6.85f }, { 46.05f,  6.45f }, { 46.10f,  6.15f }, { 46.20f,  6.05f },
    { 46.30f,  6.00f }, { 46.40f,  6.15f }, { 46.55f,  6.30f }, { 46.70f,  6.85f },
    // Jura / Alsace
    { 46.90f,  6.95f }, { 47.15f,  7.00f }, { 47.30f,  7.05f }, { 47.45f,  7.15f },
    { 47.55f,  7.55f }, { 47.85f,  7.55f }, { 48.10f,  7.55f }, { 48.40f,  7.65f },
    { 48.65f,  7.80f }, { 48.95f,  8.20f }, { 49.05f,  8.20f }, { 49.15f,  8.15f },
    // Lorraine / Luxembourg
    { 49.30f,  8.05f }, { 49.45f,  7.95f }, { 49.55f,  7.85f }, { 49.55f,  7.05f },
    { 49.50f,  6.55f }, { 49.50f,  6.35f }, { 49.55f,  5.85f }, { 49.65f,  5.65f },
    // Ardennes / Belgique
    { 49.80f,  5.45f }, { 49.85f,  5.10f }, { 49.80f,  4.85f }, { 50.05f,  4.85f },
    { 50.15f,  4.75f }, { 50.20f,  4.50f }, { 50.30f,  4.20f }, { 50.45f,  3.85f },
    // Hainaut -> retour Dunkerque
    { 50.50f,  3.55f }, { 50.65f,  3.30f }, { 50.75f,  3.10f }, { 50.85f,  2.85f },
    { 51.00f,  2.60f },
};
static const int FRANCE_CONTOUR_COUNT = sizeof(FRANCE_CONTOUR) / sizeof(FRANCE_CONTOUR[0]);

// Bounding box
#define FRANCE_LAT_MIN  42.00f
#define FRANCE_LAT_MAX  51.30f
#define FRANCE_LON_MIN  -5.00f
#define FRANCE_LON_MAX   8.40f

// ── Widget state ──────────────────────────────────────────
static lv_obj_t* map_container = nullptr;
static lv_obj_t* contour_line  = nullptr;
static lv_obj_t* dot_gps       = nullptr;
static lv_obj_t* popup_info    = nullptr;
static lv_obj_t* popup_text    = nullptr;

static lv_coord_t map_w = 0;
static lv_coord_t map_h = 0;
static float last_lat = 0, last_lon = 0;
static bool last_has_fix = false;

// Points lv_line (alloués en PSRAM pour éviter RAM interne)
static lv_point_t* contour_points = nullptr;

// ── Conversion lat/lon → pixel XY ─────────────────────────
static void latlon_to_xy(float lat, float lon, lv_coord_t* x, lv_coord_t* y) {
    float span_lon = FRANCE_LON_MAX - FRANCE_LON_MIN;
    float span_lat = FRANCE_LAT_MAX - FRANCE_LAT_MIN;

    int pad = 8;
    int usable_w = map_w - 2 * pad;
    int usable_h = map_h - 2 * pad;

    // Ratio adapté à la latitude France (~46.5°)
    // Projection Mercator simplifiee : 1 deg de longitude vaut moins en pixels que 1 deg de latitude
    // Au niveau de la France (~46.5N), 1 deg lon = cos(46.5) * 1 deg lat = 0.69 * 1 deg lat
    float lon_per_lat_ratio = cosf(46.5f * M_PI / 180.0f);  // ~0.69
    float scale_x = usable_w / (span_lon * lon_per_lat_ratio);
    float scale_y = usable_h / span_lat;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    int fit_w = (int)(span_lon * lon_per_lat_ratio * scale);
    int fit_h = (int)(span_lat * scale);
    int offset_x = pad + (usable_w - fit_w) / 2;
    int offset_y = pad + (usable_h - fit_h) / 2;

    *x = offset_x + (lv_coord_t)((lon - FRANCE_LON_MIN) * lon_per_lat_ratio * scale);
    *y = offset_y + (lv_coord_t)((FRANCE_LAT_MAX - lat) * scale);
}

// ── Callback clic popup ──────────────────────────────────
static void _map_click_cb(lv_event_t* e) {
    france_map_show_popup(popup_info == nullptr || lv_obj_has_flag(popup_info, LV_OBJ_FLAG_HIDDEN));
}

// ============================================================
//  API
// ============================================================
lv_obj_t* france_map_create(lv_obj_t* parent, lv_coord_t w, lv_coord_t h) {
    map_w = w;
    map_h = h;

    // Conteneur
    map_container = lv_obj_create(parent);
    lv_obj_set_size(map_container, w, h);
    lv_obj_set_style_bg_color(map_container, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(map_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(map_container, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(map_container, 1, 0);
    lv_obj_set_style_radius(map_container, 8, 0);
    lv_obj_set_style_pad_all(map_container, 0, 0);
    lv_obj_clear_flag(map_container, LV_OBJ_FLAG_SCROLLABLE);
    // Pas de click_cb interne pour eviter conflits avec parent (gps_card)
    lv_obj_clear_flag(map_container, LV_OBJ_FLAG_CLICKABLE);
    // lv_obj_add_event_cb(map_container, _map_click_cb, LV_EVENT_CLICKED, nullptr);

    // Allocation des points (fermés : n+1 pour boucler)
    size_t bytes = sizeof(lv_point_t) * (FRANCE_CONTOUR_COUNT + 1);
    contour_points = (lv_point_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!contour_points) {
        // Fallback RAM interne si PSRAM indispo
        contour_points = (lv_point_t*)malloc(bytes);
    }
    if (!contour_points) {
        Serial0.println("[Map] malloc points fail");
        return map_container;
    }

    // Remplir les points convertis en pixels
    for (int i = 0; i < FRANCE_CONTOUR_COUNT; i++) {
        lv_coord_t x, y;
        latlon_to_xy(FRANCE_CONTOUR[i][0], FRANCE_CONTOUR[i][1], &x, &y);
        contour_points[i].x = x;
        contour_points[i].y = y;
    }
    // Fermer la boucle
    contour_points[FRANCE_CONTOUR_COUNT] = contour_points[0];

    // Créer objet line
    contour_line = lv_line_create(map_container);
    lv_line_set_points(contour_line, contour_points, FRANCE_CONTOUR_COUNT + 1);
    lv_obj_set_style_line_color(contour_line, COLOR_ACCENT, 0);
    lv_obj_set_style_line_width(contour_line, 2, 0);
    lv_obj_set_style_line_rounded(contour_line, true, 0);
    lv_obj_clear_flag(contour_line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(contour_line, 0, 0);

    // Point GPS (rouge avec bordure blanche)
    dot_gps = lv_obj_create(map_container);
    lv_obj_set_size(dot_gps, 12, 12);
    lv_obj_set_style_radius(dot_gps, 6, 0);
    lv_obj_set_style_bg_color(dot_gps, COLOR_RED, 0);
    lv_obj_set_style_bg_opa(dot_gps, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dot_gps, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(dot_gps, 2, 0);
    lv_obj_add_flag(dot_gps, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(dot_gps, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dot_gps, LV_OBJ_FLAG_SCROLLABLE);

    Serial0.printf("[Map] Carte France creee %dx%d (%d points)\n",
        w, h, FRANCE_CONTOUR_COUNT);
    return map_container;
}

void france_map_update_position(float lat, float lon, bool has_fix) {
    last_lat = lat;
    last_lon = lon;
    last_has_fix = has_fix;

    if (!dot_gps) return;

    if (!has_fix) {
        lv_obj_add_flag(dot_gps, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_coord_t x, y;
    latlon_to_xy(lat, lon, &x, &y);
    lv_obj_set_pos(dot_gps, x - 6, y - 6);
    lv_obj_clear_flag(dot_gps, LV_OBJ_FLAG_HIDDEN);
}

void france_map_show_popup(bool show) {
    if (!map_container) return;

    if (show) {
        if (!popup_info) {
            popup_info = lv_obj_create(map_container);
            lv_obj_set_size(popup_info, map_w - 20, 60);
            lv_obj_align(popup_info, LV_ALIGN_BOTTOM_MID, 0, -6);
            lv_obj_set_style_bg_color(popup_info, lv_color_hex(0x161B22), 0);
            lv_obj_set_style_bg_opa(popup_info, LV_OPA_90, 0);
            lv_obj_set_style_border_color(popup_info, COLOR_ACCENT, 0);
            lv_obj_set_style_border_width(popup_info, 1, 0);
            lv_obj_set_style_radius(popup_info, 6, 0);
            lv_obj_set_style_pad_all(popup_info, 4, 0);
            lv_obj_clear_flag(popup_info, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(popup_info, LV_OBJ_FLAG_CLICKABLE);

            popup_text = lv_label_create(popup_info);
            lv_obj_set_style_text_color(popup_text, COLOR_WHITE, 0);
            lv_obj_align(popup_text, LV_ALIGN_LEFT_MID, 0, 0);
        }

        char buf[128];
        if (last_has_fix) {
            snprintf(buf, sizeof(buf),
                "Lat: %.5f°\nLon: %.5f°",
                last_lat, last_lon);
        } else {
            snprintf(buf, sizeof(buf), "Pas de signal GPS");
        }
        lv_label_set_text(popup_text, buf);
        lv_obj_clear_flag(popup_info, LV_OBJ_FLAG_HIDDEN);
    } else if (popup_info) {
        lv_obj_add_flag(popup_info, LV_OBJ_FLAG_HIDDEN);
    }
}
