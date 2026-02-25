/**
 * @file lorawan_hal_esp32.cpp
 * @brief LoRaWAN HAL — ESP32-S3 (RAK3312) implementation
 * @version 1.0
 * @date 2026-02-25
 *
 * Implements lorawan_hal.h for the RAK3312 (ESP32-S3) platform.
 * Builds hw_config struct from gate_config.h pin defines and calls
 * lora_hardware_init(hwConfig).
 *
 * Guarded by #ifdef PLATFORM_ESP32 — compiles to empty TU on nRF52.
 */

#include "platform_select.h"

#ifdef PLATFORM_ESP32

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include "lorawan_hal.h"
#include "gate_config.h"

bool lorawan_hal_init(void) {
    /* RAK3312: Manual SX1262 pin mapping from gate_config.h */
    hw_config hwConfig;
    hwConfig.CHIP_TYPE           = SX1262_CHIP;
    hwConfig.PIN_LORA_RESET      = GATE_LORA_PIN_RESET;
    hwConfig.PIN_LORA_NSS        = GATE_LORA_PIN_NSS;
    hwConfig.PIN_LORA_SCLK       = GATE_LORA_PIN_SCK;
    hwConfig.PIN_LORA_MISO       = GATE_LORA_PIN_MISO;
    hwConfig.PIN_LORA_DIO_1      = GATE_LORA_PIN_DIO1;
    hwConfig.PIN_LORA_BUSY       = GATE_LORA_PIN_BUSY;
    hwConfig.PIN_LORA_MOSI       = GATE_LORA_PIN_MOSI;
    hwConfig.RADIO_TXEN          = -1;
    hwConfig.RADIO_RXEN          = GATE_LORA_PIN_ANT_SW;
    hwConfig.USE_DIO2_ANT_SWITCH = true;   /* Proven in Gate 5 */
    hwConfig.USE_DIO3_TCXO       = true;   /* Proven in Gate 5 */
    hwConfig.USE_DIO3_ANT_SWITCH = false;
    hwConfig.USE_LDO             = false;
    hwConfig.USE_RXEN_ANT_PWR    = false;

    /* Initialize SX1262 hardware (SPI, timers, IRQ) */
    uint32_t hw_err = lora_hardware_init(hwConfig);
    return (hw_err == 0);
}

#endif /* PLATFORM_ESP32 */
