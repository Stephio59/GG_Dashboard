/*
 * ble_task.cpp — Tâche BLE sur Core 0
 * 
 * L'UI LVGL + capteurs tournent sur Core 1 (setup/loop Arduino par défaut).
 * Le BLE tourne ici sur Core 0 pour ne pas bloquer l'UI.
 */
#include "ble_task.h"
#include "ble_scanner.h"
#include "jkbms_ble.h"
#include "victron_ble.h"
#include "heating_ble.h"

static TaskHandle_t ble_task_handle = nullptr;

static void ble_task_func(void* param) {
    Serial0.println("[BLE Task] Demarrage sur Core 0");
    Serial0.printf("[BLE Task] Core actuel: %d\n", xPortGetCoreID());

    Serial0.println("[BLE Task] Init scanner...");
    ble_scanner_init();
    Serial0.println("[BLE Task] Init Victron parser...");
    victron_init();
    Serial0.println("[BLE Task] Init JKBMS...");
    jkbms_init();
    Serial0.println("[BLE Task] Init Heating...");
    heating_init();
    Serial0.println("[BLE Task] Init termine, entree boucle");

    while (true) {
        ble_scanner_update();     // scan passif (Victron arrive via cette voie)
        jkbms_update();            // client GATT JKBMS
        heating_update();          // client GATT chauffage
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void ble_task_start() {
    BaseType_t ok = xTaskCreatePinnedToCore(
        ble_task_func,
        "BLE_Task",
        16384,
        nullptr,
        1,
        &ble_task_handle,
        0
    );
    if (ok != pdPASS) {
        Serial0.println("[BLE Task] ECHEC creation tache !");
    } else {
        Serial0.println("[BLE Task] Tache creee sur Core 0 (16KB stack)");
    }
}
