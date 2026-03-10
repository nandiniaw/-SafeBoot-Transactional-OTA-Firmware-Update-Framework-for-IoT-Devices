/*
 * ota_version.c
 *
 *  Created on: 27-Feb-2026
 *      Author: walia
 */

#include "ota_version.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "OTA_VERSION";
static nvs_handle_t ver_nvs;
static bool ready = false;

/* -----------------------------------------------------------
   INIT
----------------------------------------------------------- */
esp_err_t ota_version_init(void)
{
    /* Self-sufficient NVS init — safe to call even if already initialized */
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
    ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
}
if (ret != ESP_OK) {
    ESP_LOGW(TAG, "NVS init returned: %s (may already be initialized)",
             esp_err_to_name(ret));
}
    /* ESP_ERR_NVS_ALREADY_INITIALIZED is acceptable — ignore it */

    ret = nvs_open("fw_meta", NVS_READWRITE, &ver_nvs);
    if (ret == ESP_OK) {
        ready = true;
        ESP_LOGI(TAG, "Version store initialized");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to open NVS namespace fw_meta");
    return ret;
}

/* -----------------------------------------------------------
   STORE CONFIRMED VERSION
----------------------------------------------------------- */
void ota_version_store_confirmed(const char *version)
{
    if (!ready || version == NULL) return;

    esp_err_t ret = nvs_set_str(ver_nvs, "last_confirmed", version);
    if (ret == ESP_OK) {
        nvs_commit(ver_nvs);
        ESP_LOGI(TAG, "Confirmed version stored: %s", version);
    } else {
        ESP_LOGE(TAG, "Failed to store confirmed version");
    }
}

/* -----------------------------------------------------------
   STORE FAILED VERSION
----------------------------------------------------------- */
void ota_version_store_failed(const char *version)
{
    if (!ready || version == NULL) return;

    esp_err_t ret = nvs_set_str(ver_nvs, "last_failed", version);
    if (ret == ESP_OK) {
        nvs_commit(ver_nvs);
        ESP_LOGW(TAG, "Failed version stored: %s", version);
    } else {
        ESP_LOGE(TAG, "Failed to store failed version");
    }
}

/* -----------------------------------------------------------
   GET CURRENT CONFIRMED VERSION
----------------------------------------------------------- */
const char* ota_version_get_current(void)
{
    static char version_buf[16];

    if (!ready) return "unknown";

    size_t len = sizeof(version_buf);
    esp_err_t ret = nvs_get_str(ver_nvs, "last_confirmed",
                                version_buf, &len);

    if (ret == ESP_OK)
        return version_buf;

    return "unknown";
}