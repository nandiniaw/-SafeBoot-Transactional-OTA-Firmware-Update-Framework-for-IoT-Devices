/*
 * ota_transaction.h
 *
 *  Created on: 24-Feb-2026
 *      Author: walia
 */

#ifndef MAIN_OTA_TRANSACTION_H_
#define MAIN_OTA_TRANSACTION_H_


#include "esp_err.h"

typedef enum {
    OTA_IDLE = 0,
    OTA_DOWNLOAD_STARTED,
    OTA_DOWNLOAD_COMPLETE,
    OTA_VERIFIED,
    OTA_PENDING_BOOT,
    OTA_CONFIRMED,
    OTA_ROLLBACK
} ota_txn_state_t;

esp_err_t ota_txn_init(void);
ota_txn_state_t ota_get_state(void);
esp_err_t ota_set_state(ota_txn_state_t new_state);
void ota_check_timeout(void);
void ota_force_idle(void); 

#endif /* MAIN_OTA_TRANSACTION_H_ */
