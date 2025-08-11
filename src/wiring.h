/*
 * wiring.h
 *
 * Created: 09-07-2025 11:41:22
 * Author: Subrata
 * Description: Header file for timing functions on the ATmega328PB, providing
 *              millisecond timing using Timer0 for the RN4871 BLE library (this 
 *              wiring file is utilsed from the Arduino.h library).
 */

#ifndef WIRING_H_
#define WIRING_H_

#include <avr/io.h>
#include <avr/interrupt.h>

// 8 MHz clock frequency
#define F_CPU 8000000UL

// Timer0 configuration for millisecond timing
#define MICROSECONDS_PER_TIMER0_OVERFLOW ((64UL * 256UL) / (F_CPU / 1000000UL))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

extern volatile unsigned long timer0_overflow_count;
extern volatile unsigned long timer0_millis;
extern unsigned char timer0_fract;

unsigned long millis(void);
void initMillis(void);

#endif /* WIRING_H_ */
