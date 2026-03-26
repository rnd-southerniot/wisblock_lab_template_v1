/**
 * @file downlink_commands.cpp
 * @brief Downlink command parser and applier — Implementation
 * @version 1.0
 * @date 2026-02-25
 *
 * Implements dl_parse_and_apply():
 *   1. Validates MAGIC, VERSION, minimum length
 *   2. Switches on CMD byte
 *   3. Applies command to SystemManager subsystems
 *   4. Builds ACK payload in result struct
 *   5. Writes human-readable message to out_msg
 *
 * Does NOT include <LoRaWan-Arduino.h>. HAL v2 layering preserved.
 */

#include <Arduino.h>
#include <string.h>
#include "downlink_commands.h"
#include "system_manager.h"

#ifdef HAS_STORAGE_HAL
#include "storage_hal.h"
#endif

/* ============================================================
 * Helper: Build ACK header (MAGIC + VERSION + CMD + STATUS)
 * ============================================================ */
static uint8_t ack_header(uint8_t* buf, uint8_t cmd, uint8_t status) {
    buf[0] = DL_MAGIC;
    buf[1] = DL_VERSION;
    buf[2] = cmd;
    buf[3] = status;
    return 4;
}

/* ============================================================
 * Helper: Encode uint32_t big-endian into buffer
 * ============================================================ */
static void encode_be32(uint8_t* buf, uint32_t val) {
    buf[0] = (uint8_t)(val >> 24);
    buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >>  8);
    buf[3] = (uint8_t)(val      );
}

/* ============================================================
 * Helper: Decode uint32_t big-endian from buffer
 * ============================================================ */
static uint32_t decode_be32(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 24)
         | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] <<  8)
         | ((uint32_t)buf[3]      );
}

/* ============================================================
 * dl_parse_and_apply()
 * ============================================================ */
bool dl_parse_and_apply(const uint8_t* buf, uint8_t len, uint8_t fport,
                        SystemManager& sys,
                        char* out_msg, size_t out_msg_len,
                        DlResult& result) {

    /* Initialize result */
    memset(&result, 0, sizeof(result));

    /* ---- Silent rejection rules ---- */

    /* Frame too short for header */
    if (len < 3) {
        snprintf(out_msg, out_msg_len, "Ignored: frame too short (%d bytes)", len);
        return false;
    }

    /* Bad magic */
    if (buf[0] != DL_MAGIC) {
        snprintf(out_msg, out_msg_len, "Ignored: bad magic 0x%02X", buf[0]);
        return false;
    }

    /* Bad version */
    if (buf[1] != DL_VERSION) {
        snprintf(out_msg, out_msg_len, "Ignored: bad version 0x%02X", buf[1]);
        return false;
    }

    uint8_t cmd = buf[2];

    /* ---- Command dispatch ---- */

    switch (cmd) {

    /* --------------------------------------------------------
     * CMD 0x01: SET_INTERVAL — uint32_t BE (ms)
     * -------------------------------------------------------- */
    case DL_CMD_SET_INTERVAL: {
        /* Need 4 payload bytes after header */
        if (len < 7) {
            /* NACK — payload too short */
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK SET_INTERVAL: payload too short (%d bytes)", len);
            return true;
        }

        uint32_t new_interval = decode_be32(&buf[3]);

        /* Validate range */
        if (new_interval < DL_INTERVAL_MIN_MS || new_interval > DL_INTERVAL_MAX_MS) {
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK SET_INTERVAL: %lu ms out of range [%d..%d]",
                     (unsigned long)new_interval, DL_INTERVAL_MIN_MS, DL_INTERVAL_MAX_MS);
            return true;
        }

        /* Apply */
        bool ok = sys.scheduler().setInterval(0, new_interval);
        if (!ok) {
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK SET_INTERVAL: scheduler setInterval failed");
            return true;
        }

#ifdef HAS_STORAGE_HAL
        storage_hal_write_u32(STORAGE_KEY_INTERVAL, new_interval);
#endif

        /* ACK with current value */
        result.valid = true;
        result.needs_ack = true;
        result.ack_len = ack_header(result.ack_payload, cmd, 0x01);
        encode_be32(&result.ack_payload[4], new_interval);
        result.ack_len += 4;  /* Total: 8 bytes */

        snprintf(out_msg, out_msg_len, "ACK SET_INTERVAL: %lu ms", (unsigned long)new_interval);
        return true;
    }

    /* --------------------------------------------------------
     * CMD 0x02: SET_SLAVE_ID — uint8_t
     * -------------------------------------------------------- */
    case DL_CMD_SET_SLAVE_ID: {
        /* Need 1 payload byte after header */
        if (len < 4) {
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK SET_SLAVE_ID: payload too short (%d bytes)", len);
            return true;
        }

        uint8_t new_addr = buf[3];

        /* Validate range */
        if (new_addr < DL_SLAVE_ID_MIN || new_addr > DL_SLAVE_ID_MAX) {
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK SET_SLAVE_ID: addr %d out of range [%d..%d]",
                     new_addr, DL_SLAVE_ID_MIN, DL_SLAVE_ID_MAX);
            return true;
        }

        /* Apply */
        sys.peripheral().setSlaveAddr(new_addr);

#ifdef HAS_STORAGE_HAL
        storage_hal_write_u8(STORAGE_KEY_SLAVE_ADDR, new_addr);
#endif

        /* ACK with current value */
        result.valid = true;
        result.needs_ack = true;
        result.ack_len = ack_header(result.ack_payload, cmd, 0x01);
        result.ack_payload[4] = new_addr;
        result.ack_len += 1;  /* Total: 5 bytes */

        snprintf(out_msg, out_msg_len, "ACK SET_SLAVE_ID: addr=%d", new_addr);
        return true;
    }

    /* --------------------------------------------------------
     * CMD 0x03: REQ_STATUS — no payload
     * -------------------------------------------------------- */
    case DL_CMD_REQ_STATUS: {
        const RuntimeStats& st = sys.stats();
        uint8_t slave_addr = sys.peripheral().slaveAddr();
        uint32_t interval = sys.scheduler().taskInterval(0);

        /* ACK header */
        result.valid = true;
        result.needs_ack = true;
        result.ack_len = ack_header(result.ack_payload, cmd, 0x01);

        /* Payload: slave_id(1) + interval_ms BE(4) + uplink_ok BE(4) + modbus_ok BE(4) = 13 bytes */
        result.ack_payload[4] = slave_addr;
        encode_be32(&result.ack_payload[5], interval);
        encode_be32(&result.ack_payload[9], st.uplink_ok);
        encode_be32(&result.ack_payload[13], st.modbus_ok);
        result.ack_len += 13;  /* Total: 17 bytes */

        snprintf(out_msg, out_msg_len,
                 "ACK REQ_STATUS: slave=%d, interval=%lu, uplink_ok=%lu, modbus_ok=%lu",
                 slave_addr, (unsigned long)interval,
                 (unsigned long)st.uplink_ok, (unsigned long)st.modbus_ok);
        return true;
    }

    /* --------------------------------------------------------
     * CMD 0x04: REBOOT — uint8_t safety key
     * -------------------------------------------------------- */
    case DL_CMD_REBOOT: {
        /* Need 1 payload byte after header */
        if (len < 4) {
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK REBOOT: payload too short (%d bytes)", len);
            return true;
        }

        uint8_t safety_key = buf[3];

        if (safety_key != DL_REBOOT_SAFETY_KEY) {
            /* NACK — invalid safety key */
            result.needs_ack = true;
            result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
            snprintf(out_msg, out_msg_len, "NACK REBOOT: invalid safety key 0x%02X", safety_key);
            return true;
        }

        /* ACK with safety key echo, then set reboot flag */
        result.valid = true;
        result.needs_ack = true;
        result.needs_reboot = true;
        result.ack_len = ack_header(result.ack_payload, cmd, 0x01);
        result.ack_payload[4] = DL_REBOOT_SAFETY_KEY;
        result.ack_len += 1;  /* Total: 5 bytes */

        snprintf(out_msg, out_msg_len, "ACK REBOOT: safety key valid, reboot pending");
        return true;
    }

    /* --------------------------------------------------------
     * Unknown CMD — NACK
     * -------------------------------------------------------- */
    default: {
        result.needs_ack = true;
        result.ack_len = ack_header(result.ack_payload, cmd, 0x00);
        snprintf(out_msg, out_msg_len, "NACK: unknown command 0x%02X", cmd);
        return true;
    }

    }  /* switch */
}
