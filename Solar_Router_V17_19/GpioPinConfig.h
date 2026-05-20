// ============================================
// F1ATB Maison - GPIO Configuration
// ESP32 Pin Mapping
// ============================================

#ifndef GPIO_PIN_CONFIG_H
#define GPIO_PIN_CONFIG_H

// ============================================
// DIMMER MODULE (Triac Control)
// ============================================
#define GPIO_ZC_DETECT    34    // Zero-Cross Detection (LTV-814S)
#define GPIO_PULSE_TRIAC  17    // Triac Pulse Output → 220Ω → Pin4 MOC3021S → BTA Gate

// ============================================
// SOLID STATE RELAY (SSR)
// ============================================
#define GPIO_SSR_RELAY    32    // SSR Control Output

// ============================================
// LINKY METER
// ============================================
#define GPIO_LINKY_RX     16    // Serial RX from Linky

// ============================================
// ETHERNET W5500
// ============================================
#define GPIO_W5500_CS     5     // Chip Select (Broche 10)
#define GPIO_W5500_MOSI   13    // MOSI (Broche 8)
#define GPIO_W5500_MISO   15    // MISO (Broche 9)
#define GPIO_W5500_CLK    14    // Clock (Broche 7)
#define GPIO_W5500_INT    25    // Interrupt (signale paquet reçu → ISR)

// ============================================
// JSY-MK-194T POWER METER
// ============================================
#define GPIO_JSY_RX       27    // Serial RX
#define GPIO_JSY_TX       26    // Serial TX

// ============================================
// STATUS LEDs
// ============================================
#define GPIO_LED_RED      19    // Red Status LED
#define GPIO_LED_GREEN    18    // Green Status LED

// ============================================
// DS18B20 TEMPERATURE SENSOR
// ============================================
#define GPIO_TEMP_SENSOR1 4     // Temperature Sensor 1 (OneWire)

// ============================================
// 5V FAN (PWM)
// ============================================
#define GPIO_FAN_PWM      33    // Fan Control PWM

// ============================================
// OPTOCOUPLERS
// ============================================
#define GPIO_MOC3021S     22    // MOC3021S (Triac driver interface)
#define GPIO_LTV814S      23    // LTV-814S (AC detection/Zero-cross)

// ============================================
// RESERVED / FUTURE
// ====================