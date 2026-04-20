#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_err.h"

#include "driver/gpio.h"

static const char *TAG = "motor_server";

/* -------- Wi-Fi AP settings -------- */
#define WIFI_SSID      "RC_Car"
#define WIFI_PASS      "12345678"
#define WIFI_CHANNEL   1
#define WIFI_MAX_CONN  4

#define M1_IN1 14
#define M1_IN2 27
#define M2_IN1 26
#define M2_IN2 25
#define M3_IN1 12
#define M3_IN2 13
#define M4_IN1 2
#define M4_IN2 15

static void motor_stop(void);
static void motor_forward(void);
static void motor_reverse(void);
static void motor_left(void);
static void motor_right(void);

static esp_err_t root_handler(httpd_req_t *req);
static esp_err_t control_handler(httpd_req_t *req);
static httpd_handle_t start_webserver(void);
static void wifi_init_ap(void);

static void motor_stop(void)
{
    gpio_set_level(M1_IN1, 0); gpio_set_level(M1_IN2, 0);
    gpio_set_level(M2_IN1, 0); gpio_set_level(M2_IN2, 0);
    gpio_set_level(M3_IN1, 0); gpio_set_level(M3_IN2, 0);
    gpio_set_level(M4_IN1, 0); gpio_set_level(M4_IN2, 0);
}

static void motor_forward(void)
{
    motor_stop();
    gpio_set_level(M1_IN1, 1); gpio_set_level(M1_IN2, 0);
    gpio_set_level(M2_IN1, 1); gpio_set_level(M2_IN2, 0);
    gpio_set_level(M3_IN1, 1); gpio_set_level(M3_IN2, 0);
    gpio_set_level(M4_IN1, 1); gpio_set_level(M4_IN2, 0);
}

static void motor_reverse(void)
{
    motor_stop();
    gpio_set_level(M1_IN1, 0); gpio_set_level(M1_IN2, 1);
    gpio_set_level(M2_IN1, 0); gpio_set_level(M2_IN2, 1);
    gpio_set_level(M3_IN1, 0); gpio_set_level(M3_IN2, 1);
    gpio_set_level(M4_IN1, 0); gpio_set_level(M4_IN2, 1);
}

static void motor_left(void)
{
    motor_stop();
    /* Left side reverse, right side forward */
    gpio_set_level(M1_IN1, 0); gpio_set_level(M1_IN2, 1);
    gpio_set_level(M2_IN1, 1); gpio_set_level(M2_IN2, 0);

    gpio_set_level(M3_IN1, 0); gpio_set_level(M3_IN2, 1);
    gpio_set_level(M4_IN1, 1); gpio_set_level(M4_IN2, 0);
}

static void motor_right(void)
{
    motor_stop();
    /* Right side reverse, left side forward */
    gpio_set_level(M1_IN1, 1); gpio_set_level(M1_IN2, 0);
    gpio_set_level(M2_IN1, 0); gpio_set_level(M2_IN2, 1);

    gpio_set_level(M3_IN1, 1); gpio_set_level(M3_IN2, 0);
    gpio_set_level(M4_IN1, 0); gpio_set_level(M4_IN2, 1);
}

static void motor_dance(void)
{
    for (int i = 0; i < 3; i++) {
        motor_left();
        vTaskDelay(pdMS_TO_TICKS(200));
        motor_right();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    motor_stop();
}

static esp_err_t control_handler(httpd_req_t *req)
{
    char query[64] = {0};
    char cmd[16] = {0};

    int qlen = httpd_req_get_url_query_len(req);
    if (qlen > 0 && qlen < (int)sizeof(query)) {
        if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
            if (httpd_query_key_value(query, "cmd", cmd, sizeof(cmd)) == ESP_OK) {
                if (strcmp(cmd, "forward") == 0) {
                    motor_forward();
                } else if (strcmp(cmd, "reverse") == 0) {
                    motor_reverse();
                } else if (strcmp(cmd, "left") == 0) {
                    motor_left();
                } else if (strcmp(cmd, "right") == 0) {
                    motor_right();
                } else if (strcmp(cmd, "dance") == 0) {
                    motor_dance();
                }else {
                    motor_stop();
                }
            }
        }
    } else {
        motor_stop();
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t root_handler(httpd_req_t *req)
{
    const char *html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>"
        "<title>RC Car</title>"
        "<style>"
        "body{font-family:Segoe UI,sans-serif;background:#1a1a1a;color:#fff;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;margin:0;user-select:none;}"
        "h1{margin-bottom:20px;font-weight:300;letter-spacing:2px;color:#00d4ff;}"
        ".d-pad{display:grid;grid-template-columns:repeat(3, 80px);grid-template-rows:repeat(3, 80px);gap:15px;}"
        "button{width:80px;height:80px;border:none;border-radius:12px;background:#333;color:white;font-size:24px;transition:0.1s;touch-action:manipulation;box-shadow:0 4px #000;}"
        "button:active{background:#00d4ff;transform:translateY(4px);box-shadow:0 0 #000;}"
        ".up{grid-column:2;}.left{grid-column:1;}.stop{grid-column:2;background:#e74c3c;}.right{grid-column:3;}.down{grid-column:2;}"
        ".dance-container{margin-top:30px;}"
        ".dance-btn{width:180px;height:60px;background:#bc0ff1;color:#000;font-size:18px;font-weight:bold;box-shadow:0 4px #780a9a;}"
        ".dance-btn:active{background:#f39c12;box-shadow:0 0 #000;}"
        "</style>"
        "</head>"
        "<body>"
        "<h1>RC Car</h1>"
        "<div class='d-pad'>"
        "<button class='up' onclick=\"fetch('/control?cmd=forward')\">▲</button>"
        "<button class='left' onclick=\"fetch('/control?cmd=left')\">◀</button>"
        "<button class='stop' onclick=\"fetch('/control?cmd=stop')\">■</button>"
        "<button class='right' onclick=\"fetch('/control?cmd=right')\">▶</button>"
        "<button class='down' onclick=\"fetch('/control?cmd=reverse')\">▼</button>"
        "</div>"
        "<div class='dance-container'>"
        "<button class='dance-btn' onclick=\"fetch('/control?cmd=dance')\">🕺 DANCE</button>"
        "</div>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 8;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root);

    httpd_uri_t control = {
        .uri = "/control",
        .method = HTTP_GET,
        .handler = control_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &control);

    ESP_LOGI(TAG, "Web server started");
    return server;
}

static void wifi_init_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = WIFI_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .beacon_interval = 100,
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP started");
    ESP_LOGI(TAG, "Connect to SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "Open: http://192.168.4.1/");
}

static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask =
            (1ULL << M1_IN1) | (1ULL << M1_IN2) |
            (1ULL << M2_IN1) | (1ULL << M2_IN2) |
            (1ULL << M3_IN1) | (1ULL << M3_IN2) |
            (1ULL << M4_IN1) | (1ULL << M4_IN2),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    motor_stop();
    ESP_LOGI(TAG, "GPIO initialized");
}

static void nvs_init(void)
{   
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting app...");
    nvs_init();
    gpio_init();
    wifi_init_ap();
    start_webserver();

    ESP_LOGI(TAG, "Server ready");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
