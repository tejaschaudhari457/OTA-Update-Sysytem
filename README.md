OTA Firmware Update for ESP32

This project demonstrates different OTA (Over-The-Air) firmware update techniques for ESP32-based devices, including firmware upgrades, RTOS-based update handling, version management, and rollback recovery mechanisms.

Files
RTOS_version.cpp – OTA implementation using FreeRTOS task management.
rollback-ota.cpp – OTA update with rollback support if an update fails.
dummy_code.cpp – Sample code for testing and simulation.
block diagram – System architecture and OTA workflow.
Objective
Implement OTA firmware updates on ESP32.
Manage firmware versions.
Support rollback and recovery.
Simulate and test update processes.
Explore RTOS-based OTA mechanisms.
OTA Flow
Check for new firmware.
Download firmware image.
Verify firmware integrity.
Install firmware.
Reboot into the new version.
Roll back if the update fails.
Technologies
ESP32
C++
FreeRTOS
OTA Update Techniques
Embedded Systems
Author

Tejas Chaudhari
 GitHub: https://github.com/tejaschaudhari457
