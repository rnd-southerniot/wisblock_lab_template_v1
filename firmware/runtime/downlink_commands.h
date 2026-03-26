/**
 * @file downlink_commands.h
 * @brief Downlink command parser and applier — binary protocol v1
 * @version 1.0
 * @date 2026-02-25
 *
 * Parses ChirpStack downlink frames (fport=10), applies safe runtime
 * changes (poll interval, slave ID), and builds ACK uplink payloads.
 *
 * Binary command schema:
 *   [0xD1] [0x01] [CMD] [PAYLOAD...]
 *
 * Does NOT include <LoRaWan-Arduino.h>. HAL v2 layering preserved.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

/* Forward declaration — avoids circular include */
class SystemManager;

/* ============================================================
 * Protocol Constants
 * ============================================================ */
#define DL_MAGIC      0xD1
#define DL_VERSION    0x01

/* ============================================================
 * Command Codes
 * ============================================================ */
#define DL_CMD_SET_INTERVAL  0x01
#define DL_CMD_SET_SLAVE_ID  0x02
#define DL_CMD_REQ_STATUS    0x03
#define DL_CMD_REBOOT        0x04

/* ============================================================
 * Validation Bounds
 * ============================================================ */
#define DL_INTERVAL_MIN_MS   1000
#define DL_INTERVAL_MAX_MS   3600000
#define DL_SLAVE_ID_MIN      1
#define DL_SLAVE_ID_MAX      247
#define DL_REBOOT_SAFETY_KEY 0xA5

/* ============================================================
 * LoRaWAN Ports
 * ============================================================ */
#define DL_CMD_FPORT         10   /* Downlink command port */
#define DL_ACK_FPORT         11   /* ACK uplink port */

/* ============================================================
 * Result Struct
 * ============================================================ */
struct DlResult {
    bool     valid;            /* true if command was parsed and applied */
    bool     needs_ack;        /* true if an ACK uplink should be sent */
    uint8_t  ack_payload[20];  /* Pre-built ACK frame */
    uint8_t  ack_len;          /* ACK frame length */
    bool     needs_reboot;     /* true if reboot requested (caller should delay then reset) */
};

/**
 * Parse a downlink frame, apply the command to SystemManager, and
 * build the ACK uplink payload.
 *
 * @param buf       Raw downlink bytes
 * @param len       Downlink length
 * @param fport     LoRaWAN fport
 * @param sys       SystemManager reference (for applying changes)
 * @param out_msg   Human-readable result string (for serial logging)
 * @param out_msg_len  Size of out_msg buffer
 * @param result    Output — ACK payload and flags
 * @return true if command was recognized (even if NACK)
 */
bool dl_parse_and_apply(const uint8_t* buf, uint8_t len, uint8_t fport,
                        SystemManager& sys,
                        char* out_msg, size_t out_msg_len,
                        DlResult& result);
