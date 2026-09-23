#pragma once

#include <stdbool.h>

typedef enum {
    BOOTSTRAP_STATE_WIFI_NEEDED = 0,
    BOOTSTRAP_STATE_PROVISION_AWS,
    BOOTSTRAP_STATE_READY,
    BOOTSTRAP_STATE_ERROR,
} bootstrap_state_t;

void bootstrap_init(void);
bootstrap_state_t bootstrap_state(void);

// Returns true when the device should run the normal application loop.
bool bootstrap_is_ready(void);

// Advance the state machine. Call from the main loop.
void bootstrap_poll(void);
