// Entry point for the boot-time provisioning gate. Call once from
// app_main(), after nvs_flash_init(), before starting any other task.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// If NVS already holds a complete config, returns immediately. Otherwise
// blocks in an esp_console provisioning REPL until an operator supplies
// all required fields and runs `commit`, then reboots the device --
// provisioning mode and normal operation never run in the same boot.
void provisioning_run_if_needed(void);

#ifdef __cplusplus
}
#endif
