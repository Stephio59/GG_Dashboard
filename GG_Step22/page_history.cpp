/*
 * page_history.cpp — Page Graphiques 24h
 * 
 * Layout : grille 3 lignes x 2 colonnes de mini-graphiques
 *   Ligne 1 : SOC (large) + Tension batterie
 *   Ligne 2 : Production solaire + Conso
 *   Ligne 3 : Temperature interieure (large)
 */
#include "ui.h"
#include "lv_fonts_fr.h"
#include "icons_van.h"
#include "display_config.h"
#include "history.h"
#include <math.h>

static lv_obj_t* chart_soc   = nullptr;
static lv_obj_t* chart_volt  = nullptr;
static lv_obj_t* chart_pv    = nullptr;
static lv_obj_t* chart_load  = nullptr;
static lv_obj_t* chart_temp  = nullptr;

static lv_chart_series_t* ser_soc  = nullptr;
static lv_chart_series_t* ser_volt = nullptr;
static lv_chart_series_t* ser_pv   = nullptr;
static lv_chart_series_t* ser_load = nullptr;
static lv_chart_series_t* ser_temp = nullptr;

static lv_obj_t* lbl_soc_now  = nullptr;
static lv_obj_t* lbl_volt_now = nullptr;
static lv_obj_t* lbl_pv_now   = nullptr;
static lv_obj_t* lbl_load_now = nullptr;
static lv_obj_t* lbl_temp_now = nullptr;

// Helper : creer un mini-graph dans une carte
static lv_obj_t* make_chart_card(lv_obj_t* parent, int x, int y, int w, int h,
                                 const char* title, const char* icon, lv_color_t color,
                                 lv_obj_t** lbl_now_out, lv_chart_series_t** serie_out) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Titre + icone en haut
    lv_obj_t* lbl_title = lv_label_create(card);
    lv_label_set_text_fmt(lbl_title, "%s %s", icon, title);
    lv_obj_set_style_text_font(lbl_title, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_title, COLOR_GRAY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 8, 6);
    
    // Valeur instantanee (gros texte)
    lv_obj_t* lbl_now = lv_label_create(card);
    lv_label_set_text(lbl_now, "--");
    lv_obj_set_style_text_font(lbl_now, &FONT_28, 0);
    lv_obj_set_style_text_color(lbl_now, color, 0);
    lv_obj_align(lbl_now, LV_ALIGN_TOP_RIGHT, -8, 4);
    *lbl_now_out = lbl_now;
    
    // Chart en bas
    lv_obj_t* chart = lv_chart_create(card);
    lv_obj_set_size(chart, w - 16, h - 50);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, HIST_POINTS);
    lv_chart_set_div_line_count(chart, 2, 4);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);  // pas de points
    lv_obj_set_style_pad_all(chart, 4, 0);
    
    lv_chart_series_t* serie = lv_chart_add_series(chart, color, LV_CHART_AXIS_PRIMARY_Y);
    *serie_out = serie;
    
    return chart;
}

void page_history_build(lv_obj_t* parent) {
    ui_draw_bg(parent);

    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Historique 24h");
    lv_obj_set_style_text_font(title, &FONT_36, 0);
    lv_obj_set_style_text_color(title, COLOR_BLUE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    // Layout 3x2 :
    //   y=58 : SOC (large) + V_bat
    //   y=232: PV + Conso
    //   y=406: Temp (large)
    int margin = 8;
    int top_y = 58;
    int row_h = 165;
    int half_w = (SCREEN_WIDTH - 3*margin) / 2;
    int full_w = SCREEN_WIDTH - 2*margin;
    
    chart_soc = make_chart_card(parent, margin, top_y,
        half_w, row_h,
        "SOC Batterie", LV_SYMBOL_BATTERY_3, COLOR_GREEN,
        &lbl_soc_now, &ser_soc);
    chart_volt = make_chart_card(parent, margin*2 + half_w, top_y,
        half_w, row_h,
        "Tension", LV_SYMBOL_CHARGE, COLOR_YELLOW,
        &lbl_volt_now, &ser_volt);
    
    chart_pv = make_chart_card(parent, margin, top_y + row_h + margin,
        half_w, row_h,
        "Solaire", LV_SYMBOL_UPLOAD, COLOR_ORANGE,
        &lbl_pv_now, &ser_pv);
    chart_load = make_chart_card(parent, margin*2 + half_w, top_y + row_h + margin,
        half_w, row_h,
        "Conso", LV_SYMBOL_DOWNLOAD, COLOR_RED,
        &lbl_load_now, &ser_load);
    
    chart_temp = make_chart_card(parent, margin, top_y + 2*(row_h + margin),
        full_w, row_h - 30,
        "Temperature interieure", LV_SYMBOL_HOME, COLOR_BLUE,
        &lbl_temp_now, &ser_temp);
    
    // Legende temps en bas
    lv_obj_t* lbl_axis = lv_label_create(parent);
    lv_label_set_text(lbl_axis, "← -24h                                                                                  maintenant →");
    lv_obj_set_style_text_font(lbl_axis, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_axis, COLOR_GRAY, 0);
    lv_obj_align(lbl_axis, LV_ALIGN_BOTTOM_MID, 0, -4);
}

// Helper : applique des données à un chart
static void apply_serie_to_chart(lv_obj_t* chart, lv_chart_series_t* serie,
                                  const HistorySerie& hist) {
    static int16_t buf[HIST_POINTS];
    int n = history_get_ordered(hist, buf, HIST_POINTS);
    
    // Calcul min/max pour autoscale
    int16_t v_min = 32000, v_max = -32000;
    for (int i = 0; i < n; i++) {
        if (buf[i] != HIST_NO_DATA) {
            if (buf[i] < v_min) v_min = buf[i];
            if (buf[i] > v_max) v_max = buf[i];
        }
    }
    if (v_min == 32000) { v_min = 0; v_max = 100; }
    if (v_min == v_max) { v_min -= 1; v_max += 1; }
    
    // Marge 10%
    int16_t margin = (v_max - v_min) / 10;
    if (margin < 1) margin = 1;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, v_min - margin, v_max + margin);
    
    // Fill points - les anciens sont LV_CHART_POINT_NONE
    for (int i = 0; i < HIST_POINTS; i++) {
        // Affichage "ancien à gauche, recent à droite"
        // Position dans le chart : i (0..143)
        int chart_pos = i;
        int data_pos = i - (HIST_POINTS - n);   // les premiers points sont vides
        
        if (data_pos < 0 || buf[data_pos] == HIST_NO_DATA) {
            lv_chart_set_value_by_id(chart, serie, chart_pos, LV_CHART_POINT_NONE);
        } else {
            lv_chart_set_value_by_id(chart, serie, chart_pos, buf[data_pos]);
        }
    }
    lv_chart_refresh(chart);
}

void page_history_update() {
    if (!chart_soc) return;
    
    apply_serie_to_chart(chart_soc,  ser_soc,  hist_soc);
    apply_serie_to_chart(chart_volt, ser_volt, hist_voltage);
    apply_serie_to_chart(chart_pv,   ser_pv,   hist_pv_power);
    apply_serie_to_chart(chart_load, ser_load, hist_load_power);
    apply_serie_to_chart(chart_temp, ser_temp, hist_temp);
    
    // Valeurs instantanees (dernier point)
    char buf[32];
    if (hist_soc.count > 0) {
        int16_t v = hist_soc.data[(hist_soc.head + HIST_POINTS - 1) % HIST_POINTS];
        if (v != HIST_NO_DATA) {
            snprintf(buf, sizeof(buf), "%.1f%%", v / 10.0f);
            lv_label_set_text(lbl_soc_now, buf);
        }
    }
    if (hist_voltage.count > 0) {
        int16_t v = hist_voltage.data[(hist_voltage.head + HIST_POINTS - 1) % HIST_POINTS];
        if (v != HIST_NO_DATA) {
            snprintf(buf, sizeof(buf), "%.2fV", v / 100.0f);
            lv_label_set_text(lbl_volt_now, buf);
        }
    }
    if (hist_pv_power.count > 0) {
        int16_t v = hist_pv_power.data[(hist_pv_power.head + HIST_POINTS - 1) % HIST_POINTS];
        if (v != HIST_NO_DATA) {
            snprintf(buf, sizeof(buf), "%dW", v);
            lv_label_set_text(lbl_pv_now, buf);
        }
    }
    if (hist_load_power.count > 0) {
        int16_t v = hist_load_power.data[(hist_load_power.head + HIST_POINTS - 1) % HIST_POINTS];
        if (v != HIST_NO_DATA) {
            snprintf(buf, sizeof(buf), "%dW", v);
            lv_label_set_text(lbl_load_now, buf);
        }
    }
    if (hist_temp.count > 0) {
        int16_t v = hist_temp.data[(hist_temp.head + HIST_POINTS - 1) % HIST_POINTS];
        if (v != HIST_NO_DATA) {
            snprintf(buf, sizeof(buf), "%.1f°C", v / 10.0f);
            lv_label_set_text(lbl_temp_now, buf);
        }
    }
}
