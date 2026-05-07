#pragma once
/*
 * ble_task.h — Tâche FreeRTOS dédiée BLE sur Core 0
 * 
 * Isole toutes les opérations BLE (scan + JKBMS GATT) du core 1 où tourne
 * LVGL. Évite le freeze de l'UI pendant les opérations BLE.
 */
#include <Arduino.h>

void ble_task_start();
