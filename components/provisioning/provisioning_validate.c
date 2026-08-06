#include "provisioning_validate.h"

#include <stddef.h>
#include <string.h>

static bool is_nonempty(const char *s)
{
    return s != NULL && s[0] != '\0';
}

bool provisioning_host_is_valid(const char *host)
{
    if (!is_nonempty(host)) {
        return false;
    }
    return strncmp(host, "http://", 7) != 0 && strncmp(host, "https://", 8) != 0;
}

bool provisioning_config_is_complete(const provisioning_config_t *cfg)
{
    return cfg != NULL
        && is_nonempty(cfg->wifi_ssid)
        && is_nonempty(cfg->wifi_pass)
        && is_nonempty(cfg->cf_client_id)
        && is_nonempty(cfg->cf_client_secret)
        && provisioning_host_is_valid(cfg->phone_host)
        && is_nonempty(cfg->webhook_id);
}
