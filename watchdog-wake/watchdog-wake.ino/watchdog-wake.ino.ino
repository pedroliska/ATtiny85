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

int PIN = 1;
int REGULAR_LIGHT_MS = 3000;
int WAKE_INDICATOR_LIGHT_MS = 1000;

//int RANDOM_SLEEP_MIN = 73;      //minimum number of 8-sec wdt periods to sleep for in random mode (~10 min)
//int RANDOM_SLEEP_MAX = 220;     //maximum number of 8-sec wdt periods to sleep for in random mode (~30 min)
int RANDOM_SLEEP_MIN = 1;
int RANDOM_SLEEP_MAX = 2;

uint8_t ledState, mode, mcucr1, mcucr2;
bool keepSleeping;                   //flag to keep sleeping or not in random mode
unsigned long msNow;                 //the current time from millis()
unsigned long msLast;                //the last time the LEDs were changed
unsigned long msWakeUp;              //the time we woke up
unsigned long msRunTime;             //how long to stay awake for
long wdtCount;                       //how many 8-sec WDT periods we've slept for
long wdtLimit;                       //number of WDT periods to wake after

void setup() {
//  wdt_reset();
//  wdtDisable();
  setPinsOutput();
}

void loop() {
//  msNow = millis();
//  goToSleep();
  blinkLed(REGULAR_LIGHT_MS);
  delay(1000);
}

void setPinsOutput(void)
{
    pinMode(PIN, OUTPUT);
}

void setPinsInput(void)
{
    digitalWrite(PIN, LOW);
    pinMode(PIN, INPUT);
}

void blinkLed (int msOfLight) {
  setPinsOutput();            //briefly blink an LED so we can see the wdt wake-ups
  digitalWrite(PIN, HIGH);
  delay(msOfLight);
  setPinsInput();
}

void goToSleep(void)
{
    do {
        GIMSK |= _BV(INT0);                       //enable INT0
        MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level
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
        GIMSK = 0x00;                  //disable INT0
        sleep_disable();
        wdtDisable();                  //don't need the watchdog while we're awake
        sei();                         //enable interrupts again (but INT0 is disabled above)

        if (++wdtCount < wdtLimit) {
            keepSleeping = true;
            blinkLed(WAKE_INDICATOR_LIGHT_MS);            //briefly blink an LED so we can see the wdt wake-ups
        }
        else {
            keepSleeping = false;
        }
    } while (keepSleeping);
    
    setPinsOutput();
    msWakeUp = millis();

    wdtCount = 0;     //set up for next time
    wdtLimit = random(RANDOM_SLEEP_MIN, RANDOM_SLEEP_MAX + 1);
}

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


