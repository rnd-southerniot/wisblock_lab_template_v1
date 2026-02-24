/**
 * @file transport_interface.h
 * @brief Abstract interface for uplink transport (LoRaWAN, WiFi, BLE, etc.)
 * @version 1.0
 * @date 2026-02-24
 *
 * Pure virtual interface for uplink sinks.
 * Implementations: LoRaTransport, (future: WiFiTransport, BLETransport, etc.)
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>

/**
 * Abstract interface for uplink transport (LoRaWAN, WiFi, BLE, etc.)
 */
class TransportInterface {
public:
    virtual ~TransportInterface() = default;

    /** Initialize radio hardware + protocol stack. Returns true on success. */
    virtual bool init() = 0;

    /**
     * Connect to network (e.g., OTAA join). May block.
     * @param timeout_ms  Maximum time to wait for connection
     * @return true if connected
     */
    virtual bool connect(uint32_t timeout_ms) = 0;

    /** Check if transport is connected (e.g., joined) */
    virtual bool isConnected() const = 0;

    /**
     * Send data uplink.
     * @param data  Payload bytes
     * @param len   Payload length
     * @param port  Application port (1-223 for LoRaWAN)
     * @return true if send succeeded (queued for TX)
     */
    virtual bool send(const uint8_t* data, uint8_t len, uint8_t port) = 0;

    /** Human-readable name for logging */
    virtual const char* name() const = 0;
};
