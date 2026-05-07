/*
 * page_heating.cpp — Chauffage Nordkapp diesel BLE
 */
#include "ui.h"
#include "lv_fonts_fr.h"
#include "icons_van.h"
#include "icons_sd.h"
#include "display_config.h"
#include "heating_ble.h"
#include "bme_wire1.h"

static lv_obj_t* btn_power      = nullptr;
static lv_obj_t* lbl_btn_power  = nullptr;
static lv_obj_t* lbl_current    = nullptr;
static lv_obj_t* lbl_target     = nullptr;
static lv_obj_t* arc_target     = nullptr;
static lv_obj_t* lbl_status     = nullptr;
static lv_obj_t* lbl_fan        = nullptr;
static lv_obj_t* lbl_pump       = nullptr;
static lv_obj_t* lbl_voltage    = nullptr;
static lv_obj_t* lbl_case_temp  = nullptr;
static lv_obj_t* lbl_ble_dot    = nullptr;

static uint8_t local_target_temp = 19;  // valeur initiale consigne

static void _power_cb(lv_event_t* e) {
    Serial0.println("[UI] Appui bouton ON/OFF chauffage");
    heating_set_power(true);  // toggle
}

static void _arc_cb(lv_event_t* e) {
    local_target_temp = lv_arc_get_value(arc_target);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d °C", local_target_temp);
    lv_label_set_text(lbl_target, buf);
    
    // Envoyer la commande après un petit délai (évite spam)
    static uint32_t last_send = 0;
    if (millis() - last_send > 1000) {
        heating_set_temp(local_target_temp);
        last_send = millis();
    }
}

void page_heating_build(lv_obj_t* parent) {
    ui_draw_bg(parent);

    // ═══ Titre ═══
    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Chauffage");
    lv_obj_set_style_text_font(title, &FONT_36, 0);
    lv_obj_set_style_text_color(title, COLOR_ORANGE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    // ════════════════════════════════════════════════════════
    //  CARTE TEMPERATURE + CONSIGNE (gauche, x=10, y=60, 500x400)
    // ════════════════════════════════════════════════════════
    lv_obj_t* card_temp = lv_obj_create(parent);
    lv_obj_set_size(card_temp, 500, 400);
    lv_obj_set_pos(card_temp, 10, 60);
    lv_obj_add_style(card_temp, &ui.style_card, 0);
    lv_obj_clear_flag(card_temp, LV_OBJ_FLAG_SCROLLABLE);

    // Titre + pastille BLE - icone heater BMP si dispo
    const lv_img_dsc_t* heater_ic = icons_sd_get("heater");
    if (heater_ic) {
        lv_obj_t* img = lv_img_create(card_temp);
        lv_img_set_src(img, heater_ic);
        lv_img_set_zoom(img, 96);  // 64*96/256 = 24px
        lv_obj_set_pos(img, 10, 4);
    } else {
        // Fallback texte simple
        lv_obj_t* t_ic = lv_label_create(card_temp);
        lv_label_set_text(t_ic, "[!]");
        lv_obj_set_style_text_font(t_ic, &FONT_18, 0);
        lv_obj_set_style_text_color(t_ic, COLOR_ORANGE, 0);
        lv_obj_align(t_ic, LV_ALIGN_TOP_LEFT, 10, 4);
    }

    lv_obj_t* t_c = lv_label_create(card_temp);
    lv_label_set_text(t_c, "Ambiance / Consigne");
    lv_obj_set_style_text_font(t_c, &FONT_20, 0);
    lv_obj_set_style_text_color(t_c, COLOR_ACCENT, 0);
    lv_obj_align(t_c, LV_ALIGN_TOP_LEFT, 40, 4);

    lbl_ble_dot = lv_label_create(card_temp);
    lv_label_set_text(lbl_ble_dot, ICON_BLUETOOTH);
    lv_obj_set_style_text_font(lbl_ble_dot, &ICON_FONT_22, 0);
    lv_obj_set_style_text_color(lbl_ble_dot, COLOR_GRAY, 0);
    lv_obj_align(lbl_ble_dot, LV_ALIGN_TOP_RIGHT, -10, 4);

    // Temp ambiante (haut)
    lv_obj_t* t_amb = lv_label_create(card_temp);
    lv_label_set_text(t_amb, "Ambiante");
    lv_obj_set_style_text_font(t_amb, &FONT_16, 0);
    lv_obj_set_style_text_color(t_amb, COLOR_GRAY, 0);
    lv_obj_align(t_amb, LV_ALIGN_TOP_MID, 0, 40);

    lbl_current = lv_label_create(card_temp);
    lv_label_set_text(lbl_current, "-- °C");
    lv_obj_set_style_text_font(lbl_current, &FONT_48, 0);
    lv_obj_set_style_text_color(lbl_current, COLOR_WHITE, 0);
    lv_obj_align(lbl_current, LV_ALIGN_TOP_MID, 0, 60);

    // Arc consigne (bas, grand)
    arc_target = lv_arc_create(card_temp);
    lv_obj_set_size(arc_target, 220, 220);
    lv_arc_set_range(arc_target, 8, 36);
    lv_arc_set_value(arc_target, local_target_temp);
    lv_arc_set_rotation(arc_target, 135);
    lv_arc_set_bg_angles(arc_target, 0, 270);
    lv_obj_set_style_arc_width(arc_target, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_target, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_target, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_target, COLOR_ORANGE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc_target, COLOR_ORANGE, LV_PART_KNOB);
    lv_obj_align(arc_target, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_add_event_cb(arc_target, _arc_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    lbl_target = lv_label_create(card_temp);
    char buf[16]; snprintf(buf, sizeof(buf), "%d °C", local_target_temp);
    lv_label_set_text(lbl_target, buf);
    lv_obj_set_style_text_font(lbl_target, &FONT_48, 0);
    lv_obj_set_style_text_color(lbl_target, COLOR_ORANGE, 0);
    lv_obj_align_to(lbl_target, arc_target, LV_ALIGN_CENTER, 0, -8);

    lv_obj_t* lbl_consigne = lv_label_create(card_temp);
    lv_label_set_text(lbl_consigne, "Consigne");
    lv_obj_set_style_text_font(lbl_consigne, &FONT_14, 0);
    lv_obj_set_style_text_color(lbl_consigne, COLOR_GRAY, 0);
    lv_obj_align_to(lbl_consigne, arc_target, LV_ALIGN_CENTER, 0, 28);

    // ════════════════════════════════════════════════════════
    //  CARTE BOUTON ON/OFF (droite haut, x=520, y=60, 494x195)
    // ════════════════════════════════════════════════════════
    lv_obj_t* card_btn = lv_obj_create(parent);
    lv_obj_set_size(card_btn, 494, 195);
    lv_obj_set_pos(card_btn, 520, 60);
    lv_obj_add_style(card_btn, &ui.style_card, 0);
    lv_obj_clear_flag(card_btn, LV_OBJ_FLAG_SCROLLABLE);

    btn_power = lv_btn_create(card_btn);
    lv_obj_set_size(btn_power, 430, 140);
    lv_obj_align(btn_power, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(btn_power, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_bg_opa(btn_power, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn_power, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(btn_power, 2, 0);
    lv_obj_set_style_radius(btn_power, 14, 0);
    lv_obj_add_event_cb(btn_power, _power_cb, LV_EVENT_CLICKED, nullptr);

    lbl_btn_power = lv_label_create(btn_power);
    lv_label_set_text(lbl_btn_power, LV_SYMBOL_POWER "  OFF");
    lv_obj_set_style_text_font(lbl_btn_power, &FONT_48, 0);
    lv_obj_set_style_text_color(lbl_btn_power, COLOR_WHITE, 0);
    lv_obj_center(lbl_btn_power);

    // ════════════════════════════════════════════════════════
    //  CARTE INFOS (droite bas, x=520, y=265, 494x195)
    // ════════════════════════════════════════════════════════
    lv_obj_t* card_info = lv_obj_create(parent);
    lv_obj_set_size(card_info, 494, 195);
    lv_obj_set_pos(card_info, 520, 265);
    lv_obj_add_style(card_info, &ui.style_card, 0);
    lv_obj_clear_flag(card_info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t_i = lv_label_create(card_info);
    lv_label_set_text(t_i, "Etat chauffage");
    lv_obj_set_style_text_font(t_i, &FONT_20, 0);
    lv_obj_set_style_text_color(t_i, COLOR_ACCENT, 0);
    lv_obj_align(t_i, LV_ALIGN_TOP_LEFT, 10, 4);

    lbl_status = lv_label_create(card_info);
    lv_label_set_text(lbl_status, "--");
    lv_obj_set_style_text_font(lbl_status, &FONT_20, 0);
    lv_obj_set_style_text_color(lbl_status, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_status, 12, 35);

    lbl_case_temp = lv_label_create(card_info);
    lv_label_set_text(lbl_case_temp, "Chambre : -- °C");
    lv_obj_set_style_text_font(lbl_case_temp, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_case_temp, COLOR_WHITE, 0);
    lv_obj_set_pos(lbl_case_temp, 12, 70);

    lbl_fan = lv_label_create(card_info);
    lv_label_set_text(lbl_fan, "Ventilateur : -- rpm");
    lv_obj_set_style_text_font(lbl_fan, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_fan, COLOR_WHITE, 0);
    lv_obj_set_pos(lbl_fan, 12, 100);

    lbl_pump = lv_label_create(card_info);
    lv_label_set_text(lbl_pump, "Pompe diesel : -- Hz");
    lv_obj_set_style_text_font(lbl_pump, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_pump, COLOR_WHITE, 0);
    lv_obj_set_pos(lbl_pump, 12, 130);

    lbl_voltage = lv_label_create(card_info);
    lv_label_set_text(lbl_voltage, "Alim : -- V");
    lv_obj_set_style_text_font(lbl_voltage, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_voltage, COLOR_GRAY, 0);
    lv_obj_set_pos(lbl_voltage, 12, 160);
}

void page_heating_update() {
    if (!lbl_current) return;
    char buf[64];

    // Pastille BLE
    if (heating_is_connected()) {
        lv_obj_set_style_text_color(lbl_ble_dot, COLOR_GREEN, 0);
    } else {
        lv_obj_set_style_text_color(lbl_ble_dot, COLOR_GRAY, 0);
    }

    // Temp ambiante : priorité au BME280, sinon au capteur du chauffage
    if (bmeAvailable && bmeData.valid) {
        snprintf(buf, sizeof(buf), "%.1f °C", bmeData.temperature);
        lv_label_set_text(lbl_current, buf);
        float diff = bmeData.temperature - local_target_temp;
        lv_color_t c = COLOR_WHITE;
        if      (diff < -2) c = COLOR_BLUE;
        else if (diff > 2)  c = COLOR_ORANGE;
        lv_obj_set_style_text_color(lbl_current, c, 0);
    } else if (heatingData.valid) {
        snprintf(buf, sizeof(buf), "%.0f °C", heatingData.room_temp_c);
        lv_label_set_text(lbl_current, buf);
    }

    // Bouton ON/OFF
    if (heatingData.valid) {
        if (heatingData.running) {
            lv_obj_set_style_bg_color(btn_power, COLOR_ORANGE, 0);
            lv_label_set_text(lbl_btn_power, LV_SYMBOL_POWER "  ON");
            lv_obj_set_style_text_color(lbl_btn_power, lv_color_black(), 0);
        } else {
            lv_obj_set_style_bg_color(btn_power, lv_color_hex(0x21262D), 0);
            lv_label_set_text(lbl_btn_power, LV_SYMBOL_POWER "  OFF");
            lv_obj_set_style_text_color(lbl_btn_power, COLOR_WHITE, 0);
        }

        // Statut
        const char* state_txt = "?";
        lv_color_t state_col = COLOR_GRAY;
        switch (heatingData.running_state) {
            case 0: state_txt = LV_SYMBOL_CLOSE " Arrete";      state_col = COLOR_GRAY;   break;
            case 1: state_txt = LV_SYMBOL_REFRESH " Demarrage"; state_col = COLOR_YELLOW; break;
            case 2: state_txt = LV_SYMBOL_CHARGE " Prechauffe"; state_col = COLOR_ORANGE; break;
            case 3: state_txt = LV_SYMBOL_CHARGE " Chauffe";    state_col = COLOR_ORANGE; break;
            case 4: state_txt = LV_SYMBOL_REFRESH " Refroidit"; state_col = COLOR_BLUE;   break;
            default:
                snprintf(buf, sizeof(buf), "Etat %d", heatingData.running_state);
                state_txt = buf;
                break;
        }
        lv_label_set_text(lbl_status, state_txt);
        lv_obj_set_style_text_color(lbl_status, state_col, 0);

        snprintf(buf, sizeof(buf), "Chambre : %.0f °C", heatingData.case_temp_c);
        lv_label_set_text(lbl_case_temp, buf);

        snprintf(buf, sizeof(buf), "Ventilateur : %d rpm", heatingData.fan_rpm);
        lv_label_set_text(lbl_fan, buf);

        snprintf(buf, sizeof(buf), "Pompe diesel : %.1f Hz", heatingData.fuel_pump_hz * 0.1f);
        lv_label_set_text(lbl_pump, buf);

        snprintf(buf, sizeof(buf), "Alim : %.1f V", heatingData.supply_voltage);
        lv_label_set_text(lbl_voltage, buf);
    } else {
        // Pas de données
        snprintf(buf, sizeof(buf), LV_SYMBOL_REFRESH "  %s", heating_state_str());
        lv_label_set_text(lbl_status, buf);
        lv_obj_set_style_text_color(lbl_status, COLOR_GRAY, 0);
        lv_label_set_text(lbl_case_temp, "Chambre : -- °C");
        lv_label_set_text(lbl_fan, "Ventilateur : -- rpm");
        lv_label_set_text(lbl_pump, "Pompe diesel : -- Hz");
        lv_label_set_text(lbl_voltage, "Alim : -- V");
    }
}
