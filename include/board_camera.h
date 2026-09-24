#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef DEVICE_HAS_CAMERA
#define DEVICE_HAS_CAMERA 0
#endif

#if DEVICE_HAS_CAMERA

typedef struct {
    bool ok;
    uint32_t last_frame_bytes;
    uint32_t capture_count;
    uint32_t fail_count;
    char sensor[24];
} board_camera_status_t;

bool board_camera_init(void);
bool board_camera_capture_jpeg(uint8_t **out_buf, size_t *out_len);
void board_camera_release_frame(void);
void board_camera_get_status(board_camera_status_t *out);

#else

typedef struct {
    bool ok;
    uint32_t last_frame_bytes;
    uint32_t capture_count;
    uint32_t fail_count;
    char sensor[24];
} board_camera_status_t;

static inline bool board_camera_init(void)
{
    return false;
}
static inline bool board_camera_capture_jpeg(uint8_t **out_buf, size_t *out_len)
{
    (void)out_buf;
    (void)out_len;
    return false;
}
static inline void board_camera_release_frame(void) {}
static inline void board_camera_get_status(board_camera_status_t *out)
{
    if (out) {
        *out = (board_camera_status_t){0};
    }
}

#endif
