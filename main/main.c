#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASS "YOUR_PASS"
#define METADATA_URL "https://example.com/metadata.json"

static const char *TAG = "OTA";

bool is_version_newer(const char *current, const char *remote) {
    int curMaj, curMin, curPatch;
    int remMaj, remMin, remPatch;
    sscanf(current, "%d.%d.%d", &curMaj, &curMin, &curPatch);
    sscanf(remote, "%d.%d.%d", &remMaj, &remMin, &remPatch);
    if (remMaj > curMaj) return true;
    if (remMaj == curMaj && remMin > curMin) return true;
    if (remMaj == curMaj && remMin == curMin && remPatch > curPatch) return true;
    return false;
}

bool fetch_update_metadata(char *url_out, size_t url_size) {
    esp_http_client_config_t cfg = {
        .url = METADATA_URL,
        .cert_pem = NULL,
        .skip_cert_common_name_check = true
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (esp_http_client_open(client, 0) != ESP_OK) return false;

    char buf[512];
    int len = esp_http_client_read(client, buf, sizeof(buf) - 1);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (len <= 0) return false;

    buf[len] = '\0';
    cJSON *json = cJSON_Parse(buf);
    if (!json) return false;

    cJSON *ver = cJSON_GetObjectItem(json, "version");
    cJSON *url = cJSON_GetObjectItem(json, "url");
    if (!cJSON_IsString(ver) || !cJSON_IsString(url)) {
        cJSON_Delete(json);
        return false;
    }

    const char *current = esp_app_get_description()->version;
    ESP_LOGI(TAG, "Current FW: %s | Server FW: %s", current, ver->valuestring);

    if (is_version_newer(current, ver->valuestring)) {
        strncpy(url_out, url->valuestring, url_size);
        cJSON_Delete(json);
        return true;
    }

    cJSON_Delete(json);
    return false;
}

void ota_check_and_update() {
    char firmware_url[256];

    if (fetch_update_metadata(firmware_url, sizeof(firmware_url))) {
        ESP_LOGI(TAG, "New version found → downloading: %s", firmware_url);

        esp_http_client_config_t config = {
            .url = firmware_url,
            .cert_pem = NULL,
            .skip_cert_common_name_check = true
        };

        esp_https_ota_config_t ota_config = {
            .http_config = &config
        };

        esp_err_t ret = esp_https_ota(&ota_config);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "OTA Success! Rebooting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Failed!");
        }
    } else {
        ESP_LOGI(TAG, "Already up to date ✅");
    }
}


void wifi_init() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t wifi_config = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS }
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void app_main() {
    nvs_flash_init();
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(5000));
    ota_check_and_update();
}
