/*
 * ota_risk_engine.c
 *
 *  Created on: 27-Feb-2026
 *      Author: walia
 */

#include "ota_risk_engine.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "OTA_RISK";
static nvs_handle_t risk_nvs;
static bool ready = false;

#define MAX_BOOT_ATTEMPTS 3
#define MAX_CRASH_COUNT   5

/* -----------------------------------------------------------
   INIT
----------------------------------------------------------- */
esp_err_t ota_risk_init(void)
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

    ret = nvs_open("ota_risk", NVS_READWRITE, &risk_nvs);
    if (ret == ESP_OK) {
        ready = true;
        ESP_LOGI(TAG, "Risk engine initialized");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to open NVS namespace ota_risk");
    return ESP_FAIL;
}

/* -----------------------------------------------------------
   BOOT COUNTER
----------------------------------------------------------- */
void ota_risk_increment_boot(void)
{
    if (!ready) return;

    int32_t boot_count = 0;
    nvs_get_i32(risk_nvs, "boot_cnt", &boot_count);
    boot_count++;
    nvs_set_i32(risk_nvs, "boot_cnt", boot_count);
    nvs_commit(risk_nvs);

    ESP_LOGW(TAG, "Boot attempt: %d", boot_count);
}

void ota_risk_mark_stable(void)
{
    if (!ready) return;

    nvs_set_i32(risk_nvs, "boot_cnt", 0);
    nvs_commit(risk_nvs);

    ESP_LOGI(TAG, "System marked stable. Boot counter reset.");
}

bool ota_risk_should_force_rollback(void)
{
    if (!ready) return false;

    int32_t boot_count = 0;
    nvs_get_i32(risk_nvs, "boot_cnt", &boot_count);

    return (boot_count >= MAX_BOOT_ATTEMPTS);
}

/* -----------------------------------------------------------
   CRASH TELEMETRY
----------------------------------------------------------- */
void ota_risk_record_crash(void)
{
    if (!ready) return;

    int32_t crash_count = 0;
    nvs_get_i32(risk_nvs, "crash_cnt", &crash_count);
    crash_count++;
    nvs_set_i32(risk_nvs, "crash_cnt", crash_count);
    nvs_commit(risk_nvs);

    ESP_LOGW(TAG, "Crash recorded. Total crashes: %d", crash_count);
}

uint32_t ota_risk_get_crash_count(void)
{
    if (!ready) return 0;

    int32_t crash_count = 0;
    nvs_get_i32(risk_nvs, "crash_cnt", &crash_count);

    return (uint32_t)crash_count;
}

void ota_risk_reset_crash_count(void)
{
    if (!ready) return;

    nvs_set_i32(risk_nvs, "crash_cnt", 0);
    nvs_commit(risk_nvs);

    ESP_LOGI(TAG, "Crash counter reset.");
}