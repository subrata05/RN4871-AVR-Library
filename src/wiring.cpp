/*
 * wiring.cpp
 *
 * Created: 09-07-2025 11:41:11
 * Author: Subrata
 * Description: Implementation of timing functions for the ATmega328PB, providing
 *              millisecond timing using Timer0 for the RN4871 BLE library.
 */

#include "wiring.h"

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
unsigned char timer0_fract = 0;

// -----------------------------------------------------------------------------------
// Timer0 overflow interrupt handler
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Increments the millisecond counter on Timer0 overflow, handling fractional milliseconds.
// -----------------------------------------------------------------------------------
ISR(TIMER0_OVF_vect) {
    unsigned long m = timer0_millis;
    unsigned char f = timer0_fract;

    m += MILLIS_INC; // Add whole milliseconds
    f += FRACT_INC; // Add fractional milliseconds
    if (f >= FRACT_MAX) {
        f -= FRACT_MAX; // Adjust for overflow
        m += 1; // Increment millisecond counter
    }

    timer0_fract = f;
    timer0_millis = m;
    timer0_overflow_count++;
}

// -----------------------------------------------------------------------------------
// Get millisecond count procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: unsigned long - Current millisecond count
// Returns the current millisecond count, ensuring atomic read by disabling interrupts.
// -----------------------------------------------------------------------------------
unsigned long millis(void) {
    unsigned long m;
    uint8_t oldSREG = SREG;

    cli(); // Disable interrupts
    m = timer0_millis; // Read millisecond counter
    SREG = oldSREG; // Restore interrupt state

    return m;
}

// -----------------------------------------------------------------------------------
// Initialize Timer0 procedure
// -----------------------------------------------------------------------------------
// Input : None
// Output: void
// Configures Timer0 with a prescaler of 64 for millisecond timing and enables overflow interrupt.
// -----------------------------------------------------------------------------------
void initMillis(void) {
    sei(); // Enable global interrupts
    TCCR0A = 0; // Normal mode
    TCCR0B = 0;
    TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler 64
    TIMSK0 |= (1 << TOIE0); // Enable overflow interrupt
}
