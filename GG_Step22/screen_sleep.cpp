/*
 * screen_sleep.cpp — Implémentation mise en veille écran
 */
#include "screen_sleep.h"
#include "user_config.h"
#include "rgb_lcd_port.h"   // wavesahre_rgb_lcd_bl_on/off
#include "lvgl.h"

// Variables d'état
static ScreenState  _state         = SCREEN_ON;
static uint32_t     _last_activity = 0;
static uint8_t      _user_brightness_pct = 100;

// Variable globale créée par page_system_build() (peut être nullptr si non encore initialisée)
extern lv_obj_t* brightness_mask;

static void _set_backlight(bool on) {
    if (on)  wavesahre_rgb_lcd_bl_on();
    else     wavesahre_rgb_lcd_bl_off();
}

void screen_sleep_init() {
    _state = SCREEN_ON;
    _last_activity = millis();
    _set_backlight(true);
    Serial0.println("[Sleep] Init OK (timeout DIM 90s, OFF 120s)");
}

void screen_sleep_wake() {
    _last_activity = millis();
    
    if (_state != SCREEN_ON) {
        // Réveil
        _set_backlight(true);
        _state = SCREEN_ON;
        Serial0.println("[Sleep] Reveil ecran");
    }
}

void screen_sleep_update() {
    uint32_t now = millis();
    uint32_t inactive_ms = now - _last_activity;
    
    switch (_state) {
        case SCREEN_ON:
            if (inactive_ms >= (userConfig.screen_dim_s * 1000UL)) {
                _state = SCREEN_DIM;
                // Dimming : on assombrit via brightness_mask (layer noir 50% opaque)
                if (brightness_mask) {
                    lv_obj_set_style_bg_opa(brightness_mask, LV_OPA_50, 0);
                }
                Serial0.println("[Sleep] Dimming (50%)");
            }
            break;
        case SCREEN_DIM:
            if (inactive_ms >= (userConfig.screen_off_s * 1000UL)) {
                _state = SCREEN_OFF;
                _set_backlight(false);
                Serial0.println("[Sleep] Ecran OFF");
            }
            break;
        case SCREEN_OFF:
            // Rien — on attend un wake()
            break;
    }
}

ScreenState screen_get_state() { return _state; }
bool screen_is_awake() { return _state == SCREEN_ON; }

void screen_set_user_brightness(uint8_t pct) {
    if (pct > 100) pct = 100;
    _user_brightness_pct = pct;
    // (déjà géré par brightness_mask dans page_system.cpp)
}

uint8_t screen_get_user_brightness() { return _user_brightness_pct; }
