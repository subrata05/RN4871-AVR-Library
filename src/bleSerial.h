/*
 * bleSerial.h
 *
 * Created: 09-07-2025 11:39:06
 * Author: Subrata
 * Description: Header file for UART communication with the RN4871 BLE module
 *              using UART0 on the ATmega328PB microcontroller.
 */

#ifndef BLESERIAL_H_
#define BLESERIAL_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include "ringBuffer.h"
#include "wiring.h"

// 8 MHz clock frequency
#define F_CPU 8000000UL
// UART baud rate
#define BLE_BAUD 9600
// Baud rate register value
#define BLE_UBRR_VALUE ((F_CPU / (8UL * BLE_BAUD)) - 1)
// Buffer size for UART communication
#define BLE_BUFFER_SIZE 64

typedef struct {
    bool initialized;                // Initialization status
    RingBuffer_t rx_buffer;         // Receive ring buffer
    RingBuffer_t tx_buffer;         // Transmit ring buffer
    uint8_t rx_storage[BLE_BUFFER_SIZE]; // Receive buffer storage
    uint8_t tx_storage[BLE_BUFFER_SIZE]; // Transmit buffer storage
} ble_uart_t;

extern ble_uart_t ble;

void bleInit(void);
int bleAvailable(void);
int bleRead(void);
bool blePrint(uint8_t data);
bool blePrintChar(char data);
void blePrintString(const char* str);
void blePrintStringln(const char* str);
void bleTxFlush(void);
void bleRxFlush(void);
size_t bleReadBytes(char* buffer, uint16_t length);

#endif /* BLESERIAL_H_ */
