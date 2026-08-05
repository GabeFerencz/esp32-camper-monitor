// esp_console-based provisioning REPL: lets an operator type WiFi
// credentials, the Cloudflare Access service token, and the phone-home
// hostname in over the same USB/UART connection idf.py monitor already
// uses. See provisioning.c for how this is driven from app_main.
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Registers the provisioning commands (set-wifi, set-cf-token, set-host,
// show, commit) and starts the console REPL. esp_console_start_repl()
// runs the REPL on its own task, so this call returns immediately;
// commit_sem is given once an operator runs `commit` against a complete,
// valid config -- the caller blocks on it to know when to reboot.
void provisioning_console_start(SemaphoreHandle_t commit_sem);

#ifdef __cplusplus
}
#endif
