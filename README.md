### RTOS_version.cpp
OTA update implementation using an RTOS environment for task management and update handling.

### rollback-ota.cpp
OTA update mechanism with rollback support to restore the previous firmware version if an update fails.

### dummy_code.cpp
Sample code used for testing and simulation purposes.

### block diagram
Text-based representation of the OTA update system architecture and workflow.

## Project Objective

The objective of this project is to study and implement various OTA update techniques for embedded devices, including:

- Basic OTA firmware updates
- RTOS-based OTA implementation
- Firmware version management
- Rollback and recovery mechanisms
- Update process simulation and testing

---

## OTA Update Flow

1. Device checks for available firmware updates.
2. Firmware image is downloaded.
3. Firmware integrity is verified.
4. New firmware is installed.
5. Device reboots with the updated firmware.
6. Rollback is performed if the update fails.

---

## Technologies Used

- C++
- Embedded Systems Concepts
- RTOS
- OTA Firmware Update Techniques

---

## Author

**Tejas Chaudhari**

GitHub: https://github.com/tejaschaudhari457
