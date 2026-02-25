/**
 * @file lorawan_hal_nrf52.cpp
 * @brief LoRaWAN HAL — nRF52840 (RAK4631) implementation
 * @version 1.0
 * @date 2026-02-25
 *
 * Implements lorawan_hal.h for the RAK4631 (nRF52840) platform.
 * Uses lora_rak4630_init() one-liner — BSP handles all SX1262 pins.
 * Requires -DRAK4630 and -D_VARIANT_RAK4630_ build flags.
 *
 * Guarded by #ifdef PLATFORM_NRF52 — compiles to empty TU on ESP32.
 */

#include "platform_select.h"

#ifdef PLATFORM_NRF52

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include "lorawan_hal.h"

bool lorawan_hal_init(void) {
    /* RAK4631: BSP one-liner — all SX1262 pins handled internally.
     * Requires -DRAK4630 and -D_VARIANT_RAK4630_ build flags. */
    uint32_t hw_err = lora_rak4630_init();
    return (hw_err == 0);
}

#endif /* PLATFORM_NRF52 */
