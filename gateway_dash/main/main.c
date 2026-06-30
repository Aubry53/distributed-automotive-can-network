#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "can_protocol.h"

#define CAN_TX_IO      22
#define CAN_RX_IO      21

#define WIFI_SSID      "ESP32_Automotive_Network"
#define WIFI_PASS      "12345678"


static const char *TAG = "GATEWAY_NET";
// AJOUTEZ CETTE LIGNE ICI :
static httpd_handle_t server = NULL; 



// Variables globales partagées pour stocker les dernières valeurs reçues du bus CAN
static uint16_t global_rpm = 0;
static int8_t   global_temp = 0;
static uint8_t  global_fuel = 0;

// Page Web Dashboard avec script Fetch synchrone à haute fréquence (Toutes les 200ms)
const char* html_page = 
"<!DOCTYPE html><html><head><meta charset='utf-8'><title>CAN Dashboard</title>"
"<style>body{font-family:Arial;background:#111;color:#fff;text-align:center;padding-top:30px;}"
".card{background:#222;padding:20px;margin:15px auto;width:300px;border-radius:10px;border:1px solid #333;}"
".val{font-size:32px;color:#00ffcc;font-weight:bold;}</style></head>"
"<body><h1>Automotive CAN Gateway</h1>"
"<div class='card'><h2>Régime Moteur</h2><div id='rpm' class='val'>0</div> tr/min</div>"
"<div class='card'><h2>Température</h2><div id='temp' class='val'>0</div> °C</div>"
"<div class='card'><h2>Carburant</h2><div id='fuel' class='val'>0</div> %</div>"
"<script>"
"setInterval(function(){"
"  fetch('/data').then(response => response.json()).then(d => {"
"    document.getElementById('rpm').innerHTML = d.rpm;"
"    document.getElementById('temp').innerHTML = d.temp;"
"    document.getElementById('fuel').innerHTML = d.fuel;"
"  }).catch(err => console.error(err));"
"}, 200);" // Rafraîchissement automatique à 5Hz pour coller au rythme du bus CAN
"</script></body></html>";

// Handler pour envoyer la page HTML
esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler pour envoyer le flux JSON des données CAN
esp_err_t data_handler(httpd_req_t *req) {
    char json_buffer[128];
    snprintf(json_buffer, sizeof(json_buffer), "{\"rpm\":%u,\"temp\":%d,\"fuel\":%u}", 
             global_rpm, global_temp, global_fuel);
             
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Tâche de réception CAN (Met à jour les variables globales en continu)
void can_rx_task_sync(void *pvParameters) {
    twai_message_t rx_msg;

    while (1) {
        if (twai_receive(&rx_msg, portMAX_DELAY) == ESP_OK) {
            if (rx_msg.identifier == CAN_ID_ENGINE_DATA) {
                can_engine_data_t data;
                memcpy(&data, rx_msg.data, sizeof(can_engine_data_t));
                
                // Stockage immédiat des valeurs dans notre zone tampon sécurisée
                global_rpm = data.rpm;
                global_temp = data.temperature;
                global_fuel = data.fuel_level;
                
                ESP_LOGI(TAG, "[CAN ➔ RAM] Updated: RPM=%u Temp=%d Fuel=%u", global_rpm, global_temp, global_fuel);
            }
        }
    }
}

void start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 5;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Route 1 : Page d'accueil HTML
        httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = get_handler };
        httpd_register_uri_handler(server, &uri_get);

        // Route 2 : Endpoint API JSON
        httpd_uri_t uri_data = { .uri = "/data", .method = HTTP_GET, .handler = data_handler };
        httpd_register_uri_handler(server, &uri_data);
        
        ESP_LOGI(TAG, "Serveur Web de streaming JSON initialisé.");
    }
}

void init_wifi(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    
    esp_netif_ip_info_t ip_info;
    esp_netif_str_to_ip4("11.22.33.44", &ip_info.ip);
    esp_netif_str_to_ip4("11.22.33.44", &ip_info.gw);
    esp_netif_str_to_ip4("255.255.255.0", &ip_info.netmask);
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi Hotspot actif. SSID: %s", WIFI_SSID);
}

void init_can(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_IO, CAN_RX_IO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_wifi();
    start_web_server();
    init_can();

    xTaskCreate(can_rx_task_sync, "CanRxTaskSync", 4096, NULL, 6, NULL);
}

