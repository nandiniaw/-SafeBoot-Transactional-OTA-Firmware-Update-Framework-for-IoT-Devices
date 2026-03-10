/*
 * ota_downloader.c
 *
 *  Created on: 27-Feb-2026
 *      Author: walia
 */

#include "ota_downloader.h"
#include "ota_transaction.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA_DOWNLOAD";

/* Certificate embedded via CMakeLists.txt */
extern const char server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_server_cert_pem_end");

/* -----------------------------------------------------------
   INIT
----------------------------------------------------------- */
esp_err_t ota_downloader_init(void)
{
    ESP_LOGI(TAG, "OTA Downloader ready");
    return ESP_OK;
}

/* -----------------------------------------------------------
   DOWNLOAD AND APPLY FIRMWARE
----------------------------------------------------------- */
esp_err_t ota_download_and_apply(const char *url)
{
    if (url == NULL) {
        ESP_LOGE(TAG, "Invalid URL");
        return ESP_ERR_INVALID_ARG;
    }
    /* Safety reset before starting new OTA session */
    ota_force_idle();

    /* ---------- PARTITION CHECK ---------- */
    const esp_partition_t *update_partition =
        esp_ota_get_next_update_partition(NULL);

    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition available");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Writing to partition: %s", update_partition->label);

    /* ---------- MEMORY CHECK ---------- */
    if (esp_get_free_heap_size() < 30000) {
        ESP_LOGE(TAG, "Insufficient heap for OTA. Aborting.");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Starting OTA from: %s", url);

    /* ---------- MARK TRANSACTION STARTED ---------- */
    esp_err_t ret = ota_set_state(OTA_DOWNLOAD_STARTED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set DOWNLOAD_STARTED state");
        return ret;
    }

    /* ---------- CONFIGURE HTTPS OTA ---------- */
    esp_http_client_config_t http_config = {
        .url             = "https://10.113.208.40/firmware.bin",
        .cert_pem        = server_cert_pem_start,
        .timeout_ms      = 10000,
        .keep_alive_enable = true,
        .max_redirection_count = 5,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    /* ---------- PERFORM OTA DOWNLOAD ---------- */
    ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA download complete");

        /* Update transaction states — check each transition */
        ESP_ERROR_CHECK(ota_set_state(OTA_DOWNLOAD_COMPLETE));
        ESP_ERROR_CHECK(ota_set_state(OTA_VERIFIED));
        ESP_ERROR_CHECK(ota_set_state(OTA_PENDING_BOOT));

        ESP_LOGI(TAG, "Rebooting to apply firmware...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();

    } else {
        ESP_LOGE(TAG, "OTA download failed: %s", esp_err_to_name(ret));
ESP_ERROR_CHECK(ota_set_state(OTA_ROLLBACK));
        return ret;
    }

    return ESP_OK;
}