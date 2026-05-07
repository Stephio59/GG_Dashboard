/*
 * GG Van Dashboard - Step 3
 * Navigation 5 pages + WiFi Multi (maison + van + camping)
 */
#include "rgb_lcd_port.h"
#include "gui_paint.h"
#include "image.h"
#include "user_wifi.h"
#include "gt911.h"
#include "i2c.h"
#include "io_extension.h"
#include "sd_card.h"
#include "sd_lvgl_fs.h"
#include "icons_sd.h"
#include "theme_mode.h"
#include "user_config.h"
#include <time.h>
#include <lvgl.h>
#include "lv_conf.h"
#include "config.h"
#include "ui.h"
#include "bme_wire1.h"
#include "scd41.h"
#include "pir_sensor.h"
#include "screen_sleep.h"
#include "history.h"
#include "weather.h"
#include "ble_task.h"
#include "water_state.h"
#include "gps.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include "espnow_master.h"

WiFiMulti wifiMulti;

// ── Reseaux WiFi connus ──────────────────────────────────────
#define WIFI_SSID_1   "TP-Link_92E9"          // Maison TP-Link
#define WIFI_PASS_1   "stephio59"
#define WIFI_SSID_2   "Freebox-5C6D45"        // Maison Freebox
#define WIFI_PASS_2   "r4792qmsmvqxn9trq3rsm3"

static UBYTE *lvgl_fb = NULL;

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    uint16_t *dst = (uint16_t *)lvgl_fb + area->y1 * EXAMPLE_LCD_H_RES + area->x1;
    uint16_t *src = (uint16_t *)color_map;
    for (uint32_t row = 0; row < h; row++) {
        memcpy(dst, src, w * 2);
        dst += EXAMPLE_LCD_H_RES;
        src += w;
    }
    wavesahre_rgb_lcd_display(lvgl_fb);
    lv_disp_flush_ready(drv);
}

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    touch_gt911_point_t point = touch_gt911_read_point(1);
    if (point.cnt > 0) {
        data->point.x = point.x[0];
        data->point.y = point.y[0];
        data->state = LV_INDEV_STATE_PRESSED;
        // Tout touch reset le timer d'inactivité
        display_force_on();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static uint32_t t_update    = 0;
static uint32_t t_wifi_check = 0;
static uint32_t t_espnow_ping = 0;

void setup() {
    Serial0.begin(115200);
    // Attendre que le port USB-CDC soit prêt (max 5s)
    uint32_t t0 = millis();
    while (!Serial0 && (millis() - t0) < 5000) {
        delay(50);
    }
    delay(500);
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("   GG Van Dashboard - Step 22 boot");
    Serial0.println("========================================");
    Serial0.flush();
    if (psramFound()) Serial0.println("[Boot] PSRAM OK");
    Serial0.flush();

    DEV_I2C_Init();
    Serial0.println("[Boot] I2C init OK");
    Serial0.flush();

    IO_EXTENSION_Init();
    Serial0.println("[Boot] IO extension OK");
    
    // ── Carte SD (juste apres IO_EXTENSION car la SD utilise EXIO4) ──
    if (sd_card_init()) {
        Serial0.println("[Boot] SD card OK");
        sd_card_list_root();
    } else {
        Serial0.println("[Boot] SD card absente ou erreur (le dashboard fonctionne sans)");
    }
    Serial0.flush();
    
    // ── Charger configuration utilisateur depuis SD ──
    config_load();
    Serial0.flush();

    waveshare_esp32_s3_rgb_lcd_init();
    Serial0.println("[Boot] RGB LCD OK");
    Serial0.flush();

    wavesahre_rgb_lcd_bl_on();
    touch_gt911_init();
    Serial0.println("[Boot] Touch GT911 OK");
    Serial0.flush();

    UDOUBLE Imagesize = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2;
    lvgl_fb = (UBYTE *)heap_caps_malloc(Imagesize, MALLOC_CAP_SPIRAM);
    if (lvgl_fb == NULL) { Serial0.println("[Boot] malloc fail!"); exit(0); }
    memset(lvgl_fb, 0, Imagesize);
    wavesahre_rgb_lcd_display(lvgl_fb);
    Serial0.println("[Boot] LCD framebuffer OK");
    Serial0.flush();

    lv_init();

    // Buffer LVGL : 80 lignes (plus fluide que 40)
    static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
        EXAMPLE_LCD_H_RES * 80 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    static lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(
        EXAMPLE_LCD_H_RES * 80 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 80);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res  = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);
    Serial0.println("[Boot] LVGL OK");
    
    // Driver LVGL filesystem pour SD card (lettre 'S:')
    sd_lvgl_fs_init();
    
    // Charger les icones depuis la SD (best effort)
    icons_sd_load_all();

    ui.begin();
    Serial0.println("[Boot] UI OK");

    // BME280 sur Wire1 (GPIO 21/22)
    bme_wire1_init();
    Serial0.println("[Boot] BME280 OK");

    // SCD41 (CO2) sur bus I2C partagé
    delay(50);  // Laisser le SCD41 finir son boot interne
    scd41_init();
    Serial0.println("[Boot] SCD41 OK");

    // PIR sur GPIO 6
    pir_init();
    Serial0.println("[Boot] PIR OK");
    
    screen_sleep_init();
    Serial0.println("[Boot] Screen sleep manager OK");
    
    history_init();
    Serial0.println("[Boot] History buffers OK");
    Serial0.println("[Boot] PIR OK");

    // Eau (CBE placeholder - sera remplace par ESP-NOW)
    water_init();
    Serial0.println("[Boot] Water init OK");
    
    // GPS NEO-6M
    gps_init();
    Serial0.println("[Boot] GPS init OK");

    // ── WiFi Multi-réseaux ──────────────────────────────────
    WiFi.mode(WIFI_STA);
    delay(100);
    wifiMulti.addAP(userConfig.wifi_ssid_1, userConfig.wifi_pass_1);
    wifiMulti.addAP(userConfig.wifi_ssid_2, userConfig.wifi_pass_2);
    Serial0.println("[Boot] Scan WiFi en cours (max 15s)...");
    
    // Tentative connexion avec timeout 15s
    uint32_t wifi_start = millis();
    uint8_t wifi_status = WL_DISCONNECTED;
    while ((millis() - wifi_start) < 15000) {
        wifi_status = wifiMulti.run(5000);  // timeout 5s par essai
        if (wifi_status == WL_CONNECTED) break;
        Serial0.printf("[Boot] WiFi tentative... status=%d (5s)\n", wifi_status);
        delay(500);
    }
    
    if (wifi_status == WL_CONNECTED) {
        Serial0.printf("[Boot] WiFi OK: %s (IP %s) RSSI=%d dBm\n",
            WiFi.SSID().c_str(),
            WiFi.localIP().toString().c_str(),
            WiFi.RSSI());
        // NTP : UTC+1 (Paris hiver) + DST géré auto
        configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
        Serial0.println("[Boot] NTP configure");
    } else {
        Serial0.println("[Boot] *** WiFi NON connecte (verifie SSID/passwords) ***");
        Serial0.println("[Boot] Le dashboard fonctionnera sans WiFi");
        // Lister ce qu'on voit pour aider le diagnostic
        Serial0.println("[Boot] Reseaux WiFi visibles autour:");
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n && i < 8; i++) {
            Serial0.printf("  - %s (RSSI %d dBm) %s\n",
                WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "[OPEN]" : "[secured]");
        }
    }

    // ── ESP-NOW (master) ─────────────────────────────────────
    Serial0.println("[Boot] Init ESP-NOW...");
    espnow_master_init();

    // BLE Scanner + JKBMS sur Core 0 (tâche séparée) — APRES WiFi
    Serial0.println("[Boot] Demarrage tache BLE sur Core 0...");
    ble_task_start();

    Serial0.println("[Boot] PRET!");
    Serial0.printf("[Boot] Free heap: %u KB, PSRAM: %u KB\n",
        ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
}

static uint32_t t_sensors_ms = 0;
static uint32_t t_ui_update_ms = 0;

void loop() {
    uint32_t now = millis();

    // ══ PRIORITE 1 : LVGL + touch (le plus souvent possible) ══
    lv_timer_handler();

    // ══ PRIORITE 2 : capteurs lents (toutes 500ms) ══
    if (now - t_sensors_ms >= 500) {
        t_sensors_ms = now;
        bme_wire1_update();
        scd41_update();
        weather_update();
        weather_update_7days();
        water_update();
        
        // Une seule fois apres synchro NTP : recharger l'historique du jour depuis SD
        static bool history_csv_loaded = false;
        if (!history_csv_loaded && time(nullptr) > 8 * 3600) {
            history_load_from_sd();
            history_csv_loaded = true;
        }

        // Reconnexion WiFi auto
        if (now - t_wifi_check >= 30000) {
            t_wifi_check = now;
            wifiMulti.run();
        }
        
        // Ping ESP-NOW toutes les 5s pour que le slave sache qu'on est vivant
        if (now - t_espnow_ping >= 5000) {
            t_espnow_ping = now;
            espnow_send_ping();
        }
    }

    // ══ PRIORITE 3 : capteurs rapides (chaque iteration) ══
    pir_update();
    gps_update();
    screen_sleep_update();
    history_update();   // record toutes les 10 min
    
    // ══ Mode jour/nuit (check toutes les 30s) ══
    static uint32_t t_theme_ms = 0;
    if (now - t_theme_ms >= 30000) {
        t_theme_ms = now;
        theme_update_auto();
    }

    // ══ PRIORITE 4 : UI refresh valeurs (1 fois par seconde) ══
    if (now - t_ui_update_ms >= 1000) {
        t_ui_update_ms = now;
        ui.update();
    }

    delay(2);
}
