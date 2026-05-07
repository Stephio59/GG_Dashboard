/*
 * page_battery.cpp — Énergie : batterie JKBMS + solaire Victron MPPT 100/30
 */
#include "ui.h"
#include "lv_fonts_fr.h"
#include "icons_van.h"
#include "icons_sd.h"
#include "display_config.h"
#include "jkbms_ble.h"
#include "victron_ble.h"
#include <math.h>

extern UiManager ui;

// Widgets batterie (gauche)
static lv_obj_t* arc_soc      = nullptr;
static lv_obj_t* lbl_soc      = nullptr;
static lv_obj_t* lbl_bat_volt = nullptr;
static lv_obj_t* lbl_bat_amp  = nullptr;
static lv_obj_t* lbl_bat_temp = nullptr;
static lv_obj_t* lbl_bat_cap  = nullptr;
static lv_obj_t* lbl_statut   = nullptr;
static lv_obj_t* lbl_ble_dot  = nullptr;

// Widgets solaire (droite haut)
static lv_obj_t* lbl_solar_w   = nullptr;
static lv_obj_t* lbl_solar_v   = nullptr;
static lv_obj_t* lbl_solar_a   = nullptr;
static lv_obj_t* lbl_solar_day = nullptr;
static lv_obj_t* lbl_solar_dot = nullptr;

// Widgets conso (droite bas)
static lv_obj_t* lbl_conso   = nullptr;

void page_battery_build(lv_obj_t* parent) {
    ui_draw_bg(parent);

    // ═══ Titre ═══
    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Energie");
    lv_obj_set_style_text_font(title, &FONT_36, 0);
    lv_obj_set_style_text_color(title, COLOR_GREEN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    // Bouton Historique (label cliquable simple en haut a droite)
    lv_obj_t* lbl_hist = lv_label_create(parent);
    lv_label_set_text(lbl_hist, LV_SYMBOL_LIST "  Historique");
    lv_obj_align(lbl_hist, LV_ALIGN_TOP_RIGHT, -16, 16);
    lv_obj_set_style_text_font(lbl_hist, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_hist, COLOR_BLUE, 0);
    lv_obj_add_flag(lbl_hist, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lbl_hist, [](lv_event_t* e){
        ui.showPage(PAGE_HISTORY);
    }, LV_EVENT_CLICKED, nullptr);

    // ════════════════════════════════════════════════════════
    //  CARTE BATTERIE (gauche, x=10, y=60, w=500, h=400)
    // ════════════════════════════════════════════════════════
    lv_obj_t* card_bat = lv_obj_create(parent);
    lv_obj_set_size(card_bat, 500, 400);
    lv_obj_set_pos(card_bat, 10, 60);
    lv_obj_add_style(card_bat, &ui.style_card, 0);
    lv_obj_clear_flag(card_bat, LV_OBJ_FLAG_SCROLLABLE);

    // Titre carte
    lv_obj_t* t_bat = lv_label_create(card_bat);
    lv_label_set_text(t_bat, "Batterie JK-BMS 300 Ah");
    lv_obj_set_style_text_font(t_bat, &FONT_22, 0);
    lv_obj_set_style_text_color(t_bat, COLOR_ACCENT, 0);
    lv_obj_align(t_bat, LV_ALIGN_TOP_LEFT, 10, 4);

    // Pastille BLE (petit point coloré en haut-droite)
    lbl_ble_dot = lv_label_create(card_bat);
    lv_label_set_text(lbl_ble_dot, ICON_BLUETOOTH);
    lv_obj_set_style_text_font(lbl_ble_dot, &ICON_FONT_22, 0);
    lv_obj_set_style_text_color(lbl_ble_dot, COLOR_GRAY, 0);
    lv_obj_align(lbl_ble_dot, LV_ALIGN_TOP_RIGHT, -10, 4);

    // Arc SOC (210x210 centré-gauche)
    arc_soc = lv_arc_create(card_bat);
    lv_obj_set_size(arc_soc, 210, 210);
    lv_arc_set_range(arc_soc, 0, 100);
    lv_arc_set_value(arc_soc, 0);
    lv_arc_set_rotation(arc_soc, 135);
    lv_arc_set_bg_angles(arc_soc, 0, 270);
    lv_obj_remove_style(arc_soc, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(arc_soc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_soc, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_soc, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_soc, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_soc, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_pos(arc_soc, 10, 55);

    lbl_soc = lv_label_create(card_bat);
    lv_label_set_text(lbl_soc, "--%");
    lv_obj_set_style_text_font(lbl_soc, &FONT_48, 0);
    lv_obj_set_style_text_color(lbl_soc, COLOR_WHITE, 0);
    lv_obj_align_to(lbl_soc, arc_soc, LV_ALIGN_CENTER, 0, 0);

    // Colonne droite : V, A, capacité, temp (x=250..480)
    lbl_bat_volt = lv_label_create(card_bat);
    lv_label_set_text(lbl_bat_volt, "-- V");
    lv_obj_set_style_text_font(lbl_bat_volt, &FONT_36, 0);
    lv_obj_set_style_text_color(lbl_bat_volt, COLOR_WHITE, 0);
    lv_obj_set_pos(lbl_bat_volt, 260, 60);

    lbl_bat_amp = lv_label_create(card_bat);
    lv_label_set_text(lbl_bat_amp, "-- A");
    lv_obj_set_style_text_font(lbl_bat_amp, &FONT_28, 0);
    lv_obj_set_style_text_color(lbl_bat_amp, COLOR_BLUE, 0);
    lv_obj_set_pos(lbl_bat_amp, 260, 115);

    lbl_bat_cap = lv_label_create(card_bat);
    lv_label_set_text(lbl_bat_cap, "-- / -- Ah");
    lv_obj_set_style_text_font(lbl_bat_cap, &FONT_22, 0);
    lv_obj_set_style_text_color(lbl_bat_cap, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_bat_cap, 260, 170);

    lbl_bat_temp = lv_label_create(card_bat);
    lv_label_set_text(lbl_bat_temp, "-- °C");
    lv_obj_set_style_text_font(lbl_bat_temp, &FONT_20, 0);
    lv_obj_set_style_text_color(lbl_bat_temp, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_bat_temp, 260, 210);

    // Statut en bas de la carte
    lbl_statut = lv_label_create(card_bat);
    lv_label_set_text(lbl_statut, "En attente...");
    lv_obj_set_style_text_font(lbl_statut, &FONT_18, 0);
    lv_obj_set_style_text_color(lbl_statut, COLOR_GRAY, 0);
    lv_obj_align(lbl_statut, LV_ALIGN_BOTTOM_MID, 0, -10);

    // ════════════════════════════════════════════════════════
    //  CARTE SOLAIRE (droite haut, x=520, y=60, w=494, h=195)
    // ════════════════════════════════════════════════════════
    lv_obj_t* card_sol = lv_obj_create(parent);
    lv_obj_set_size(card_sol, 494, 195);
    lv_obj_set_pos(card_sol, 520, 60);
    lv_obj_add_style(card_sol, &ui.style_card, 0);
    lv_obj_clear_flag(card_sol, LV_OBJ_FLAG_SCROLLABLE);

    // Icône soleil + titre - BMP solar depuis SD (fallback texte simple)
    const lv_img_dsc_t* solar_ic = icons_sd_get("solar");
    if (solar_ic) {
        lv_obj_t* img = lv_img_create(card_sol);
        lv_img_set_src(img, solar_ic);
        lv_img_set_zoom(img, 96);  // ~24px
        lv_obj_set_pos(img, 6, 4);
    } else {
        // Fallback texte
        lv_obj_t* sol_ic = lv_label_create(card_sol);
        lv_label_set_text(sol_ic, "[*]");
        lv_obj_set_style_text_font(sol_ic, &FONT_18, 0);
        lv_obj_set_style_text_color(sol_ic, COLOR_YELLOW, 0);
        lv_obj_align(sol_ic, LV_ALIGN_TOP_LEFT, 10, 4);
    }

    lv_obj_t* t_sol = lv_label_create(card_sol);
    lv_label_set_text(t_sol, "Solaire MPPT 100/30");
    lv_obj_set_style_text_font(t_sol, &FONT_20, 0);
    lv_obj_set_style_text_color(t_sol, COLOR_YELLOW, 0);
    lv_obj_align(t_sol, LV_ALIGN_TOP_LEFT, 40, 4);

    lbl_solar_dot = lv_label_create(card_sol);
    lv_label_set_text(lbl_solar_dot, ICON_BLUETOOTH);
    lv_obj_set_style_text_font(lbl_solar_dot, &ICON_FONT_22, 0);
    lv_obj_set_style_text_color(lbl_solar_dot, COLOR_GRAY, 0);
    lv_obj_align(lbl_solar_dot, LV_ALIGN_TOP_RIGHT, -8, 4);

    // Puissance W (gros, centre-gauche)
    lbl_solar_w = lv_label_create(card_sol);
    lv_label_set_text(lbl_solar_w, "-- W");
    lv_obj_set_style_text_font(lbl_solar_w, &FONT_48, 0);
    lv_obj_set_style_text_color(lbl_solar_w, COLOR_YELLOW, 0);
    lv_obj_set_pos(lbl_solar_w, 15, 45);

    // V et A (droite)
    lbl_solar_v = lv_label_create(card_sol);
    lv_label_set_text(lbl_solar_v, "-- V");
    lv_obj_set_style_text_font(lbl_solar_v, &FONT_20, 0);
    lv_obj_set_style_text_color(lbl_solar_v, COLOR_WHITE, 0);
    lv_obj_set_pos(lbl_solar_v, 300, 50);

    lbl_solar_a = lv_label_create(card_sol);
    lv_label_set_text(lbl_solar_a, "-- A");
    lv_obj_set_style_text_font(lbl_solar_a, &FONT_20, 0);
    lv_obj_set_style_text_color(lbl_solar_a, COLOR_WHITE, 0);
    lv_obj_set_pos(lbl_solar_a, 300, 85);

    // Production jour (bas)
    lbl_solar_day = lv_label_create(card_sol);
    lv_label_set_text(lbl_solar_day, "Aujourd'hui : -- Wh");
    lv_obj_set_style_text_font(lbl_solar_day, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_solar_day, COLOR_GRAY, 0);
    lv_obj_align(lbl_solar_day, LV_ALIGN_BOTTOM_MID, 0, -12);

    // ════════════════════════════════════════════════════════
    //  CARTE CONSO/STATS (droite bas, x=520, y=265, w=494, h=195)
    // ════════════════════════════════════════════════════════
    lv_obj_t* card_conso = lv_obj_create(parent);
    lv_obj_set_size(card_conso, 494, 195);
    lv_obj_set_pos(card_conso, 520, 265);
    lv_obj_add_style(card_conso, &ui.style_card, 0);
    lv_obj_clear_flag(card_conso, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t_conso = lv_label_create(card_conso);
    lv_label_set_text(t_conso, "Statistiques");
    lv_obj_set_style_text_font(t_conso, &FONT_20, 0);
    lv_obj_set_style_text_color(t_conso, COLOR_ORANGE, 0);
    lv_obj_align(t_conso, LV_ALIGN_TOP_LEFT, 10, 4);

    lbl_conso = lv_label_create(card_conso);
    lv_label_set_text(lbl_conso,
        "Puissance      : -- W\n"
        "Autonomie est. : -- h\n"
        "Cycles batterie: --\n"
        "Ecart cellules : -- mV");
    lv_obj_set_style_text_font(lbl_conso, &FONT_18, 0);
    lv_obj_set_style_text_color(lbl_conso, COLOR_WHITE, 0);
    lv_obj_align(lbl_conso, LV_ALIGN_CENTER, 0, 15);
}

void page_battery_update() {
    if (!lbl_soc) return;
    char buf[128];

    // ── Pastille BLE JKBMS ───────────────────────────────
    if (jkbms_is_connected()) {
        lv_obj_set_style_text_color(lbl_ble_dot, COLOR_GREEN, 0);
    } else {
        lv_obj_set_style_text_color(lbl_ble_dot, COLOR_GRAY, 0);
    }

    // ── Données JKBMS ────────────────────────────────────
    if (jkbmsData.valid) {
        int soc_int = (int)roundf(jkbmsData.soc_pct);
        lv_arc_set_value(arc_soc, soc_int);
        snprintf(buf, sizeof(buf), "%d%%", soc_int);
        lv_label_set_text(lbl_soc, buf);

        lv_color_t soc_color = COLOR_GREEN;
        if      (soc_int < 20) soc_color = COLOR_RED;
        else if (soc_int < 40) soc_color = COLOR_ORANGE;
        else if (soc_int < 60) soc_color = COLOR_YELLOW;
        lv_obj_set_style_arc_color(arc_soc, soc_color, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(lbl_soc, soc_color, 0);

        snprintf(buf, sizeof(buf), "%.2f V", jkbmsData.voltage);
        lv_label_set_text(lbl_bat_volt, buf);

        snprintf(buf, sizeof(buf), "%+.2f A", jkbmsData.current);
        lv_label_set_text(lbl_bat_amp, buf);
        lv_obj_set_style_text_color(lbl_bat_amp,
            jkbmsData.current >= 0 ? COLOR_GREEN : COLOR_ORANGE, 0);

        snprintf(buf, sizeof(buf), "%.0f / %.0f Ah",
            jkbmsData.capacity_remain_ah, jkbmsData.capacity_total_ah);
        lv_label_set_text(lbl_bat_cap, buf);

        snprintf(buf, sizeof(buf), "MOS %.0f°C  T %.0f°C",
            jkbmsData.temp_mos_c, jkbmsData.temp_t1_c);
        lv_label_set_text(lbl_bat_temp, buf);

        float tmax = jkbmsData.temp_mos_c;
        if (jkbmsData.temp_t1_c > tmax) tmax = jkbmsData.temp_t1_c;
        lv_color_t tc = COLOR_GRAY;
        if      (tmax > 55) tc = COLOR_RED;
        else if (tmax > 45) tc = COLOR_ORANGE;
        else if (tmax < 2 ) tc = COLOR_BLUE;
        lv_obj_set_style_text_color(lbl_bat_temp, tc, 0);

        if (jkbmsData.current > 0.5f) {
            lv_label_set_text(lbl_statut, LV_SYMBOL_CHARGE " Charge en cours");
            lv_obj_set_style_text_color(lbl_statut, COLOR_GREEN, 0);
        } else if (jkbmsData.current < -0.5f) {
            lv_label_set_text(lbl_statut, LV_SYMBOL_DOWN " Decharge en cours");
            lv_obj_set_style_text_color(lbl_statut, COLOR_ORANGE, 0);
        } else {
            lv_label_set_text(lbl_statut, LV_SYMBOL_OK " Au repos");
            lv_obj_set_style_text_color(lbl_statut, COLOR_GRAY, 0);
        }

        // Stats
        float power_abs = fabsf(jkbmsData.power);
        float autonomie_h = 0;
        if (jkbmsData.current < -0.1f && jkbmsData.capacity_remain_ah > 0.1f) {
            autonomie_h = jkbmsData.capacity_remain_ah / fabsf(jkbmsData.current);
        }
        int cell_diff_mv = (int)roundf(jkbmsData.cell_v_diff * 1000);
        snprintf(buf, sizeof(buf),
            "Puissance      : %.0f W\n"
            "Autonomie est. : %.1f h\n"
            "Cycles batterie: %lu\n"
            "Ecart cellules : %d mV",
            power_abs, autonomie_h,
            (unsigned long)jkbmsData.cycle_count, cell_diff_mv);
        lv_label_set_text(lbl_conso, buf);
    } else {
        lv_arc_set_value(arc_soc, 0);
        lv_label_set_text(lbl_soc, "--");
        lv_label_set_text(lbl_bat_volt, "-- V");
        lv_label_set_text(lbl_bat_amp, "-- A");
        lv_label_set_text(lbl_bat_cap, "-- / -- Ah");
        lv_label_set_text(lbl_bat_temp, "-- °C");
        snprintf(buf, sizeof(buf), LV_SYMBOL_REFRESH "  %s", jkbms_state_str());
        lv_label_set_text(lbl_statut, buf);
        lv_obj_set_style_text_color(lbl_statut, COLOR_GRAY, 0);
        lv_label_set_text(lbl_conso,
            "Puissance      : -- W\n"
            "Autonomie est. : -- h\n"
            "Cycles batterie: --\n"
            "Ecart cellules : -- mV");
    }

    // ── Données Victron MPPT ─────────────────────────────
    if (victronMpptData.valid) {
        lv_obj_set_style_text_color(lbl_solar_dot, COLOR_GREEN, 0);
        snprintf(buf, sizeof(buf), "%.0f W", victronMpptData.power_pv_w);
        lv_label_set_text(lbl_solar_w, buf);
        snprintf(buf, sizeof(buf), "%.1f V", victronMpptData.voltage_bat);
        lv_label_set_text(lbl_solar_v, buf);
        snprintf(buf, sizeof(buf), "%.1f A", victronMpptData.current);
        lv_label_set_text(lbl_solar_a, buf);
        snprintf(buf, sizeof(buf), "Aujourd'hui : %.0f Wh", victronMpptData.yield_today_wh);
        lv_label_set_text(lbl_solar_day, buf);
    } else {
        lv_obj_set_style_text_color(lbl_solar_dot, COLOR_GRAY, 0);
        lv_label_set_text(lbl_solar_w, "-- W");
        lv_label_set_text(lbl_solar_v, "-- V");
        lv_label_set_text(lbl_solar_a, "-- A");
        lv_label_set_text(lbl_solar_day, "En attente...");
    }
}
