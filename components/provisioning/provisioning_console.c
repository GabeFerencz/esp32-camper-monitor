#include "provisioning_console.h"

#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "provisioning_schema.h"
#include "provisioning_store.h"
#include "provisioning_validate.h"

static const char *TAG = "provisioning_console";

static SemaphoreHandle_t s_commit_sem;

static struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} s_wifi_args;

static struct {
    struct arg_str *client_id;
    struct arg_str *client_secret;
    struct arg_end *end;
} s_cf_token_args;

static struct {
    struct arg_str *hostname;
    struct arg_end *end;
} s_host_args;

static struct {
    struct arg_str *webhook_id;
    struct arg_end *end;
} s_webhook_args;

// Prints a value with only its last 4 characters visible, so a captured
// terminal log (screenshot, copy-paste into an issue) can't leak the
// full secret. Non-secret fields use print_plain_field() instead.
static void print_masked_field(const char *label, const char *value)
{
    size_t len = strlen(value);
    if (len == 0) {
        printf("  %-14s (not set)\n", label);
        return;
    }
    size_t visible = (len > 4) ? 4 : len;
    size_t masked = len - visible;
    printf("  %-14s ", label);
    for (size_t i = 0; i < masked; i++) {
        putchar('*');
    }
    printf("%s\n", value + masked);
}

static void print_plain_field(const char *label, const char *value)
{
    printf("  %-14s %s\n", label, (value[0] != '\0') ? value : "(not set)");
}

static int cmd_set_wifi(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&s_wifi_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, s_wifi_args.end, argv[0]);
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_WIFI_SSID, s_wifi_args.ssid->sval[0]);
    if (err == ESP_OK) {
        err = provisioning_store_save_field(PROVISIONING_KEY_WIFI_PASS, s_wifi_args.password->sval[0]);
    }
    if (err != ESP_OK) {
        printf("failed to save WiFi credentials: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("WiFi credentials saved\n");
    return 0;
}

static int cmd_set_cf_token(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&s_cf_token_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, s_cf_token_args.end, argv[0]);
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_CF_ID, s_cf_token_args.client_id->sval[0]);
    if (err == ESP_OK) {
        err = provisioning_store_save_field(PROVISIONING_KEY_CF_SECRET, s_cf_token_args.client_secret->sval[0]);
    }
    if (err != ESP_OK) {
        printf("failed to save Cloudflare Access service token: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Cloudflare Access service token saved\n");
    return 0;
}

static int cmd_set_host(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&s_host_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, s_host_args.end, argv[0]);
        return 1;
    }

    const char *hostname = s_host_args.hostname->sval[0];
    if (!provisioning_host_is_valid(hostname)) {
        printf("invalid hostname -- no URL scheme prefix (http://, https://), and non-empty\n");
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_PHONE_HOST, hostname);
    if (err != ESP_OK) {
        printf("failed to save phone-home host: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("phone-home host saved\n");
    return 0;
}

static int cmd_set_webhook(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&s_webhook_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, s_webhook_args.end, argv[0]);
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_WEBHOOK_ID, s_webhook_args.webhook_id->sval[0]);
    if (err != ESP_OK) {
        printf("failed to save webhook ID: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("webhook ID saved\n");
    return 0;
}

static int cmd_show(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    provisioning_config_t cfg;
    esp_err_t err = provisioning_store_load(&cfg);
    if (err != ESP_OK) {
        printf("failed to read current config from NVS: %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("current provisioning state:\n");
    print_plain_field("wifi_ssid", cfg.wifi_ssid);
    print_masked_field("wifi_pass", cfg.wifi_pass);
    print_plain_field("cf_client_id", cfg.cf_client_id);
    print_masked_field("cf_client_secret", cfg.cf_client_secret);
    print_plain_field("phone_host", cfg.phone_host);
    print_masked_field("webhook_id", cfg.webhook_id);
    printf("  %-14s %s\n", "complete", provisioning_config_is_complete(&cfg) ? "yes" : "no");
    return 0;
}

static int cmd_commit(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    provisioning_config_t cfg;
    esp_err_t err = provisioning_store_load(&cfg);
    if (err != ESP_OK) {
        printf("failed to read current config from NVS: %s\n", esp_err_to_name(err));
        return 1;
    }
    if (!provisioning_config_is_complete(&cfg)) {
        printf("config incomplete -- set-wifi, set-cf-token, set-host, and set-webhook must all be set (see 'show')\n");
        return 1;
    }

    printf("provisioning complete -- rebooting into normal operation\n");
    fflush(stdout);
    xSemaphoreGive(s_commit_sem);
    return 0;
}

static void register_commands(void)
{
    s_wifi_args.ssid = arg_str1(NULL, NULL, "<ssid>", "WiFi network name");
    s_wifi_args.password = arg_str1(NULL, NULL, "<password>", "WiFi password");
    s_wifi_args.end = arg_end(2);
    const esp_console_cmd_t set_wifi_cmd = {
        .command = "set-wifi",
        .help = "Store the WiFi SSID and password",
        .hint = NULL,
        .func = &cmd_set_wifi,
        .argtable = &s_wifi_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_wifi_cmd));

    s_cf_token_args.client_id = arg_str1(NULL, NULL, "<client-id>", "Cloudflare Access service token client ID");
    s_cf_token_args.client_secret = arg_str1(NULL, NULL, "<client-secret>", "Cloudflare Access service token client secret");
    s_cf_token_args.end = arg_end(2);
    const esp_console_cmd_t set_cf_token_cmd = {
        .command = "set-cf-token",
        .help = "Store the Cloudflare Access service token",
        .hint = NULL,
        .func = &cmd_set_cf_token,
        .argtable = &s_cf_token_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_cf_token_cmd));

    s_host_args.hostname = arg_str1(NULL, NULL, "<hostname>", "phone-home hostname, no scheme prefix");
    s_host_args.end = arg_end(1);
    const esp_console_cmd_t set_host_cmd = {
        .command = "set-host",
        .help = "Store the phone-home hostname",
        .hint = NULL,
        .func = &cmd_set_host,
        .argtable = &s_host_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_host_cmd));

    s_webhook_args.webhook_id = arg_str1(NULL, NULL, "<webhook-id>", "Home Assistant webhook ID");
    s_webhook_args.end = arg_end(1);
    const esp_console_cmd_t set_webhook_cmd = {
        .command = "set-webhook",
        .help = "Store the Home Assistant webhook ID",
        .hint = NULL,
        .func = &cmd_set_webhook,
        .argtable = &s_webhook_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_webhook_cmd));

    const esp_console_cmd_t show_cmd = {
        .command = "show",
        .help = "Show currently stored provisioning values (secrets masked)",
        .hint = NULL,
        .func = &cmd_show,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&show_cmd));

    const esp_console_cmd_t commit_cmd = {
        .command = "commit",
        .help = "Validate the stored config and reboot into normal operation",
        .hint = NULL,
        .func = &cmd_commit,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&commit_cmd));
}

void provisioning_console_start(SemaphoreHandle_t commit_sem)
{
    s_commit_sem = commit_sem;

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "provisioning>";

    esp_console_register_help_command();
    register_commands();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    ESP_LOGI(TAG, "provisioning console ready -- type 'help' for commands");
}
