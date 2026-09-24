#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef DEVICE_HAS_SD
#define DEVICE_HAS_SD 0
#endif

#if DEVICE_HAS_SD

typedef struct {
    bool ok;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t write_count;
    uint32_t fail_count;
    char last_path[64];
} board_sd_status_t;

bool board_sd_init(void);
bool board_sd_write_file(const char *path, const uint8_t *data, size_t len);
void board_sd_get_status(board_sd_status_t *out);

#else

typedef struct {
    bool ok;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t write_count;
    uint32_t fail_count;
    char last_path[64];
} board_sd_status_t;

static inline bool board_sd_init(void)
{
    return false;
}
static inline bool board_sd_write_file(const char *path, const uint8_t *data, size_t len)
{
    (void)path;
    (void)data;
    (void)len;
    return false;
}
static inline void board_sd_get_status(board_sd_status_t *out)
{
    if (out) {
        *out = (board_sd_status_t){0};
    }
}

#endif
