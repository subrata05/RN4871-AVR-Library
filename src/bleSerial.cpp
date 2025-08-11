/*
 * bleSerial.cpp
 *
 * Created: 09-07-2025 11:39:17
 * Author: Subrata
 * Description: Implementation of UART communication for the RN4871 BLE module
 *              using UART0 on the ATmega328PB microcontroller.
 */

#include "bleSerial.h"

ble_uart_t ble;

// -----------------------------------------------------------------------------------
// UART initialization procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Configures UART0 for communication with the RN4871 at 9600 baud, initializing
// ring buffers and enabling UART interrupts.
// -----------------------------------------------------------------------------------
void bleInit(void) {
    if (ble.initialized) return;

    RingBuffer_init(&ble.rx_buffer, ble.rx_storage, BLE_BUFFER_SIZE); // Initialize receive buffer
    RingBuffer_init(&ble.tx_buffer, ble.tx_storage, BLE_BUFFER_SIZE); // Initialize transmit buffer

    UBRR0 = BLE_UBRR_VALUE; // Set baud rate
    UCSR0A = (1 << U2X0); // Enable double speed mode
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << UDRIE0); // Enable RX, TX, interrupts
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data, no parity, 1 stop bit

    ble.initialized = true;
}

// -----------------------------------------------------------------------------------
// Check available bytes procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: int - Number of bytes available
// Returns the number of bytes available in the UART receive buffer.
// -----------------------------------------------------------------------------------
int bleAvailable(void) {
    return RingBuffer_available(&ble.rx_buffer);
}

// -----------------------------------------------------------------------------------
// Read byte procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: int - The byte read or -1 if empty
// Reads a single byte from the UART receive buffer, returning -1 if empty.
// -----------------------------------------------------------------------------------
int bleRead(void) {
    char data;
    if (!RingBuffer_pop(&ble.rx_buffer, &data)) {
        return -1; // Buffer empty
    }
    return (int)data;
}

// -----------------------------------------------------------------------------------
// Read multiple bytes procedure
// -----------------------------------------------------------------------------------
// Input : buffer - Destination buffer, length - Number of bytes to read
// Output: size_t - Number of bytes read
// Reads up to the specified number of bytes from the UART receive buffer with a timeout.
// -----------------------------------------------------------------------------------
size_t bleReadBytes(char* buffer, uint16_t length) {
    size_t bytesRead = 0;
    uint32_t start = millis();
    const uint16_t timeout = 1000;

    while (bytesRead < length && (millis() - start < timeout)) {
        if (bleAvailable()) {
            int c = bleRead();
            if (c != -1) {
                buffer[bytesRead++] = (char)c;
            }
        }
    }
    return bytesRead;
}

// -----------------------------------------------------------------------------------
// Write byte procedure
// -----------------------------------------------------------------------------------
// Input : data - Byte to write
// Output: bool - True if successful, false if buffer full
// Writes a single byte to the UART transmit buffer and enables the transmit interrupt.
// -----------------------------------------------------------------------------------
bool blePrint(uint8_t data) {
    if (!RingBuffer_push(&ble.tx_buffer, (char)data)) {
        return false; // Buffer full
    }
    UCSR0B |= (1 << UDRIE0); // Enable transmit interrupt
    return true;
}

// -----------------------------------------------------------------------------------
// Write character procedure
// -----------------------------------------------------------------------------------
// Input : data - Character to write
// Output: bool - True if successful, false if buffer full
// Writes a single character to the UART transmit buffer and enables the transmit interrupt.
// -----------------------------------------------------------------------------------
bool blePrintChar(char data) {
    if (!RingBuffer_push(&ble.tx_buffer, data)) {
        return false; // Buffer full
    }
    UCSR0B |= (1 << UDRIE0); // Enable transmit interrupt
    return true;
}

// -----------------------------------------------------------------------------------
// Write string procedure
// -----------------------------------------------------------------------------------
// Input : str - The string to write
// Output: void
// Writes a null-terminated string to the UART transmit buffer, sending each character.
// -----------------------------------------------------------------------------------
void blePrintString(const char* str) {
    while (*str) {
        while (!blePrintChar(*str++)); // Send character, wait if buffer full
    }
}

// -----------------------------------------------------------------------------------
// Write string with CR+LF procedure
// -----------------------------------------------------------------------------------
// Input : str - The string to write
// Output: void
// Writes a string followed by a carriage return and line feed to the UART transmit buffer.
// -----------------------------------------------------------------------------------
void blePrintStringln(const char* str) {
    blePrintString(str); // Write string
    blePrintString("\r\n"); // Append CR+LF
}

// -----------------------------------------------------------------------------------
// Flush transmit buffer procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Clears the UART transmit buffer and disables the transmit interrupt.
// -----------------------------------------------------------------------------------
void bleTxFlush(void) {
    UCSR0B &= ~(1 << UDRIE0); // Disable transmit interrupt
    ble.tx_buffer.head = 0; // Reset buffer pointers
    ble.tx_buffer.tail = 0;
}

// -----------------------------------------------------------------------------------
// Flush receive buffer procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Clears the UART receive buffer by resetting its pointers.
// -----------------------------------------------------------------------------------
void bleRxFlush(void) {
    ble.rx_buffer.head = 0; // Reset buffer pointers
    ble.rx_buffer.tail = 0;
}

// -----------------------------------------------------------------------------------
// UART receive interrupt handler
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Handles incoming UART data by pushing it to the receive ring buffer.
// -----------------------------------------------------------------------------------
ISR(USART0_RX_vect) {
    char data = (char)UDR0; // Read incoming byte
    RingBuffer_push(&ble.rx_buffer, data); // Push to buffer, ignore if full
}

// -----------------------------------------------------------------------------------
// UART transmit interrupt handler
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Sends the next byte from the transmit buffer when the UART data register is empty.
// -----------------------------------------------------------------------------------
ISR(USART0_UDRE_vect) {
    char data;
    if (RingBuffer_pop(&ble.tx_buffer, &data)) {
        UDR0 = (uint8_t)data; // Send byte
    } else {
        UCSR0B &= ~(1 << UDRIE0); // Disable interrupt if buffer empty
    }
}
