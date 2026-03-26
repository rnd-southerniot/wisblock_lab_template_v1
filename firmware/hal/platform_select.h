/**
 * @file platform_select.h
 * @brief Maps core-level defines to platform-level defines
 * @version 1.0
 * @date 2026-02-25
 *
 * Build system defines CORE_RAK4631 or CORE_RAK3312.
 * This header maps those to PLATFORM_NRF52 or PLATFORM_ESP32
 * so that HAL implementations can use platform-level guards
 * without knowing about specific WisBlock product SKUs.
 *
 * Include this header in all HAL implementation files.
 */

#pragma once

#if defined(CORE_RAK4631)
    #define PLATFORM_NRF52
#elif defined(CORE_RAK3312)
    #define PLATFORM_ESP32
#else
    #error "No core defined. Set -DCORE_RAK4631 or -DCORE_RAK3312 in build_flags."
#endif
