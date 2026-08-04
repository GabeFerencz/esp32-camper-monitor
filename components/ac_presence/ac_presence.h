// Glue between the AC-presence GPIO and the debounce state machine:
// owns a dedicated FreeRTOS task that samples the pin, feeds it through
// ac_presence_debounce_process(), and logs confirmed transitions.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Initializes the sensing GPIO and starts the sampling task. Call once
// from app_main().
void ac_presence_start(void);

#ifdef __cplusplus
}
#endif
