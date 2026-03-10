/*
 * ota_version.h
 *
 *  Created on: 27-Feb-2026
 *      Author: walia
 */

#ifndef MAIN_OTA_VERSION_H_
#define MAIN_OTA_VERSION_H_

#include <stdbool.h>
#include "esp_err.h"

esp_err_t ota_version_init(void);
void ota_version_store_confirmed(const char *version);
void ota_version_store_failed(const char *version);
const char* ota_version_get_current(void);

#endif /* MAIN_OTA_VERSION_H_ */