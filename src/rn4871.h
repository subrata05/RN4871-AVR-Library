/*
 * rn4871.h
 *
 * Created: 09-07-2025 11:40:39
 * Author: Subrata
 * Description: Header file for the RN4871 Bluetooth Low Energy (BLE) module
 *              library, designed for the ATmega328PB microcontroller. Declares
 *              functions and types for controlling the RN4871 module via UART0.
 */

#ifndef RN4871_H_
#define RN4871_H_

#include "bleSerial.h"
#include "wiring.h"
#include "rn4871_const.h"

typedef enum {
    dataMode, // Transparent UART data mode
    cmdMode   // Command mode for configuration
} operationMode_t;

extern operationMode_t operationMode;

void hwInit(uint8_t rstPin, volatile uint8_t *ddr, volatile uint8_t *port);
bool expectResponse(const char* expectedResponse, uint16_t timeout);
void sendCommand(const char* command);
void sendData(const char* data, uint16_t dataLen);
bool reboot(void);
void setOperationMode(operationMode_t newMode);
operationMode_t getOperationMode(void);
void flush(void);
bool swInit(void);
void cleanInputBuffer(void);
void enterDataMode(void);
bool enterCommandMode(void);
bool clearAllServices(void);
bool stopAdvertising(void);
bool clearPermanentAdvertising(void);
bool clearPermanentBeacon(void);
bool clearImmediateAdvertising(void);
bool clearImmediateBeacon(void);
bool setSerializedName(const char *newName);
bool setSupportedFeatures(uint16_t bitmap);
bool setDefaultServices(uint8_t bitmap);
bool setAdvPower(uint8_t value);
bool setServiceUUID(const char *uuid);
bool setCharactUUID(const char *uuid, uint8_t property, uint8_t octetLen);
bool startPermanentAdvertising(uint8_t adType, const char adData[]);
bool startCustomAdvertising(uint16_t interval);
const char* getDeviceName(void);
uint16_t readUntilCR(char* buffer, uint16_t size, uint16_t start = 0);
uint16_t readUntilCR(void);
int getConnectionStatus(void);
const char* getLastResponse(void);
bool writeLocalCharacteristic(uint16_t handle, const char value[]);
bool readLocalCharacteristic(uint16_t handle);
bool getFirmwareVersion(void);
bool startScanning(void);
bool startAdvertising(void);
uint16_t parseLsCmd(const char* targetUuid, uint8_t targetProperty);
uint16_t findHandle(const char* targetUuid, uint8_t targetProperty);

#endif /* RN4871_H_ */
