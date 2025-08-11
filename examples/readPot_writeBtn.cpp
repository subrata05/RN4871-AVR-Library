/*
 * readPot_writeBtn.cpp
 *
 * Created: 09-07-2025 11:29:33
 * Author: Subrata
 * Description: Example program demonstrating the use of the RN4871 BLE library
 *              on the ATmega328PB microcontroller. Configures a BLE service with
 *              one readable characteristic for analog input and one writable
 *              characteristic for LED control.
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "rn4871.h"
#include "bleSerial.h"
#include "wiring.h"

#define F_CPU 8000000UL // 8 MHz clock frequency

// LED pin definitions
#define LED_PIN1 PIND6
#define LED_PIN2 PIND5
#define LED_PIN3 PIND4
#define LED_DDR DDRD
#define LED_PORT PORTD

// Lock state enumeration
typedef enum {
    LOCKED,
    UNLOCKED
} lock_state_t;

lock_state_t lock_state = LOCKED;

// BLE service and characteristic definitions
const char* myDeviceName = "Avocado";
const char* myServiceUUID = "AD11CF40063F11E5BE3E0002A5D5C51B";
const char* potCharUUID = "AD11CF40163F11E5BE3E0002A5D5C51B";
const uint16_t potCharLen = 4;
uint16_t potHandle;
char potPayload[potCharLen * 4 + 1];

const char* toggleLedCharUUID = "AD11CF40363F11E5BE3E0002A5D5C51B";
const uint8_t toggleLedCharLen = 1;
uint16_t toggleHandle;

uint16_t analogRead(uint8_t pin) {
    DDRC &= ~(1 << pin); // Set pin as input
    DIDR0 |= (1 << pin); // Disable digital input buffer
    ADMUX = (1 << REFS0); // Reference voltage AVcc
    if (pin > 5) return 0; // Invalid pin check
    ADMUX &= ~0x0F; // Clear previous channel
    ADMUX |= (pin & 0x0F); // Select ADC channel
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, prescaler 128
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for completion
    return ADC;
}

void timer_for_10ms(void) {
    cli(); // Disable interrupts
    TCCR1A = 0; // Clear Timer1 control registers
    TCCR1B = 0;
    TCNT1 = 0; // Clear counter
    OCR1A = 1249; // Set compare value for 10ms at 8MHz with prescaler 64
    TCCR1B |= (1 << WGM12); // CTC mode
    TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
    TIMSK1 |= (1 << OCIE1A); // Enable compare interrupt
    sei(); // Enable interrupts
}

ISR(TIMER1_COMPA_vect) {
    lock_state = (lock_state == LOCKED) ? UNLOCKED : LOCKED;
}

int main(void) {
    // Initialize peripherals
    bleInit();
    initMillis();
    timer_for_10ms();
    LED_DDR |= (1 << LED_PIN1) | (1 << LED_PIN2) | (1 << LED_PIN3); // Set LED pins as output
    LED_PORT &= ~((1 << LED_PIN1) | (1 << LED_PIN2) | (1 << LED_PIN3)); // Turn LEDs off

    // Initialize RN4871 BLE module
    hwInit(7, &DDRD, &PORTD);
    if (!swInit()) {
        while (1); // Halt on initialization failure
    }

    // Configure BLE services
    enterCommandMode();
    stopAdvertising();
    clearAllServices();
    setSerializedName(myDeviceName);
    setServiceUUID(myServiceUUID);
    setCharactUUID(potCharUUID, READ_PROPERTY, potCharLen);
    setCharactUUID(toggleLedCharUUID, WRITE_PROPERTY, toggleLedCharLen);

    // Find characteristic handles
    char handleStr[5];
    potHandle = findHandle(potCharUUID, READ_PROPERTY);
    toggleHandle = findHandle(toggleLedCharUUID, WRITE_PROPERTY);

    // Reboot and start advertising
    if (reboot() && enterCommandMode()) {
        startCustomAdvertising(200);
        setAdvPower(0);
    }

    while (1) {
        if (getConnectionStatus() == 1) { // Connected
            // Read and send analog value
            uint16_t analogValue = analogRead(0); // Read from PC0
            sprintf(potPayload, "%04X", analogValue); // Format as 4-digit hex
            writeLocalCharacteristic(potHandle, potPayload);

            // Check for LED control command
            if (lock_state == UNLOCKED && readLocalCharacteristic(toggleHandle)) {
                const char* resp = getLastResponse();
                char* number_str = strstr(resp, "CMD> ") + 5; // Extract value after "CMD> "
                uint8_t val = (uint8_t)strtol(number_str, NULL, 16);
                // Control LEDs based on received value
                LED_PORT &= ~((1 << LED_PIN1) | (1 << LED_PIN2) | (1 << LED_PIN3)); // Clear LEDs
                if (val == 0x05) {
                    LED_PORT |= (1 << LED_PIN1); // Turn on LED1
                } else if (val == 0x06) {
                    LED_PORT |= (1 << LED_PIN2); // Turn on LED2
                } else if (val == 0x07) {
                    LED_PORT |= (1 << LED_PIN3); // Turn on LED3
                }
                lock_state = LOCKED; // Reset lock state
            }
            _delay_ms(20); // Small delay to prevent flooding
        } else {
            _delay_ms(300); // Wait longer when disconnected
        }
    }

    return 0;
}
