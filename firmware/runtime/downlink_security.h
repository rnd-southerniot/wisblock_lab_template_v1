#pragma once

#include <stdint.h>

class SystemManager;

#define SDL_FRAME_MAGIC        0xD1
#define SDL_PROTOCOL_VERSION   0x02
#define SDL_HMAC_KEY_LEN       32
#define SDL_HMAC_TRUNC_LEN     4
#define SDL_MIN_FRAME_LEN      12
#define SDL_MAX_FRAME_LEN      60
#define SDL_NONCE_STORAGE_KEY  "sdl_nonce"

#define SDL_CMD_SET_INTERVAL   0x01
#define SDL_CMD_SET_SLAVE_ADDR 0x02
#define SDL_CMD_REQUEST_STATUS 0x03
#define SDL_CMD_REBOOT         0x04

struct SecureDlResult {
    bool valid;
    uint8_t cmd_id;
    bool auth_fail;
    bool replay_fail;
    bool version_fail;
    char msg[96];
};

SecureDlResult secure_dl_validate_and_apply(const uint8_t* frame,
                                            uint8_t len,
                                            SystemManager& sys,
                                            const uint8_t key[SDL_HMAC_KEY_LEN]);
