/*
 * ringBuffer.cpp
 *
 * Created: 09-07-2025 11:40:08
 * Author: Subrata
 * Description: Implementation of a ring buffer for UART communication with the
 *              RN4871 BLE module, providing efficient data storage and retrieval.
 */

#include "ringBuffer.h"

// -----------------------------------------------------------------------------------
// Ring buffer initialization procedure
// -----------------------------------------------------------------------------------
// Input : rb - Pointer to ring buffer, buffer - Storage array, size - Buffer size
// Output: void
// Initializes the ring buffer by setting the storage, size, and resetting head/tail indices.
// -----------------------------------------------------------------------------------
void RingBuffer_init(RingBuffer_t* rb, uint8_t* buffer, uint8_t size) {
    rb->buffer = buffer; // Set buffer storage
    rb->size = size; // Set buffer size
    rb->head = 0; // Initialize write index
    rb->tail = 0; // Initialize read index
}

// -----------------------------------------------------------------------------------
// Push character to ring buffer procedure
// -----------------------------------------------------------------------------------
// Input : rb - Pointer to ring buffer, data - Character to push
// Output: bool - True if successful, false if buffer full
// Adds a character to the ring buffer at the head index, checking for buffer full condition.
// -----------------------------------------------------------------------------------
bool RingBuffer_push(RingBuffer_t* rb, char data) {
    uint8_t next_head = (rb->head + 1) & (rb->size - 1); // Calculate next head
    if (next_head == rb->tail) {
        return false; // Buffer full
    }
    rb->buffer[rb->head] = (uint8_t)data; // Store data
    rb->head = next_head; // Update head
    return true;
}

// -----------------------------------------------------------------------------------
// Pop character from ring buffer procedure
// -----------------------------------------------------------------------------------
// Input : rb - Pointer to ring buffer, data - Pointer to store retrieved character
// Output: bool - True if successful, false if buffer empty
// Retrieves a character from the ring buffer at the tail index, checking for empty condition.
// -----------------------------------------------------------------------------------
bool RingBuffer_pop(RingBuffer_t* rb, char* data) {
    if (rb->head == rb->tail) {
        return false; // Buffer empty
    }
    *data = (char)rb->buffer[rb->tail]; // Retrieve data
    rb->tail = (rb->tail + 1) & (rb->size - 1); // Update tail
    return true;
}

// -----------------------------------------------------------------------------------
// Get available bytes procedure
// -----------------------------------------------------------------------------------
// Input : rb - Pointer to ring buffer
// Output: int - Number of bytes available
// Calculates the number of bytes currently stored in the ring buffer using bitwise operations.
// -----------------------------------------------------------------------------------
int RingBuffer_available(RingBuffer_t* rb) {
    return (rb->size + rb->head - rb->tail) & (rb->size - 1);
}

// -----------------------------------------------------------------------------------
// Check ring buffer empty procedure
// -----------------------------------------------------------------------------------
// Input : rb - Pointer to ring buffer
// Output: bool - True if empty, false otherwise
// Checks if the ring buffer contains no data by comparing head and tail indices.
// -----------------------------------------------------------------------------------
bool RingBuffer_is_empty(RingBuffer_t* rb) {
    return rb->head == rb->tail;
}

// -----------------------------------------------------------------------------------
// Check ring buffer full procedure
// -----------------------------------------------------------------------------------
// Input : rb - Pointer to ring buffer
// Output: bool - True if full, false otherwise
// Checks if the ring buffer is full by checking if the next head equals the tail.
// -----------------------------------------------------------------------------------
bool RingBuffer_is_full(RingBuffer_t* rb) {
    return ((rb->head + 1) & (rb->size - 1)) == rb->tail;
}
