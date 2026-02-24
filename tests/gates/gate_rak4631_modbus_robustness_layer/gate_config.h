/**
 * @file gate_config.h
 * @brief Gate 4 — RAK4631 Modbus Robustness Layer Configuration
 * @version 1.0
 * @date 2026-02-24
 *
 * Defines all parameters for gate_rak4631_modbus_robustness_layer.
 * Extends Gate 3 with: retry mechanism, error classification,
 * multi-register reads, and UART recovery.
 *
 * Discovered device parameters are centralised here (from Gate 2 pass).
 * Gate runner references only these #defines.
 * Modbus frame layer and HAL are reused from Gate 3 (not duplicated).
 * No hardware_profile.h dependency — uses variant.h defines directly.
 */

#pragma once

#include <stdint.h>

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_rak4631_modbus_robustness_layer"
#define GATE_VERSION                    "1.0"
#define GATE_ID                         4

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE4-R4631]"
#define GATE_LOG_PASS                   "[PASS]"
#define GATE_LOG_FAIL                   "[FAIL]"
#define GATE_LOG_INFO                   "[INFO]"
#define GATE_LOG_STEP                   "[STEP]"

/* ============================================================
 * UART / Serial1 Pin Configuration (nRF52840)
 *
 * Serial1 TX/RX fixed by variant.h — no pin args to begin().
 *   PIN_SERIAL1_TX = 16 (P0.16)
 *   PIN_SERIAL1_RX = 15 (P0.15)
 * ============================================================ */

/* ============================================================
 * RS485 Control (RAK5802)
 *
 * RAK5802 uses TP8485E with HARDWARE auto-direction:
 *   WB_IO2 (pin 34) = 3V3_S power enable (powers RAK5802)
 *   WB_IO1 (pin 17) = NC (not connected to DE/RE on RAK5802)
 *   DE/RE driven automatically from UART TX line.
 *   No manual GPIO toggling required.
 * ============================================================ */
#define GATE_RS485_EN_PIN               34      /* WB_IO2 — 3V3_S power enable */

/* ============================================================
 * Previously Discovered Device Parameters (from Gate 2 pass)
 *
 * Gate 2 result: Slave=1, Baud=4800, Parity=NONE (8N1)
 * Response: 01 03 02 00 07 F9 86
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE           1
#define GATE_DISCOVERED_BAUD            4800
#define GATE_DISCOVERED_PARITY          0       /* 0=None, 2=Even (nRF52: no Odd) */

/* ============================================================
 * Multi-Register Configuration
 *
 * Read 2 consecutive holding registers starting at 0x0000.
 * Expected response: [addr, 0x03, byte_count=4, v0_hi, v0_lo,
 *                     v1_hi, v1_lo, crc_lo, crc_hi] = 9 bytes.
 * Validation: byte_count must equal 2 * GATE_MODBUS_QUANTITY.
 * ============================================================ */
#define GATE_MODBUS_QUANTITY            2       /* Number of registers to read */
#define GATE_MODBUS_QUANTITY_HI         0x00
#define GATE_MODBUS_QUANTITY_LO         0x02
#define GATE_MODBUS_TARGET_REG_HI       0x00    /* Start register high byte */
#define GATE_MODBUS_TARGET_REG_LO       0x00    /* Start register low byte = 0x0000 */
#define GATE_MODBUS_MAX_REGISTERS       8       /* Upper bound for buffer sizing */

/* ============================================================
 * Modbus RTU Protocol Constants (universal — same as Gate 3)
 *
 * These values are also defined in Gate 3's gate_config.h and
 * are used by the shared modbus_frame.cpp and hal_uart.cpp.
 * They MUST remain identical to Gate 3's definitions.
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD      0x03    /* Read Holding Registers */
#define GATE_MODBUS_REQ_FRAME_LEN       8       /* addr(1)+func(1)+reg(2)+qty(2)+crc(2) */
#define GATE_MODBUS_RESP_MIN_LEN        5       /* addr(1)+func(1)+data(1)+crc(2) minimum */
#define GATE_MODBUS_RESP_MAX_LEN        32      /* Generous buffer for response */
#define GATE_MODBUS_EXCEPTION_BIT       0x80    /* OR'd with function code in exception */

/* ============================================================
 * CRC-16/MODBUS Constants (universal — same as Gate 3)
 * ============================================================ */
#define GATE_CRC_INIT                   0xFFFF
#define GATE_CRC_POLY                   0xA001  /* Reflected polynomial */

/* ============================================================
 * Retry Configuration
 *
 * 3 retries after initial attempt = 4 total attempts maximum.
 * Retryable errors: TIMEOUT, CRC_FAIL only.
 * UART re-init triggered after GATE_CONSEC_FAIL_REINIT
 * consecutive retryable failures.
 * ============================================================ */
#define GATE_RETRY_MAX                  3       /* Max retry attempts (not counting initial) */
#define GATE_RETRY_DELAY_MS             50      /* Delay between retries */
#define GATE_CONSEC_FAIL_REINIT         2       /* Re-init UART after N consecutive failures */

/* ============================================================
 * Timing Parameters
 * ============================================================ */
#define GATE_RESPONSE_TIMEOUT_MS        200     /* Per-attempt response timeout */
#define GATE_UART_STABILIZE_MS          50      /* Delay after UART configuration */
#define GATE_UART_FLUSH_DELAY_MS        10      /* Delay after flush before next op */
#define GATE_INTER_BYTE_TIMEOUT_MS      5       /* Frame delimiter inter-byte gap */
#define GATE_TOTAL_TIMEOUT_MS           30000   /* Overall gate timeout: 30s */

/* ============================================================
 * Error Classification Enum
 *
 * Classifies each attempt's outcome. Used by retry logic to
 * decide: retry (TIMEOUT, CRC_FAIL) or abort (all others).
 * ============================================================ */
typedef enum {
    GATE_ERR_NONE        = 0,   /* Success — valid response received */
    GATE_ERR_TIMEOUT     = 1,   /* No response within timeout — retryable */
    GATE_ERR_CRC_FAIL    = 2,   /* Response CRC-16 mismatch — retryable */
    GATE_ERR_LENGTH_FAIL = 3,   /* Frame < 5 bytes or byte_count mismatch — non-retryable */
    GATE_ERR_EXCEPTION   = 4,   /* Modbus exception response — non-retryable */
    GATE_ERR_UART_ERROR  = 5    /* UART write or re-init failed — non-retryable */
} gate_error_t;

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                       0
#define GATE_FAIL_UART_INIT             1   /* Initial Serial1.begin() failed */
#define GATE_FAIL_TX_ERROR              2   /* hal_uart_write() failed (all attempts) */
#define GATE_FAIL_NO_RESPONSE           3   /* All retries exhausted — all timeouts */
#define GATE_FAIL_CRC_ERROR             4   /* All retries exhausted — all CRC failures */
#define GATE_FAIL_ADDR_MISMATCH         5   /* Slave address mismatch (non-retryable) */
#define GATE_FAIL_SHORT_FRAME           6   /* Response < 5 bytes (non-retryable) */
#define GATE_FAIL_SYSTEM_CRASH          7   /* Runner did not reach GATE COMPLETE */
#define GATE_FAIL_BYTE_COUNT            8   /* byte_count != 2 * quantity (non-retryable) */
#define GATE_FAIL_RETRIES_EXHAUSTED     9   /* Mixed errors across retries */
#define GATE_FAIL_UART_RECOVERY         10  /* UART re-init failed during recovery */
#define GATE_FAIL_EXCEPTION             11  /* Modbus exception response (non-retryable) */
