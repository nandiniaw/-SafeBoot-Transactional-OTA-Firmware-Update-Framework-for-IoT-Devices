/*
 * ota_risk_engine.h
 *
 *  Created on: 27-Feb-2026
 *      Author: walia
 */

#ifndef MAIN_OTA_RISK_ENGINE_H_
#define MAIN_OTA_RISK_ENGINE_H_

#include <stdbool.h>
#include "esp_err.h"

esp_err_t ota_risk_init(void);
void ota_risk_increment_boot(void);
void ota_risk_mark_stable(void);
bool ota_risk_should_force_rollback(void);

/* Crash Telemetry */
void ota_risk_record_crash(void);
uint32_t ota_risk_get_crash_count(void);
void ota_risk_reset_crash_count(void);

#endif /* MAIN_OTA_RISK_ENGINE_H_ */