#include "provisioning_console.h"

#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "provisioning_fingerprint.h"
#include "provisioning_schema.h"
#include "provisioning_store.h"
#include "provisioning_validate.h"

static const char *TAG = "provisioning_console";

static SemaphoreHandle_t s_commit_sem;

// SSID is the only field still taken as a line argument -- see
// read_masked_line() below for why every other field is a masked prompt.
// set-cf-token/set-host/set-webhook take no line arguments at all, so
// they're registered without an argtable (same as show/commit) and just
// reject a stray argc > 1 by hand -- an argtable's arg_end() needs room
// for at least one possible error, so arg_end(0) isn't a safe way to
// express "no arguments".
static struct {
    struct arg_str *ssid;
    struct arg_end *end;
} s_wifi_args;

static void print_plain_field(const char *label, const char *value)
{
    printf("  %-14s %s\n", label, (value[0] != '\0') ? value : "(not set)");
}

// Prints a non-reversible fingerprint instead of any part of the real
// value, so a captured terminal log (screenshot, copy-paste into an
// issue) can't leak the secret -- not even the trailing characters the
// old masked-but-partial display used to show.
static void print_fingerprint_field(const char *label, const char *value)
{
    if (value[0] == '\0') {
        printf("  %-14s (not set)\n", label);
        return;
    }
    char fp[PROVISIONING_FINGERPRINT_LEN + 1];
    provisioning_fingerprint(value, fp);
    printf("  %-14s fp:%s\n", label, fp);
}

// Overwrites a stack buffer that held a secret, so it doesn't sit around
// in memory for the rest of the command's execution. The volatile
// pointer keeps the compiler from optimizing the writes away as dead
// stores to a buffer that's about to go out of scope.
static void wipe_secret(char *buf, size_t len)
{
    volatile char *p = buf;
    while (len--) {
        *p++ = 0;
    }
}

// Reads one line from the console with masked echo: '*' is printed for
// each character typed instead of the character itself, and backspace
// erases both the buffered character and its on-screen '*'. Used for
// every secret-class field. This is why those fields can no longer be
// line arguments -- esp_console's line editor (linenoise) echoes an
// entire typed line character-by-character as part of normal terminal
// feedback, so a secret passed as `<password>` on the command line was
// visible in the raw serial stream the moment it was typed, well before
// `show` was ever involved.
static esp_err_t read_masked_line(const char *prompt, char *out, size_t out_size)
{
    printf("%s", prompt);
    fflush(stdout);

    size_t len = 0;
    for (;;) {
        int c = fgetc(stdin);
        if (c == EOF) {
            return ESP_FAIL;
        }
        if (c == '\r' || c == '\n') {
            break;
        }
        if (c == 0x7f || c == 0x08) { // DEL / backspace
            if (len > 0) {
                len--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
        if (c == 0x03) { // Ctrl-C
            printf("^C\n");
            return ESP_FAIL;
        }
        if (len + 1 >= out_size) {
            printf("\ninput too long (max %d characters) -- aborted, nothing saved\n", (int)out_size - 1);
            while (c != '\r' && c != '\n' && c != EOF) {
                c = fgetc(stdin);
            }
            return ESP_ERR_INVALID_SIZE;
        }
        out[len++] = (char)c;
        putchar('*');
        fflush(stdout);
    }
    out[len] = '\0';
    putchar('\n');
    return ESP_OK;
}

static int cmd_set_wifi(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&s_wifi_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, s_wifi_args.end, argv[0]);
        printf("usage: set-wifi <ssid>  (password is entered at a masked prompt)\n");
        return 1;
    }

    char password[PROVISIONING_PASS_MAX_LEN + 1];
    if (read_masked_line("WiFi password: ", password, sizeof(password)) != ESP_OK) {
        printf("WiFi credentials not saved\n");
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_WIFI_SSID, s_wifi_args.ssid->sval[0]);
    if (err == ESP_OK) {
        err = provisioning_store_save_field(PROVISIONING_KEY_WIFI_PASS, password);
    }
    wipe_secret(password, sizeof(password));
    if (err != ESP_OK) {
        printf("failed to save WiFi credentials: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("WiFi credentials saved\n");
    return 0;
}

static int cmd_set_cf_token(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        printf("usage: set-cf-token  (client ID and secret are entered at masked prompts)\n");
        return 1;
    }

    char client_id[PROVISIONING_CF_ID_MAX_LEN + 1];
    char client_secret[PROVISIONING_CF_SECRET_MAX_LEN + 1];
    esp_err_t err = read_masked_line("Cloudflare Access client ID: ", client_id, sizeof(client_id));
    if (err == ESP_OK) {
        err = read_masked_line("Cloudflare Access client secret: ", client_secret, sizeof(client_secret));
    }
    if (err != ESP_OK) {
        wipe_secret(client_id, sizeof(client_id));
        wipe_secret(client_secret, sizeof(client_secret));
        printf("Cloudflare Access service token not saved\n");
        return 1;
    }

    err = provisioning_store_save_field(PROVISIONING_KEY_CF_ID, client_id);
    if (err == ESP_OK) {
        err = provisioning_store_save_field(PROVISIONING_KEY_CF_SECRET, client_secret);
    }
    wipe_secret(client_id, sizeof(client_id));
    wipe_secret(client_secret, sizeof(client_secret));
    if (err != ESP_OK) {
        printf("failed to save Cloudflare Access service token: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Cloudflare Access service token saved\n");
    return 0;
}

static int cmd_set_host(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        printf("usage: set-host  (hostname is entered at a masked prompt)\n");
        return 1;
    }

    char hostname[PROVISIONING_HOST_MAX_LEN + 1];
    if (read_masked_line("phone-home hostname: ", hostname, sizeof(hostname)) != ESP_OK) {
        printf("phone-home host not saved\n");
        return 1;
    }

    if (!provisioning_host_is_valid(hostname)) {
        printf("invalid hostname -- no URL scheme prefix (http://, https://), and non-empty\n");
        wipe_secret(hostname, sizeof(hostname));
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_PHONE_HOST, hostname);
    wipe_secret(hostname, sizeof(hostname));
    if (err != ESP_OK) {
        printf("failed to save phone-home host: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("phone-home host saved\n");
    return 0;
}

static int cmd_set_webhook(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        printf("usage: set-webhook  (webhook ID is entered at a masked prompt)\n");
        return 1;
    }

    char webhook_id[PROVISIONING_WEBHOOK_ID_MAX_LEN + 1];
    if (read_masked_line("Home Assistant webhook ID: ", webhook_id, sizeof(webhook_id)) != ESP_OK) {
        printf("webhook ID not saved\n");
        return 1;
    }

    esp_err_t err = provisioning_store_save_field(PROVISIONING_KEY_WEBHOOK_ID, webhook_id);
    wipe_secret(webhook_id, sizeof(webhook_id));
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
    print_fingerprint_field("wifi_pass", cfg.wifi_pass);
    print_fingerprint_field("cf_client_id", cfg.cf_client_id);
    print_fingerprint_field("cf_client_secret", cfg.cf_client_secret);
    print_fingerprint_field("phone_host", cfg.phone_host);
    print_fingerprint_field("webhook_id", cfg.webhook_id);
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
    s_wifi_args.end = arg_end(1);
    const esp_console_cmd_t set_wifi_cmd = {
        .command = "set-wifi",
        .help = "Store the WiFi SSID; password is entered at a masked prompt that follows",
        .hint = "<ssid>",
        .func = &cmd_set_wifi,
        .argtable = &s_wifi_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_wifi_cmd));

    const esp_console_cmd_t set_cf_token_cmd = {
        .command = "set-cf-token",
        .help = "Store the Cloudflare Access service token; client ID and secret are entered at masked prompts that follow",
        .hint = NULL,
        .func = &cmd_set_cf_token,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_cf_token_cmd));

    const esp_console_cmd_t set_host_cmd = {
        .command = "set-host",
        .help = "Store the phone-home hostname, entered at a masked prompt that follows",
        .hint = NULL,
        .func = &cmd_set_host,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_host_cmd));

    const esp_console_cmd_t set_webhook_cmd = {
        .command = "set-webhook",
        .help = "Store the Home Assistant webhook ID, entered at a masked prompt that follows",
        .hint = NULL,
        .func = &cmd_set_webhook,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_webhook_cmd));

    const esp_console_cmd_t show_cmd = {
        .command = "show",
        .help = "Show currently stored provisioning values (secrets shown as a fingerprint, never plaintext)",
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
