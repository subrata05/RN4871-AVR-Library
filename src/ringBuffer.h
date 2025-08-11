/*
 * ringBuffer.h
 *
 * Created: 09-07-2025 11:39:34
 * Author: Subrata
 * Description: Header file for a ring buffer implementation used in UART
 *              communication with the RN4871 BLE module.
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t* buffer; // Pointer to buffer storage
    uint8_t size;    // Buffer size (must be power of 2)
    uint8_t head;    // Write index
    uint8_t tail;    // Read index
} RingBuffer_t;

void RingBuffer_init(RingBuffer_t* rb, uint8_t* buffer, uint8_t size);
bool RingBuffer_push(RingBuffer_t* rb, char data);
bool RingBuffer_pop(RingBuffer_t* rb, char* data);
int RingBuffer_available(RingBuffer_t* rb);
bool RingBuffer_is_empty(RingBuffer_t* rb);
bool RingBuffer_is_full(RingBuffer_t* rb);

#endif /* RINGBUFFER_H_ */
