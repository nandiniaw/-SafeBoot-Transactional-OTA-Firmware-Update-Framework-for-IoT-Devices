/*
 * ota_downloader.h
 *
 *  Created on: 27-Feb-2026
 *      Author: walia
 */

#ifndef MAIN_OTA_DOWNLOADER_H_
#define MAIN_OTA_DOWNLOADER_H_

#include "esp_err.h"

esp_err_t ota_downloader_init(void);
esp_err_t ota_download_and_apply(const char *url);

#endif /* MAIN_OTA_DOWNLOADER_H_ */