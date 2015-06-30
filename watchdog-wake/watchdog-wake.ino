#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#ifndef ARDUINO
#include <stdint.h>
#include "millionUtil.h"         //not needed if compiled with Arduino & Arduino-Tiny
#endif

#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR

int PIN = 1;
int REGULAR_HI_MS = 1000;
int WAKE_INDICATOR_HI_MS = 200;
//int WAKE_INDICATOR_HI_MS = 0;

//int RANDOM_SLEEP_MIN = 73;  // minimum number of 8-sec WDT periods to sleep for (~10 min)
//int RANDOM_SLEEP_MAX = 220; // maximum number of 8-sec WDT periods to sleep for (~30 min)
//int RANDOM_SLEEP_MIN = 1;
//int RANDOM_SLEEP_MAX = 2;
//int RANDOM_SLEEP_MIN = 38;  //5 mins
//int RANDOM_SLEEP_MAX = 38;  //5 mins
//int RANDOM_SLEEP_MIN = 8;   //1 min
//int RANDOM_SLEEP_MAX = 8;   //1 min
//int RANDOM_SLEEP_MIN = 75;  // 10 mins (10 * 60 / 8)
//int RANDOM_SLEEP_MAX = 112; // 15 mins
int RANDOM_SLEEP_MIN = 675;  // 90 mins
int RANDOM_SLEEP_MAX = 675;  // 90 mins

uint8_t mcucr1, mcucr2;
bool keepSleeping;                   //flag to keep sleeping or not
unsigned long msNow;                 //the current time from millis()
unsigned long msWakeUp;              //the time we woke up
long wdtCount;                       //how many 8-sec WDT periods we've slept for
long wdtLimit;                       //number of WDT periods to wake after

void setup() { }

void loop() {
  makeTone(REGULAR_HI_MS);
  goToSleep();
}

void makeTone(int msOfTone) {
  pinMode(PIN, OUTPUT);
  startTone();
  delay(msOfTone);  // let the tone sound for a bit
  stopTone();
  pinMode(PIN, INPUT);
}
void startTone() {
  //TCCR1 = 0x94; // faster clock
  //TCCR1 = 0x99;   
  //TCCR1 = 0x9F;   // slower clock (max value)

  //OCR1C = 4; // highest pitch (hard to listen)
  //OCR1C = 5;
  //OCR1C = 200;
  //OCR1C = 255; // lowest pitch (max value)

  // hi pitch combo (perfect with 3v no resistor, and no transistor)
//  TCCR1 = 0x94;
//  OCR1C = 4;

  // old phone combo (too silent with 3v, no resistor, and no transistor)
//  TCCR1 = 0x99;   
//  OCR1C = 255;

  // audible combo
  TCCR1 = 0x94;
  OCR1C = 20;
}
void stopTone() {
  TCCR1 = 0x90;           // stop the counter
}


void goToSleep(void)
{
    msNow = millis();
    wdtCount = 0;
    wdtLimit = random(RANDOM_SLEEP_MIN, RANDOM_SLEEP_MAX + 1);
    
    do {
        ACSR |= _BV(ACD);                         //disable the analog comparator
        ADCSRA &= ~_BV(ADEN);                     //disable ADC
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        
        wdtEnable();              //start the WDT
        
        //turn off the brown-out detector.
        //must have an ATtiny45 or ATtiny85 rev C or later for software to be able to disable the BOD.
        //current while sleeping will be <0.5uA if BOD is disabled, <25uA if not.
        cli();
        mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
        mcucr2 = mcucr1 & ~_BV(BODSE);
        MCUCR = mcucr1;
        MCUCR = mcucr2;
        sei();                         //ensure interrupts enabled so we can wake up again
        sleep_cpu();                   //go to sleep
                                       //----zzzz----zzzz----zzzz----zzzz
        cli();                         //wake up here, disable interrupts
        sleep_disable();
        wdtDisable();                  //don't need the watchdog while we're awake
        sei();                         //enable interrupts again (but INT0 is disabled above)

        if (++wdtCount < wdtLimit) {
            keepSleeping = true;
            if (WAKE_INDICATOR_HI_MS > 0) {
              makeTone(WAKE_INDICATOR_HI_MS);            //briefly blink an LED so we can see the wdt wake-ups
            }
        }
        else {
            keepSleeping = false;
        }
    } while (keepSleeping);
    
    msWakeUp = millis();
}

ISR(WDT_vect) {}                  //don't need to do anything here when the WDT wakes the MCU

//enable the WDT for 8sec interrupt
void wdtEnable(void)
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);    //8192ms
    sei();
}


//disable the WDT
void wdtDisable(void)
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = 0x00;
    sei();
}


