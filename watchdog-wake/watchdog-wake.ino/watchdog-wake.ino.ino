/*-----------------------------------------------------------------------*
 * millionOhms.cpp -- "Warning" Lights for ATtiny85/45.                  *
 * Flashes four LEDs, different speeds and patterns are selected by      *
 * pressing the SELECT button.  Sleeps after five minutes to conserve    *
 * battery power (2xAA).  A long press on the SELECT button will also    *
 * initiate sleep mode.                                                  *
 *                                                                       *
 * Set fuse bytes: L=0x62 H=0xDE E=0xFF                                  *
 * (BOD is set at 1.8V, system clock is 1MHz, from the internal 8MHz RC  *
 * oscillator divided by 8.  Low and Extended fuses remain at their      *
 * factory default values.)                                              *
 *                                                                       *
 * Jack Christensen 18Sep2011 v1                                         *
 * 18Jan2013 v2 Added random mode which uses the watchdog timer to wake  *
 *           the MCU and flash the LEDs at random intervals. Invoke      *
 *           random mode by pressing RESET and SELECT simultaneously,    *
 *           first release RESET, and then release SELECT. Press and     *
 *           release RESET alone to return to normal (manual) mode.     *
 *                                                                       *
 * This work is licensed under the Creative Commons Attribution-         *
 * ShareAlike 3.0 Unported License. To view a copy of this license,      *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a        *
 * letter to Creative Commons, 171 Second Street, Suite 300,             *
 * San Francisco, California, 94105, USA.                                *
 *-----------------------------------------------------------------------*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <Button.h>              //http://github.com/JChristensen/Button
#ifndef ARDUINO
#include <stdint.h>
#include "millionUtil.h"         //not needed if compiled with Arduino & Arduino-Tiny
#endif

#define RANDOM_RUN_MS 16000      //milliseconds to stay awake for in random mode (16 sec)
#define MANUAL_RUN_MS 300000     //milliseconds to stay awake for in manual mode (5 min)
#define RANDOM_SLEEP_MIN 73      //minimum number of 8-sec wdt periods to sleep for in random mode (~10 min)
#define RANDOM_SLEEP_MAX 220     //maximum number of 8-sec wdt periods to sleep for in random mode (~30 min)
#define LED1 1                   //PB1 LED1
#define LED2 0                   //PB0 LED2
#define LED3 3                   //PB3 LED3
#define LED4 4                   //PB4 LED4
#define BUTTON 2                 //PB2
#define delayBase 125U           //ms between changing LEDs
#define TACT_DEBOUNCE 25         //debounce time for tact switches in ms
#define LONG_PRESS 1000      //milliseconds for a long button press
#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR
#define nPATTERNS 8U             //number of display patterns
#define nSPEEDS 3U               //number of display speeds
#define MAX_MODE (nPATTERNS * nSPEEDS) - 1    //encode speeds and patterns into a single number

//function prototypes
void setup(void);
void loop(void);
void goToSleep(void);
void wdtEnable(void);
void wdtDisable(void);
void runLEDs(void);
void setPinsOutput(void);
void setPinsInput(void);

Button btn = Button(BUTTON, true, true, TACT_DEBOUNCE);    //instantiate the tact switch
uint8_t ledState, mode, mcucr1, mcucr2;
bool randomMode;                     //flag if we're in random wake mode (via WDT)
volatile bool buttonWake;            //flag whether the button woke us as opposed to the WDT
bool keepSleeping;                   //flag to keep sleeping or not in random mode
unsigned long msNow;                 //the current time from millis()
unsigned long msLast;                //the last time the LEDs were changed
unsigned long msWakeUp;              //the time we woke up
unsigned long msRunTime;             //how long to stay awake for
long wdtCount;                       //how many 8-sec WDT periods we've slept for
long wdtLimit;                       //number of WDT periods to wake after
uint8_t randLED;                     //for the random case

#ifndef ARDUINO
int main(void)               //main() not needed in Arduino environment
{
    setupUtil();
    setup();
    loop();
}
#endif

void setup(void)
{
    wdt_reset();
    wdtDisable();
    setPinsOutput();
    randomMode = btn.read();                //invoke random wake mode if button pressed
    while (btn.isPressed()) btn.read();     //wait for the button to be released
    msRunTime = randomMode ? RANDOM_RUN_MS : MANUAL_RUN_MS;
    wdtLimit = random(RANDOM_SLEEP_MIN, RANDOM_SLEEP_MAX + 1);
}

void loop(void)
{
    for (;;) {
        msNow = millis();
        btn.read();
        if (btn.wasReleased()) {
            if (++mode > MAX_MODE) mode = 0;
        }
        else if (btn.pressedFor(LONG_PRESS)) {
            setPinsInput();
            while (!btn.wasReleased()) btn.read();    //wait for the button to be released
            goToSleep();
        }
        else if (msNow - msWakeUp >= msRunTime) {
            setPinsInput();
            goToSleep();
        }
        runLEDs();
    }
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
        if (randomMode) wdtEnable();              //start the WDT if we're in random mode
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
        if (buttonWake) {              //what woke us
            keepSleeping = false;      //the button woke us, so turn the lights on again
            msRunTime = MANUAL_RUN_MS;
            btn.read();
            while (!btn.wasReleased()) btn.read();        //wait for the button to be released
        }
        else if (randomMode && (++wdtCount < wdtLimit)) {
            keepSleeping = true;
            //setPinsOutput();            //briefly blink an LED so we can see the wdt wake-ups
            //digitalWrite(LED4, HIGH);
            //_delay_ms(10);
            //setPinsInput();
        }
        else {
            keepSleeping = false;
            msRunTime = RANDOM_RUN_MS;
            if (++mode > MAX_MODE) mode = 0;
        }
    } while (keepSleeping);
    
    setPinsOutput();
    buttonWake = false;
    msWakeUp = millis();
    if (randomMode) {              //set up for next time
        wdtCount = 0;
        wdtLimit = random(RANDOM_SLEEP_MIN, RANDOM_SLEEP_MAX + 1);
    }
}

ISR(INT0_vect)
{
    buttonWake = true;            //flag that the button woke us up (and not the wdt)
    GIMSK = 0x00;                 //disable INT0
}    

ISR(WDT_vect) {}                  //don't need to do anything here when the WDT wakes the MCU

//enable the wdt for 8sec interrupt
void wdtEnable(void)
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);    //8192ms
    sei();
}

//disable the wdt
void wdtDisable(void)
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = 0x00;
    sei();
}

void runLEDs(void)
{
    if ( msNow - msLast >= delayBase << (mode % nSPEEDS)) {
        msLast = msNow;
        switch (mode / nSPEEDS) {

            case 0:                        // o  -    -  o
                switch (ledState % 2) {    // -  o    o  -
                    case 0:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED4, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED3, HIGH);
                        break;
                }
                break;

            case 1:                        // o  -    -  o    -  -    -  -
                switch (ledState) {        // -  -    -  -    o  -    -  o
                    case 0:
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED1, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        break;
                    case 2:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, HIGH);
                        break;
                    case 3:
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, HIGH);
                        break;
                }
                break;

            case 2:                        // o  o    -  -
                switch (ledState % 2) {    // -  -    o  o
                    case 0:
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, HIGH);
                        digitalWrite(LED4, HIGH);
                        break;
                }
                break;

            case 3:                        // o  -    -  o    -  -    -  -
                switch (ledState) {        // -  -    -  -    -  o    o  -
                    case 0:
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED1, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        break;
                    case 2:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED4, HIGH);
                        break;
                    case 3:
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED3, HIGH);
                        break;
                }
                break;

            case 4:                        // o  o    -  -
                switch (ledState % 2) {    // o  o    -  -
                    case 0:
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED3, HIGH);
                        digitalWrite(LED4, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, LOW);
                        break;
                }
                break;

            case 5:                        // o  -    -  -    -  -    -  o
                switch (ledState) {        // -  -    o  -    -  o    -  -
                    case 0:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED1, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED3, HIGH);
                        break;
                    case 2:
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED4, HIGH);
                        break;
                    case 3:
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED2, HIGH);
                        break;
                }
                break;

            case 6:                        // o  -    -  o
                switch (ledState % 2) {    // o  -    -  o
                    case 0:
                        digitalWrite(LED2, LOW);
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED3, HIGH);
                        break;
                    case 1:
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED3, LOW);
                        digitalWrite(LED2, HIGH);
                        digitalWrite(LED4, HIGH);
                        break;
                }
                break;

            case 7:                            //random
                randLED = random(0, 4);        //a number between 0 and 3
                if (randLED > 1) ++randLED;    //maps 0,1,2,3 to 0, 1, 3, 4
                digitalWrite(LED1, LOW);
                digitalWrite(LED2, LOW);
                digitalWrite(LED3, LOW);
                digitalWrite(LED4, LOW);
                digitalWrite(randLED, HIGH);
                break;
        }
        if (++ledState > 3) ledState = 0;
    }
}

void setPinsOutput(void)
{
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
}

void setPinsInput(void)
{
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
    pinMode(LED1, INPUT);
    pinMode(LED2, INPUT);
    pinMode(LED3, INPUT);
    pinMode(LED4, INPUT);
}
