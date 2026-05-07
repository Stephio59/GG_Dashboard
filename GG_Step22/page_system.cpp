/*
 * page_system.cpp — Page Système avec BLE Scanner
 * 
 * Layout :
 *   - Gauche (480x480)   : Infos système + slider luminosité
 *   - Droite (530x480)   : Liste BLE détectés
 */
#include "ui.h"
#include "lv_fonts_fr.h"
#include "display_config.h"
#include "ble_scanner.h"
#include "sd_card.h"
#include <WiFi.h>

static lv_obj_t* lbl_sys_info      = nullptr;
static lv_obj_t* lbl_brightness    = nullptr;
// Globale (pas static) pour que screen_sleep.cpp puisse y acceder
lv_obj_t* brightness_mask   = nullptr;
static lv_obj_t* ble_list_label    = nullptr;
static lv_obj_t* ble_count_label   = nullptr;

// ── Callback slider luminosité ─────────────────────────
static void _brightness_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    int opa = (100 - val) * 2;
    if (brightness_mask) lv_obj_set_style_bg_opa(brightness_mask, opa, 0);
    if (lbl_brightness) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Luminosité : %d%%", val);
        lv_label_set_text(lbl_brightness, buf);
    }
}

void page_system_build(lv_obj_t* parent) {
    ui_draw_bg(parent);

    // ═══ Titre ═══
    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS "  Système");
    lv_obj_set_style_text_font(title, &FONT_36, 0);
    lv_obj_set_style_text_color(title, COLOR_WHITE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // ═══ GAUCHE : infos système (500x400) ═══
    lv_obj_t* card_info = lv_obj_create(parent);
    lv_obj_set_size(card_info, 480, 400);
    lv_obj_set_pos(card_info, 10, 60);
    lv_obj_add_style(card_info, &ui.style_card, 0);
    lv_obj_clear_flag(card_info, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t_info = lv_label_create(card_info);
    lv_label_set_text(t_info, LV_SYMBOL_WIFI "  Informations");
    lv_obj_set_style_text_font(t_info, &FONT_22, 0);
    lv_obj_set_style_text_color(t_info, COLOR_ACCENT, 0);
    lv_obj_align(t_info, LV_ALIGN_TOP_LEFT, 8, 2);

    lbl_sys_info = lv_label_create(card_info);
    lv_label_set_text(lbl_sys_info, "Chargement...");
    lv_obj_set_style_text_font(lbl_sys_info, &FONT_18, 0);
    lv_obj_set_style_text_color(lbl_sys_info, COLOR_WHITE, 0);
    lv_obj_align(lbl_sys_info, LV_ALIGN_TOP_LEFT, 10, 40);

    // Slider luminosité en bas
    lv_obj_t* lbl_b_title = lv_label_create(card_info);
    lv_label_set_text(lbl_b_title, LV_SYMBOL_IMAGE "  Luminosité");
    lv_obj_set_style_text_font(lbl_b_title, &FONT_20, 0);
    lv_obj_set_style_text_color(lbl_b_title, COLOR_ACCENT, 0);
    lv_obj_align(lbl_b_title, LV_ALIGN_BOTTOM_LEFT, 8, -85);

    lbl_brightness = lv_label_create(card_info);
    lv_label_set_text(lbl_brightness, "Luminosité : 100%");
    lv_obj_set_style_text_font(lbl_brightness, &FONT_16, 0);
    lv_obj_set_style_text_color(lbl_brightness, COLOR_WHITE, 0);
    lv_obj_align(lbl_brightness, LV_ALIGN_BOTTOM_LEFT, 8, -55);

    lv_obj_t* slider = lv_slider_create(card_info);
    lv_obj_set_size(slider, 440, 25);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, _brightness_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Overlay noir global (singleton)
    if (!brightness_mask) {
        brightness_mask = lv_obj_create(lv_scr_act());
        lv_obj_set_size(brightness_mask, SCREEN_WIDTH, SCREEN_HEIGHT);
        lv_obj_set_pos(brightness_mask, 0, 0);
        lv_obj_set_style_bg_color(brightness_mask, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(brightness_mask, 0, 0);
        lv_obj_set_style_border_width(brightness_mask, 0, 0);
        lv_obj_set_style_radius(brightness_mask, 0, 0);
        lv_obj_add_flag(brightness_mask, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_clear_flag(brightness_mask, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(brightness_mask, LV_OBJ_FLAG_SCROLLABLE);
    }

    // ═══ DROITE : BLE Scanner (520x400) ═══
    lv_obj_t* card_ble = lv_obj_create(parent);
    lv_obj_set_size(card_ble, 520, 400);
    lv_obj_set_pos(card_ble, 495, 60);
    lv_obj_add_style(card_ble, &ui.style_card, 0);
    lv_obj_clear_flag(card_ble, LV_OBJ_FLAG_SCROLLABLE);

    // Titre + icône BT
    lv_obj_t* ble_icon = lv_label_create(card_ble);
    lv_label_set_text(ble_icon, ICON_BLUETOOTH);
    lv_obj_set_style_text_font(ble_icon, &ICON_FONT_22, 0);
    lv_obj_set_style_text_color(ble_icon, COLOR_BLUE, 0);
    lv_obj_align(ble_icon, LV_ALIGN_TOP_LEFT, 8, 4);

    lv_obj_t* t_ble = lv_label_create(card_ble);
    lv_label_set_text(t_ble, "Scanner BLE");
    lv_obj_set_style_text_font(t_ble, &FONT_22, 0);
    lv_obj_set_style_text_color(t_ble, COLOR_ACCENT, 0);
    lv_obj_align(t_ble, LV_ALIGN_TOP_LEFT, 36, 2);

    ble_count_label = lv_label_create(card_ble);
    lv_label_set_text(ble_count_label, "Scan #0");
    lv_obj_set_style_text_font(ble_count_label, &FONT_14, 0);
    lv_obj_set_style_text_color(ble_count_label, COLOR_GRAY, 0);
    lv_obj_align(ble_count_label, LV_ALIGN_TOP_RIGHT, -8, 6);

    // Zone scrollable pour la liste
    lv_obj_t* scroll_area = lv_obj_create(card_ble);
    lv_obj_set_size(scroll_area, 504, 350);
    lv_obj_align(scroll_area, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_opa(scroll_area, 0, 0);
    lv_obj_set_style_border_width(scroll_area, 0, 0);
    lv_obj_set_style_pad_all(scroll_area, 2, 0);
    lv_obj_set_scrollbar_mode(scroll_area, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(scroll_area, LV_DIR_VER);

    ble_list_label = lv_label_create(scroll_area);
    lv_label_set_text(ble_list_label, "En attente du premier scan...");
    lv_obj_set_style_text_font(ble_list_label, &FONT_14, 0);
    lv_obj_set_style_text_color(ble_list_label, COLOR_WHITE, 0);
    lv_obj_set_width(ble_list_label, 498);
    lv_label_set_long_mode(ble_list_label, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(ble_list_label, true);
}

void page_system_update() {
    // Infos système
    if (lbl_sys_info) {
        // Refresh des stats SD (espace libre peut bouger)
        sd_card_refresh_stats();
        
        char buf[480];
        char sd_line[120];
        if (sdCard.present && sdCard.mounted) {
            snprintf(sd_line, sizeof(sd_line),
                "SD      : %s  %llu/%llu MB libres",
                sdCard.type, sdCard.available_mb, sdCard.total_mb);
        } else {
            snprintf(sd_line, sizeof(sd_line), "SD      : Absente");
        }
        
        snprintf(buf, sizeof(buf),
            "WiFi    : %s\n"
            "IP      : %s\n"
            "RSSI    : %d dBm\n"
            "Uptime  : %lu s\n"
            "Heap    : %u KB libre\n"
            "PSRAM   : %u KB libre\n"
            "%s",
            WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "Déconnecté",
            WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "-",
            WiFi.RSSI(),
            millis() / 1000,
            ESP.getFreeHeap() / 1024,
            ESP.getFreePsram() / 1024,
            sd_line
        );
        lv_label_set_text(lbl_sys_info, buf);
    }

    // BLE scan count
    if (ble_count_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Scan #%lu  (%d vus)",
            bleScanState.total_scans, bleScanState.count);
        lv_label_set_text(ble_count_label, buf);
    }

    // BLE list - avec recolor pour les connus
    if (ble_list_label && bleScanState.count > 0) {
        // Construire le texte avec recolor LVGL
        // Format : "#00FF00 MAC#  Label\n  RSSI -XX dBm"
        static char buf[2048];
        buf[0] = 0;
        size_t off = 0;
        
        // 1) Afficher d'abord les appareils connus (trier)
        for (int i = 0; i < bleScanState.count; i++) {
            const BleDeviceInfo& d = bleScanState.devices[i];
            if (!d.is_known) continue;
            
            const char* color;
            const char* symbol;
            if (d.rssi == 0) {
                color  = "#DA3633";  // rouge : jamais vu
                symbol = "x";
            } else if (d.rssi >= -70) {
                color  = "#2EA043";  // vert : bon signal
                symbol = "OK";
            } else {
                color  = "#F7CC00";  // jaune : faible
                symbol = "~";
            }
            
            int n = snprintf(buf + off, sizeof(buf) - off,
                "#%s [%s] %s#\n"
                "  %s  RSSI: %d dBm\n",
                color, symbol, d.known_label,
                d.mac, d.rssi);
            if (n < 0 || (size_t)n >= sizeof(buf) - off) break;
            off += n;
        }
        
        // Séparateur
        int n = snprintf(buf + off, sizeof(buf) - off,
            "\n#8B949E --- Autres ---#\n");
        if (n > 0) off += n;
        
        // 2) Autres appareils détectés
        int shown = 0;
        for (int i = 0; i < bleScanState.count && shown < 8; i++) {
            const BleDeviceInfo& d = bleScanState.devices[i];
            if (d.is_known) continue;
            if (d.rssi == 0) continue;
            
            int n2 = snprintf(buf + off, sizeof(buf) - off,
                "#8B949E %s  %d dBm  %s#\n",
                d.mac, d.rssi, (d.name[0] ? d.name : "?"));
            if (n2 < 0 || (size_t)n2 >= sizeof(buf) - off) break;
            off += n2;
            shown++;
        }
        
        lv_label_set_text(ble_list_label, buf);
    }
}
