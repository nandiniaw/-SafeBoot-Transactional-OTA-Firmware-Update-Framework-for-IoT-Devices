# -SafeBoot-Transactional-OTA-Firmware-Update-Framework-for-IoT-Devices
A fail-safe OTA (Over-the-Air) firmware update system for low-cost IoT devices  built on ESP32 using Espressif IDF. SafeBoot protects devices against corrupted  firmware flashes and power failures during updates through a multi-layered  protection architecture.

## Overview
Standard OTA update mechanisms risk bricking a device if power is lost or 
firmware is corrupted mid-update. SafeBoot solves this by introducing a 
transactional, checkpoint-based update pipeline with automatic rollback 
capability — ensuring the device always boots into a known stable state.

## Architecture
```
OTA Update Triggered
        ↓
Risk Assessment Engine
        ↓
Transactional Checkpoint Pipeline (4-5 stages)
        ↓
Dual Partition Bootloader
        ↓
Health Check Engine
        ↓
✅ Commit New Firmware  OR  🔄 Automatic Rollback
```

## Components

### ✅ Dual Partition Bootloader
- Two firmware partitions — active and passive
- On every update, new firmware is written to the passive partition
- Bootloader switches to new partition only after successful validation
- On failure, automatically boots back from the active (stable) partition

### ✅ Transactional Checkpoint System
- Firmware update divided into 4-5 discrete stages
- Progress saved at each checkpoint to non-volatile storage
- On power loss or interruption, update resumes or rolls back 
  from the last valid checkpoint
- Prevents partial firmware writes from corrupting the device

### ✅ Automatic Rollback
- Triggered automatically on any of the following:
  - Incomplete firmware write detected
  - Firmware integrity check failure
  - Health check failure post-update
- Rolls back to last stable firmware without any manual intervention

### ✅ Risk Assessment Engine
- Validates firmware before committing the update:
  - Firmware version compatibility check
  - Binary integrity verification
  - Size and partition boundary validation
- Rejects and discards invalid firmware before it can cause damage

### 🔄 Health Check Engine *(in progress)*
- Post-update device vitals monitoring
- Flags abnormal behaviour after boot into new firmware
- Triggers rollback if stability thresholds are not met

### 🔄 Web Logging Dashboard *(in progress)*
- Real-time visibility into update status and rollback events
- Health metrics monitoring across connected devices

## Repository Structure
```
SafeBoot/
├── hello_world_main.c      # Entry point and main OTA update flow
├── ota_downloader.c        # OTA firmware download handler
├── ota_downloader.h
├── ota_risk_engine.c       # Risk assessment engine
├── ota_risk_engine.h
├── ota_transaction.c       # Transactional checkpoint system
├── ota_transaction.h
├── ota_version.c           # Version validation and compatibility check
├── ota_version.h
├── wifi_manager.c          # Wi-Fi connection management
├── wifi_manager.h
├── server_cert.pem         # SSL certificate for secure HTTPS OTA
├── CMakeLists.txt          # ESP-IDF build configuration
└── README.md
```

## Tech Stack
- **Hardware:** ESP32
- **Framework:** Espressif IDF (ESP-IDF)
- **Language:** C
- **OTA Protocol:** HTTPS over Wi-Fi
- **Storage:** NVS (Non-Volatile Storage) for checkpoint data

## How to Run

### Prerequisites
- ESP-IDF installed and configured
- ESP32 development board
- Wi-Fi network for OTA delivery

### Steps
1. Clone the repository
```bash
git clone https://github.com/yourusername/safeboot-ota-esp32.git
cd safeboot-ota-esp32
```

2. Configure your Wi-Fi and OTA server details
```bash
idf.py menuconfig
```

3. Flash the initial firmware
```bash
idf.py build flash monitor
```

4. Trigger OTA update from your server and observe checkpoint 
   and rollback behaviour in the serial monitor

## Project Status
✅ Dual partition bootloader — Complete  
✅ Transactional checkpoint system — Complete  
✅ Automatic rollback — Complete  
✅ Risk assessment engine — Complete  
🔄 Health check engine — In Progress  
🔄 Web logging dashboard — In Progress  

## Author
**Nandini Walia**
B.Tech EEE, Vellore Institute of Technology
[linkedin.com/in/nandiniwalia](https://linkedin.com/in/nandiniwalia)
