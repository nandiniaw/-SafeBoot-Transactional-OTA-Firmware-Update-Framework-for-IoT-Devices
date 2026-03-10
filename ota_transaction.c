/*
 * ota_transaction.c
 *
 *  Created on: 24-Feb-2026
 *      Author: walia
 */

#include "ota_transaction.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"

static const char *TAG = "OTA_TXN";
#define TIMEOUT_SECONDS 15

static nvs_handle_t ota_nvs;
static bool nvs_ready = false;

/* -----------------------------------------------------------
   VALID TRANSITION CHECK
----------------------------------------------------------- */
static bool is_valid_transition(ota_txn_state_t current, ota_txn_state_t next)
{
    if (next == current + 1)
        return true;

    if ((current == OTA_CONFIRMED || current == OTA_ROLLBACK)
        && next == OTA_IDLE)
        return true;

    if (next == OTA_ROLLBACK)
        return true;

    return false;
}

/* -----------------------------------------------------------
   INIT
----------------------------------------------------------- */
esp_err_t ota_txn_init(void)
{
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

    ret = nvs_open("ota_store", NVS_READWRITE, &ota_nvs);
    if (ret == ESP_OK)
        nvs_ready = true;

    return ret;
}

/* -----------------------------------------------------------
   GET STATE
----------------------------------------------------------- */
ota_txn_state_t ota_get_state(void)
{
    if (!nvs_ready)
        return OTA_IDLE;

    int32_t state = OTA_IDLE;
    esp_err_t err = nvs_get_i32(ota_nvs, "ota_state", &state);

    if (err != ESP_OK)
        return OTA_IDLE;

    return (ota_txn_state_t)state;
}

/* -----------------------------------------------------------
   SET STATE
----------------------------------------------------------- */
esp_err_t ota_set_state(ota_txn_state_t new_state)
{
    if (!nvs_ready)
        return ESP_FAIL;

    ota_txn_state_t current = ota_get_state();

    if (!is_valid_transition(current, new_state)) {
        ESP_LOGE(TAG, "Invalid transition %d -> %d", current, new_state);
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(nvs_set_i32(ota_nvs, "ota_state", new_state));

    int64_t now = esp_timer_get_time() / 1000000;
    ESP_ERROR_CHECK(nvs_set_i64(ota_nvs, "timestamp", now));

    ESP_ERROR_CHECK(nvs_commit(ota_nvs));

    ESP_LOGI(TAG, "State updated -> %d", new_state);

    return ESP_OK;
}

/* -----------------------------------------------------------
   TIMEOUT CHECK
----------------------------------------------------------- */
void ota_check_timeout(void)
{
    if (!nvs_ready)
        return;

    int64_t stored_time = 0;

    if (nvs_get_i64(ota_nvs, "timestamp", &stored_time) != ESP_OK)
        return;

    int64_t now = esp_timer_get_time() / 1000000;

    if (stored_time != 0 && (now - stored_time > TIMEOUT_SECONDS)) {

        ota_txn_state_t current = ota_get_state();

        if (current != OTA_IDLE &&
            current != OTA_CONFIRMED &&
            current != OTA_ROLLBACK) {

            ESP_LOGW(TAG, "Transaction timeout. Resetting to IDLE.");
            nvs_set_i32(ota_nvs, "ota_state", OTA_IDLE);
            nvs_commit(ota_nvs);
        }
    }
}
void ota_force_idle(void)
{
    if (!nvs_ready) return;

    int32_t current = OTA_IDLE;
    nvs_get_i32(ota_nvs, "ota_state", &current);

    ESP_LOGW("OTA_TXN", "Force resetting state from %d to IDLE", current);

    nvs_set_i32(ota_nvs, "ota_state", OTA_IDLE);
    int64_t now = esp_timer_get_time() / 1000000;
    nvs_set_i64(ota_nvs, "timestamp", now);
    nvs_commit(ota_nvs);
}