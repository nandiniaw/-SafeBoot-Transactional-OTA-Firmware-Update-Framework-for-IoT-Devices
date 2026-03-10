#include <inttypes.h>
#include <stdbool.h>
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ota_transaction.h"
#include "ota_risk_engine.h"
#include "ota_version.h"
#include "ota_downloader.h"
#include "wifi_manager.h"

static const char *TAG = "BOOT_VALIDATION";
#define FW_VERSION       "1.0.0"
#define WIFI_SSID        "Momenta"
#define WIFI_PASSWORD    "299792458"
#define OTA_FIRMWARE_URL "https://10.113.208.40:8443/firmware.bin"

/* -----------------------------------------------------------
   BASIC STABI bnLITY CHECK
----------------------------------------------------------- */
static bool stability_check(void)
{
    uint32_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", free_heap);

    if (free_heap < 15000) {
        ESP_LOGE(TAG, "Low memory detected");
        return false;
    }

    return true;
}

/* -----------------------------------------------------------
   OTA TRIGGER CONDITION
----------------------------------------------------------- */
static bool should_start_ota(void)
{
    return true;
}

/* -----------------------------------------------------------
   APPLICATION ENTRY
----------------------------------------------------------- */
void app_main(void)
{
    /* ---------- INIT LAYERS ---------- */
    ESP_ERROR_CHECK(ota_txn_init());
    ESP_ERROR_CHECK(ota_risk_init());
    ESP_ERROR_CHECK(ota_version_init());
    ESP_ERROR_CHECK(ota_downloader_init());

    /* ---------- PERMANENT BOOT STATE CLEANUP ----------
       On every boot, check the current state.
       If it is a terminal state (CONFIRMED/ROLLBACK) or
       any stuck intermediate state, force reset to IDLE.
       This bypasses the transition guard intentionally
       because this is a boot-time system reset, not a
       normal state transition.
    ---------------------------------------------------- */
    ota_txn_state_t boot_state = ota_get_state();
    ESP_LOGI(TAG, "Boot state: %d", boot_state);

    if (boot_state != OTA_PENDING_BOOT) {
        /* Only skip reset if we are pending verification
           of a newly downloaded firmware */
        ota_force_idle();
    }

    /* ---------- CHECK TRANSACTION TIMEOUT ---------- */
    ota_check_timeout();

    ESP_LOGI(TAG, "Current confirmed version: %s",
             ota_version_get_current());

    /* ---------- CONNECT TO WIFI ---------- */
    ESP_ERROR_CHECK(wifi_manager_init(WIFI_SSID, WIFI_PASSWORD));

    /* ---------- CHECK OTA BOOTLOADER STATE ---------- */
    esp_ota_img_states_t ota_state;
    const esp_partition_t *running = esp_ota_get_running_partition();

    ESP_LOGI(TAG, "Running partition: %s", running->label);

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {

        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {

            ESP_LOGW(TAG, "Firmware in PENDING_VERIFY");

            /* ---------- RESET-REASON AWARE BOOT COUNTER ---------- */
            esp_reset_reason_t reason = esp_reset_reason();

            if (reason == ESP_RST_PANIC    ||
                reason == ESP_RST_TASK_WDT ||
                reason == ESP_RST_INT_WDT  ||
                reason == ESP_RST_WDT      ||
                reason == ESP_RST_SW) {

                ESP_LOGW(TAG, "Non-power reset detected.");
                ota_risk_record_crash();
                ota_risk_increment_boot();
            }

            /* ---------- CHECK BOOT LOOP CONDITION ---------- */
            if (ota_risk_should_force_rollback()) {
                ESP_LOGE(TAG, "Exceeded max boot attempts. Forcing rollback.");
                ota_version_store_failed(FW_VERSION);
                ota_set_state(OTA_ROLLBACK);
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }

            /* ---------- TRANSACTIONAL SAFETY CHECK ---------- */
            if (ota_get_state() != OTA_PENDING_BOOT) {
                ESP_LOGE(TAG, "Transactional mismatch. Forcing rollback.");
                ota_version_store_failed(FW_VERSION);
                ota_set_state(OTA_ROLLBACK);
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }

            /* ---------- VERIFICATION WINDOW ---------- */
            ESP_LOGI(TAG, "Verification window started (10 seconds)");

            bool stable = true;

            for (int i = 0; i < 10; i++) {
                vTaskDelay(pdMS_TO_TICKS(1000));

                if (!stability_check()) {
                    stable = false;
                    break;
                }
            }

            /* ---------- DECISION ---------- */
            if (stable) {
                ESP_LOGI(TAG, "Firmware confirmed as stable");
                ota_version_store_confirmed(FW_VERSION);
                ota_set_state(OTA_CONFIRMED);
                esp_ota_mark_app_valid_cancel_rollback();
                ota_risk_mark_stable();
                ota_risk_reset_crash_count();
            } else {
                ESP_LOGE(TAG, "Firmware unstable. Rolling back.");
                ota_version_store_failed(FW_VERSION);
                ota_set_state(OTA_ROLLBACK);
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    /* ---------- NORMAL OPERATION LOOP ---------- */
    while (1) {
        ESP_LOGI(TAG, "System running. TX State: %d | Crashes: %" PRIu32,
                 ota_get_state(),
                 ota_risk_get_crash_count());

        if (should_start_ota()) {
            ota_download_and_apply(OTA_FIRMWARE_URL);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}