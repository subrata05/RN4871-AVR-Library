/*
 * rn4871.cpp
 *
 * Created: 09-07-2025 11:40:30
 * Author: Subrata
 * Description: Implementation of the RN4871 Bluetooth Low Energy (BLE) module
 *              library for the ATmega328PB microcontroller. Provides functions
 *              for configuring and controlling the RN4871 module via UART0.
 */

#include "rn4871.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_INPUT_BUFFER_SIZE 128
char uartBuffer[DEFAULT_INPUT_BUFFER_SIZE];
int uartBufferLen = DEFAULT_INPUT_BUFFER_SIZE;
char deviceName[MAX_DEVICE_NAME_LEN];

operationMode_t operationMode = dataMode;

// -----------------------------------------------------------------------------------
// Hardware initialization procedure
// -----------------------------------------------------------------------------------
// Input : rstPin - Reset pin number, ddr - Data direction register, port - Port register
// Output: void
// Configures the reset pin and performs a hardware reset if a valid pin is provided,
// ensuring the RN4871 module is properly initialized.
// -----------------------------------------------------------------------------------
void hwInit(uint8_t rstPin, volatile uint8_t *ddr, volatile uint8_t *port) {
    if (rstPin != -1) {
        *ddr |= (1 << rstPin); // Set reset pin as output
        *port &= ~(1 << rstPin); // Pull reset pin low
        _delay_ms(1); // Hold low for 1ms
        *port |= (1 << rstPin); // Pull reset pin high
        _delay_ms(500); // Wait for module stabilization
    }
}

// -----------------------------------------------------------------------------------
// Expect response procedure
// -----------------------------------------------------------------------------------
// Input : expectedResponse - The response string to expect, timeout - Timeout in ms
// Output: bool - True if response matches, false otherwise
// Reads UART data into a buffer and checks for the expected response within the
// specified timeout period, returning true if found.
// -----------------------------------------------------------------------------------
bool expectResponse(const char* expectedResponse, uint16_t timeout) {
    uint8_t index = 0;
    uint32_t start;

    bleRxFlush(); // Clear receive buffer
    memset(uartBuffer, 0, sizeof(uartBuffer)); // Initialize buffer

    start = millis();
    while (millis() - start < timeout) {
        if (bleAvailable()) {
            char c = bleRead();
            if (c != -1) {
                if (c == '\n') {
                    uartBuffer[index] = '\0';
                    if (strstr(uartBuffer, expectedResponse) != NULL) {
                        return true; // Response found
                    }
                    return false; // Complete message, no match
                } else if (index < sizeof(uartBuffer) - 1) {
                    uartBuffer[index++] = c;
                }
            }
        }
    }
    return false; // Timeout occurred
}

// -----------------------------------------------------------------------------------
// Send command procedure
// -----------------------------------------------------------------------------------
// Input : command - The ASCII command string to send
// Output: void
// Sends a command to the RN4871 module via UART, appending a carriage return,
// and flushes transmit/receive buffers for clean communication.
// -----------------------------------------------------------------------------------
void sendCommand(const char* command) {
    bleTxFlush(); // Clear transmit buffer
    bleRxFlush(); // Clear receive buffer
    blePrintString(command); // Send command
    blePrintChar(CR); // Append carriage return
}

// -----------------------------------------------------------------------------------
// Send data procedure
// -----------------------------------------------------------------------------------
// Input : data - Pointer to data buffer, dataLen - Length of data
// Output: void
// Transmits raw data to the RN4871 in data mode, sending each byte individually
// for Transparent UART communication.
// -----------------------------------------------------------------------------------
void sendData(const char* data, uint16_t dataLen) {
    for (uint16_t i = 0; i < dataLen; i++) {
        while (!blePrintChar(data[i])); // Send byte, wait if buffer full
    }
}

// -----------------------------------------------------------------------------------
// Reboot module procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if reboot successful, false otherwise
// Sends the reboot command to the RN4871 and waits for the reboot response,
// including a delay for module stabilization.
// -----------------------------------------------------------------------------------
bool reboot(void) {
    sendCommand(REBOOT); // Send reboot command
    if (expectResponse(REBOOTING_RESP, RESET_CMD_TIMEOUT)) {
        _delay_ms(RESET_CMD_TIMEOUT); // Wait for reboot completion
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------------
// Set operation mode procedure
// -----------------------------------------------------------------------------------
// Input : newMode - The new operation mode (cmdMode or dataMode)
// Output: void
// Updates the internal state to reflect the RN4871 module's operation mode.
// -----------------------------------------------------------------------------------
void setOperationMode(operationMode_t newMode) {
    operationMode = newMode;
}

// -----------------------------------------------------------------------------------
// Get operation mode procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: operationMode_t - Current operation mode
// Returns the current operation mode of the RN4871 module.
// -----------------------------------------------------------------------------------
operationMode_t getOperationMode(void) {
    return operationMode;
}

// -----------------------------------------------------------------------------------
// Software initialization procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if initialization successful, false otherwise
// Initializes the RN4871 module by rebooting and setting it to data mode,
// retrying if already in command mode.
// -----------------------------------------------------------------------------------
bool swInit(void) {
    bool initOk = false;
    if (reboot()) {
        setOperationMode(dataMode); // Set to data mode
        initOk = true;
    } else if (enterCommandMode()) {
        if (reboot()) {
            setOperationMode(dataMode);
            initOk = true;
        }
    }
    return initOk;
}

// -----------------------------------------------------------------------------------
// Flush UART buffer procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Clears the UART buffer to ensure no residual data remains.
// -----------------------------------------------------------------------------------
void flush(void) {
    memset(uartBuffer, 0, DEFAULT_INPUT_BUFFER_SIZE);
}

// -----------------------------------------------------------------------------------
// Clear input buffer procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Reads and discards all available data in the UART receive buffer.
// -----------------------------------------------------------------------------------
void cleanInputBuffer(void) {
    while (bleAvailable() > 0) {
        bleRead(); // Discard all available bytes
    }
}

// -----------------------------------------------------------------------------------
// Enter data mode procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Sends the exit command to switch the RN4871 to data mode and updates the internal state.
// -----------------------------------------------------------------------------------
void enterDataMode(void) {
    sendCommand(EXIT_CMD); // Send exit command
    setOperationMode(dataMode); // Update mode
}

// -----------------------------------------------------------------------------------
// Enter command mode procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if command mode entered, false otherwise
// Sends the command mode entry sequence ($$$) and waits for the command prompt response.
// -----------------------------------------------------------------------------------
bool enterCommandMode(void) {
    _delay_ms(DELAY_BEFORE_CMD); // Wait before sending command
    flush(); // Clear UART buffer
    bleTxFlush(); // Clear transmit buffer
    cleanInputBuffer(); // Clear receive buffer
    blePrintString(ENTER_CMD); // Send $$$ to enter command mode

    uint32_t start = millis();
    while ((millis() - start < 30) && (bleAvailable() < 5)) {} // Wait for response

    bleReadBytes(uartBuffer, bleAvailable()); // Read response
    if (strstr(uartBuffer, PROMPT) != NULL || strstr(uartBuffer, PROMPT_CR) != NULL) {
        setOperationMode(cmdMode); // Update mode
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------------
// Clear all services procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Sends the command to clear all defined GATT services on the RN4871 module.
// -----------------------------------------------------------------------------------
bool clearAllServices(void) {
    sendCommand(CLEAR_ALL_SERVICES); // Send clear services command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Stop advertising procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Sends the command to stop the RN4871 from advertising.
// -----------------------------------------------------------------------------------
bool stopAdvertising(void) {
    sendCommand(STOP_ADV); // Send stop advertising command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Clear permanent advertising procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Clears all permanent advertising data stored on the RN4871 module.
// -----------------------------------------------------------------------------------
bool clearPermanentAdvertising(void) {
    sendCommand(CLEAR_PERMANENT_ADV); // Send clear permanent advertising command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Clear permanent beacon procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Clears all permanent beacon data stored on the RN4871 module.
// -----------------------------------------------------------------------------------
bool clearPermanentBeacon(void) {
    sendCommand(CLEAR_PERMANENT_BEACON); // Send clear permanent beacon command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Clear immediate advertising procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Clears all immediate advertising data stored on the RN4871 module.
// -----------------------------------------------------------------------------------
bool clearImmediateAdvertising(void) {
    sendCommand(CLEAR_IMMEDIATE_ADV); // Send clear immediate advertising command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Clear immediate beacon procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Clears all immediate beacon data stored on the RN4871 module.
// -----------------------------------------------------------------------------------
bool clearImmediateBeacon(void) {
    sendCommand(CLEAR_IMMEDIATE_BEACON); // Send clear immediate beacon command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Set serialized name procedure
// -----------------------------------------------------------------------------------
// Input : newName - The new serialized name
// Output: bool - True if successful, false otherwise
// Configures the serialized name, ensuring it does not exceed the maximum length,
// and stores it locally.
// -----------------------------------------------------------------------------------
bool setSerializedName(const char *newName) {
    uint8_t len = strlen(SET_SERIALIZED_NAME);
    uint8_t newLen = strlen(newName);
    if (newLen > MAX_SERIALIZED_NAME_LEN) {
        newLen = MAX_SERIALIZED_NAME_LEN; // Truncate if too long
    }

    memset(deviceName, 0, newLen); // Clear local name buffer
    memcpy(deviceName, newName, newLen); // Store name locally
    bleTxFlush(); // Clear transmit buffer
    memcpy(uartBuffer, SET_SERIALIZED_NAME, len); // Copy command prefix
    memcpy(&uartBuffer[len], newName, newLen); // Append name
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Set supported features procedure
// -----------------------------------------------------------------------------------
// Input : bitmap - The feature bitmap
// Output: bool - True if successful, false otherwise
// Configures the supported features using a bitmap, formatted as a 4-digit hex string.
// -----------------------------------------------------------------------------------
bool setSupportedFeatures(uint16_t bitmap) {
    uint8_t len = strlen(SET_SUPPORTED_FEATURES);
    char c[5];
    sprintf(c, "%04X", bitmap); // Format bitmap as 4-digit hex
    uint8_t newLen = strlen(c);

    flush(); // Clear UART buffer
    memcpy(uartBuffer, SET_SUPPORTED_FEATURES, len); // Copy command prefix
    memcpy(&uartBuffer[len], c, newLen); // Append bitmap
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Set default services procedure
// -----------------------------------------------------------------------------------
// Input : bitmap - The service bitmap
// Output: bool - True if successful, false otherwise
// Configures the default services using a bitmap, formatted as a 2-digit hex string.
// -----------------------------------------------------------------------------------
bool setDefaultServices(uint8_t bitmap) {
    uint8_t len = strlen(SET_DEFAULT_SERVICES);
    char c[3];
    sprintf(c, "%02X", bitmap); // Format bitmap as 2-digit hex
    uint8_t newLen = strlen(c);

    flush(); // Clear UART buffer
    memcpy(uartBuffer, SET_DEFAULT_SERVICES, len); // Copy command prefix
    memcpy(&uartBuffer[len], c, newLen); // Append bitmap
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Set advertising power procedure
// -----------------------------------------------------------------------------------
// Input : value - The power level (0 to 5)
// Output: bool - True if successful, false otherwise
// Configures the advertising power level, clamping the value within the valid range.
// -----------------------------------------------------------------------------------
bool setAdvPower(uint8_t value) {
    if (value > MAX_POWER_OUTPUT) {
        value = MAX_POWER_OUTPUT; // Clamp to maximum
    }

    uint8_t len = strlen(SET_ADV_POWER);
    char c[2];
    sprintf(c, "%d", value); // Format power level
    flush(); // Clear UART buffer
    memcpy(uartBuffer, SET_ADV_POWER, len); // Copy command prefix
    memcpy(&uartBuffer[len], c, 1); // Append power level
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Set service UUID procedure
// -----------------------------------------------------------------------------------
// Input : uuid - The service UUID (16-bit or 128-bit)
// Output: bool - True if successful, false otherwise
// Defines a service UUID, validating its length before sending the command.
// -----------------------------------------------------------------------------------
bool setServiceUUID(const char *uuid) {
    uint8_t len = strlen(DEFINE_SERVICE_UUID);
    uint8_t newLen = strlen(uuid);

    if (newLen != PRIVATE_SERVICE_LEN && newLen != PUBLIC_SERVICE_LEN) {
        return false; // Invalid UUID length
    }

    flush(); // Clear UART buffer
    memcpy(uartBuffer, DEFINE_SERVICE_UUID, len); // Copy command prefix
    memcpy(&uartBuffer[len], uuid, newLen); // Append UUID
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Set characteristic UUID procedure
// -----------------------------------------------------------------------------------
// Input : uuid - The characteristic UUID, property - Property bitmap, octetLen - Data length
// Output: bool - True if successful, false otherwise
// Defines a characteristic UUID with properties and octet length, validating inputs.
// -----------------------------------------------------------------------------------
bool setCharactUUID(const char *uuid, uint8_t property, uint8_t octetLen) {
    if (octetLen < 0x01) {
        octetLen = 0x01; // Minimum octet length
    } else if (octetLen > 0x14) {
        octetLen = 0x14; // Maximum octet length
    }

    uint8_t newLen = strlen(uuid);
    if (newLen != PRIVATE_SERVICE_LEN && newLen != PUBLIC_SERVICE_LEN) {
        return false; // Invalid UUID length
    }

    const uint8_t prefixLen = strlen(DEFINE_CHARACT_UUID);
    const uint8_t maxUuidLen = (PRIVATE_SERVICE_LEN > PUBLIC_SERVICE_LEN) ? PRIVATE_SERVICE_LEN : PUBLIC_SERVICE_LEN;
    const uint8_t bufferSize = prefixLen + maxUuidLen + 1 + 2 + 1 + 2 + 1; // prefix + uuid + ',' + prop + ',' + octet + '\0'
    char cmdBuffer[bufferSize];
    cmdBuffer[0] = '\0';

    strcpy(cmdBuffer, DEFINE_CHARACT_UUID); // Copy prefix
    strcat(cmdBuffer, uuid); // Append UUID
    strcat(cmdBuffer, ","); // Append comma
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", property); // Format property
    strcat(cmdBuffer, hex); // Append property
    strcat(cmdBuffer, ","); // Append comma
    snprintf(hex, sizeof(hex), "%02X", octetLen); // Format octet length
    strcat(cmdBuffer, hex); // Append octet length

    sendCommand(cmdBuffer); // Send command
    return expectResponse(AOK_RESP, 500); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Start permanent advertising procedure
// -----------------------------------------------------------------------------------
// Input : adType - Advertising type, adData - Advertising data
// Output: bool - True if successful, false otherwise
// Configures and starts permanent advertising with the specified type and data.
// -----------------------------------------------------------------------------------
bool startPermanentAdvertising(uint8_t adType, const char adData[]) {
    uint8_t len = strlen(START_PERMANENT_ADV);
    char c[3];
    sprintf(c, "%02X", adType); // Format ad type
    uint8_t newLen = strlen(c);

    flush(); // Clear UART buffer
    memcpy(uartBuffer, START_PERMANENT_ADV, len); // Copy command prefix
    memcpy(&uartBuffer[len], c, newLen); // Append ad type
    memcpy(&uartBuffer[len + newLen], ",", 1); // Append comma
    memcpy(&uartBuffer[len + newLen + 1], adData, strlen(adData)); // Append ad data
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Start custom advertising procedure
// -----------------------------------------------------------------------------------
// Input : interval - Advertising interval in milliseconds
// Output: bool - True if successful, false otherwise
// Starts advertising with a custom interval, formatted as a 4-digit hex string.
// -----------------------------------------------------------------------------------
bool startCustomAdvertising(uint16_t interval) {
    uint8_t len = strlen(START_CUSTOM_ADV);
    char parameters[5];
    sprintf(parameters, "%04X", interval); // Format interval
    uint8_t newLen = strlen(parameters);

    flush(); // Clear UART buffer
    memcpy(uartBuffer, START_CUSTOM_ADV, len); // Copy command prefix
    memcpy(&uartBuffer[len], parameters, newLen); // Append interval
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Get device name procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: const char* - Pointer to the device name string
// Returns the locally stored device name set via setSerializedName.
// -----------------------------------------------------------------------------------
const char* getDeviceName() {
    return deviceName;
}

// -----------------------------------------------------------------------------------
// Get connection status procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: int - 1 (connected), 0 (not connected), -1 (timeout/error)
// Queries the RN4871 for its connection status, parsing the response.
// -----------------------------------------------------------------------------------
int getConnectionStatus(void) {
    uint16_t timeout = DEFAULT_CMD_TIMEOUT;
    unsigned long previous;

    sendCommand(GET_CONNECTION_STATUS); // Send connection status command
    previous = millis();
    while (millis() - previous < timeout) {
        if (bleAvailable() > 0) {
            if (readUntilCR() > 0) {
                if (strstr(uartBuffer, NONE_RESP) != NULL) {
                    return 0; // Not connected
                }
                return 1; // Connected
            }
        }
    }
    return -1; // Timeout
}

// -----------------------------------------------------------------------------------
// Read until carriage return procedure
// -----------------------------------------------------------------------------------
// Input : buffer - Destination buffer, size - Buffer size, start - Starting index
// Output: uint16_t - Number of bytes read
// Reads UART data into a buffer until a carriage return is encountered or timeout.
// -----------------------------------------------------------------------------------
uint16_t readUntilCR(char* buffer, uint16_t size, uint16_t start) {
    uint16_t bytesRead = 0;
    uint32_t startTime = millis();
    const uint16_t timeout = 1000;

    while (bytesRead < size - start - 1) {
        if (millis() - startTime >= timeout) {
            break; // Timeout
        }
        if (bleAvailable()) {
            int c = bleRead();
            if (c == -1) {
                continue;
            }
            if (c == CR) {
                break; // Stop at CR
            }
            buffer[start + bytesRead] = (char)c;
            bytesRead++;
        }
    }
    buffer[start + bytesRead] = '\0'; // Null-terminate
    return bytesRead;
}

// -----------------------------------------------------------------------------------
// Read until carriage return (default buffer) procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: uint16_t - Number of bytes read
// Reads UART data into the default buffer until a carriage return is encountered.
// -----------------------------------------------------------------------------------
uint16_t readUntilCR() {
    return readUntilCR(uartBuffer, uartBufferLen);
}

// -----------------------------------------------------------------------------------
// Get last response procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: const char* - Pointer to the last response string
// Returns the contents of the UART buffer containing the last RN4871 response.
// -----------------------------------------------------------------------------------
const char* getLastResponse() {
    return uartBuffer;
}

// -----------------------------------------------------------------------------------
// Write local characteristic procedure
// -----------------------------------------------------------------------------------
// Input : handle - Characteristic handle, value - Data to write
// Output: bool - True if successful, false otherwise
// Writes a value to a local characteristic identified by its handle.
// -----------------------------------------------------------------------------------
bool writeLocalCharacteristic(uint16_t handle, const char value[]) {
    uint8_t len = strlen(WRITE_LOCAL_CHARACT);
    char c[5];
    sprintf(c, "%04X", handle); // Format handle
    uint8_t newLen = strlen(c);

    flush(); // Clear UART buffer
    memcpy(uartBuffer, WRITE_LOCAL_CHARACT, len); // Copy command prefix
    memcpy(&uartBuffer[len], c, newLen); // Append handle
    memcpy(&uartBuffer[len + newLen], ",", 1); // Append comma
    memcpy(&uartBuffer[len + newLen + 1], value, strlen(value)); // Append value
    sendCommand(uartBuffer); // Send command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Read local characteristic procedure
// -----------------------------------------------------------------------------------
// Input : handle - Characteristic handle
// Output: bool - True if successful, false otherwise
// Reads the value of a local characteristic, storing it in the UART buffer.
// -----------------------------------------------------------------------------------
bool readLocalCharacteristic(uint16_t handle) {
    uint16_t timeout = DEFAULT_CMD_TIMEOUT;
    unsigned long previous;

    uint8_t len = strlen(READ_LOCAL_CHARACT);
    char c[5];
    sprintf(c, "%04X", handle); // Format handle
    uint8_t newLen = strlen(c);

    flush(); // Clear UART buffer
    memcpy(uartBuffer, READ_LOCAL_CHARACT, len); // Copy command prefix
    memcpy(&uartBuffer[len], c, newLen); // Append handle
    sendCommand(uartBuffer); // Send command
    previous = millis();
    while (millis() - previous < timeout) {
        if (bleAvailable() > 0) {
            if (readUntilCR() > 0) {
                return true; // Data read successfully
            }
        }
    }
    return false; // Timeout
}

// -----------------------------------------------------------------------------------
// Get firmware version procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if successful, false otherwise
// Queries the RN4871 for its firmware version, storing it in the UART buffer.
// -----------------------------------------------------------------------------------
bool getFirmwareVersion(void) {
    uint16_t timeout = DEFAULT_CMD_TIMEOUT;
    unsigned long previous;

    sendCommand(DISPLAY_FW_VERSION); // Send firmware version command
    previous = millis();
    while (millis() - previous < timeout) {
        if (bleAvailable() > 0) {
            if (readUntilCR() > 0) {
                return true; // Version read successfully
            }
        }
    }
    return false; // Timeout
}

// -----------------------------------------------------------------------------------
// Start scanning procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if scanning started, false otherwise
// Initiates a BLE scan to detect nearby devices.
// -----------------------------------------------------------------------------------
bool startScanning(void) {
    sendCommand(START_DEFAULT_SCAN); // Send scan command
    return expectResponse(SCANNING_RESP, DEFAULT_CMD_TIMEOUT); // Check for scanning response
}

// -----------------------------------------------------------------------------------
// Start advertising procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: bool - True if advertising started, false otherwise
// Starts the default advertising mode on the RN4871 module.
// -----------------------------------------------------------------------------------
bool startAdvertising(void) {
    sendCommand(START_DEFAULT_ADV); // Send advertising command
    return expectResponse(AOK_RESP, DEFAULT_CMD_TIMEOUT); // Check for AOK response
}

// -----------------------------------------------------------------------------------
// Parse LS command output procedure
// -----------------------------------------------------------------------------------
// Input : targetUuid - The UUID to match, targetProperty - The property to match
// Output: uint16_t - The handle if found, 0 otherwise
// Parses the LS command output to find a characteristic handle matching the specified UUID and property.
// -----------------------------------------------------------------------------------
uint16_t parseLsCmd(const char* targetUuid, uint8_t targetProperty) {
    uint8_t index = 0;
    uint16_t timeout = DEFAULT_CMD_TIMEOUT;
    uint32_t start;
    bool endReceived = false;
    bool gotCarriageReturn = false;
    uint16_t foundHandle = 0;

    bleRxFlush(); // Clear receive buffer
    flush(); // Clear UART buffer

    start = millis();
    while (millis() - start < timeout && !endReceived) {
        if (bleAvailable()) {
            char c = bleRead();
            if (c != -1) {
                if (c == '\r') {
                    gotCarriageReturn = true;
                } else if (c == '\n' && gotCarriageReturn) {
                    uartBuffer[index] = '\0';
                    if (index > 0) { // Process non-empty lines
                        if (strstr(uartBuffer, targetUuid) != NULL) {
                            char* firstComma = strchr(uartBuffer, ',');
                            if (firstComma != NULL && firstComma[1] != '\0') {
                                char handleStr[5] = {0};
                                if (strlen(firstComma + 1) >= 4) {
                                    strncpy(handleStr, firstComma + 1, 4);
                                    handleStr[4] = '\0';
                                    uint16_t handle = 0;
                                    for (int i = 0; i < 4; i++) {
                                        handle <<= 4;
                                        if (handleStr[i] >= '0' && handleStr[i] <= '9') {
                                            handle |= (handleStr[i] - '0');
                                        } else if (handleStr[i] >= 'A' && handleStr[i] <= 'F') {
                                            handle |= (handleStr[i] - 'A' + 10);
                                        } else if (handleStr[i] >= 'a' && handleStr[i] <= 'f') {
                                            handle |= (handleStr[i] - 'a' + 10);
                                        } else {
                                            handle = 0;
                                            break;
                                        }
                                    }
                                    char* secondComma = strchr(firstComma + 1, ',');
                                    if (secondComma != NULL && secondComma[1] != '\0') {
                                        char propertyStr[3] = {0};
                                        if (strlen(secondComma + 1) >= 2) {
                                            strncpy(propertyStr, secondComma + 1, 2);
                                            propertyStr[2] = '\0';
                                            uint8_t property = 0;
                                            for (int i = 0; i < 2; i++) {
                                                property <<= 4;
                                                if (propertyStr[i] >= '0' && propertyStr[i] <= '9') {
                                                    property |= (propertyStr[i] - '0');
                                                } else if (propertyStr[i] >= 'A' && propertyStr[i] <= 'F') {
                                                    property |= (propertyStr[i] - 'A' + 10);
                                                } else if (propertyStr[i] >= 'a' && propertyStr[i] <= 'f') {
                                                    property |= (propertyStr[i] - 'a' + 10);
                                                } else {
                                                    property = 0;
                                                    break;
                                                }
                                            }
                                            if (property == targetProperty) {
                                                foundHandle = handle; // Store matching handle
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (strcmp(uartBuffer, "END") == 0) {
                            endReceived = true; // End of LS command output
                        }
                    }
                    flush(); // Clear buffer for next line
                    index = 0;
                    gotCarriageReturn = false;
                } else {
                    if (gotCarriageReturn) {
                        if (index < sizeof(uartBuffer) - 1) {
                            uartBuffer[index++] = '\r';
                        }
                        gotCarriageReturn = false;
                    }
                    if (index < sizeof(uartBuffer) - 1) {
                        uartBuffer[index++] = c;
                    }
                }
            }
        }
    }

    if (index > 0 && !endReceived) { // Handle incomplete data
        uartBuffer[index] = '\0';
        if (strstr(uartBuffer, targetUuid) != NULL) {
            char* firstComma = strchr(uartBuffer, ',');
            if (firstComma != NULL && firstComma[1] != '\0') {
                char handleStr[5] = {0};
                if (strlen(firstComma + 1) >= 4) {
                    strncpy(handleStr, firstComma + 1, 4);
                    handleStr[4] = '\0';
                    uint16_t handle = 0;
                    for (int i = 0; i < 4; i++) {
                        handle <<= 4;
                        if (handleStr[i] >= '0' && handleStr[i] <= '9') {
                            handle |= (handleStr[i] - '0');
                        } else if (handleStr[i] >= 'A' && handleStr[i] <= 'F') {
                            handle |= (handleStr[i] - 'A' + 10);
                        } else if (handleStr[i] >= 'a' && handleStr[i] <= 'f') {
                            handle |= (handleStr[i] - 'a' + 10);
                        } else {
                            handle = 0;
                            break;
                        }
                    }
                    char* secondComma = strchr(firstComma + 1, ',');
                    if (secondComma != NULL && secondComma[1] != '\0') {
                        char propertyStr[3] = {0};
                        if (strlen(secondComma + 1) >= 2) {
                            strncpy(propertyStr, secondComma + 1, 2);
                            propertyStr[2] = '\0';
                            uint8_t property = 0;
                            for (int i = 0; i < 2; i++) {
                                property <<= 4;
                                if (propertyStr[i] >= '0' && propertyStr[i] <= '9') {
                                    property |= (propertyStr[i] - '0');
                                } else if (propertyStr[i] >= 'A' && propertyStr[i] <= 'F') {
                                    property |= (propertyStr[i] - 'A' + 10);
                                } else if (propertyStr[i] >= 'a' && propertyStr[i] <= 'f') {
                                    property |= (propertyStr[i] - 'a' + 10);
                                } else {
                                    property = 0;
                                    break;
                                }
                            }
                            if (property == targetProperty) {
                                foundHandle = handle;
                            }
                        }
                    }
                }
            }
        }
    }
    return foundHandle;
}

// -----------------------------------------------------------------------------------
// Find characteristic handle procedure
// -----------------------------------------------------------------------------------
// Input : targetUuid - The UUID to match, targetProperty - The property to match
// Output: uint16_t - The handle if found, 0 otherwise
// Sends the LS command and parses the output to find a characteristic handle matching
// the specified UUID and property.
// -----------------------------------------------------------------------------------
uint16_t findHandle(const char* targetUuid, uint8_t targetProperty) {
    sendCommand(LIST_SERVICES_AND_CHARS); // Send LS command
    _delay_ms(15); // Wait for response
    uint16_t handle = parseLsCmd(targetUuid, targetProperty); // Parse output
    return handle > 0 ? handle : 0; // Return handle or 0 if not found
}

