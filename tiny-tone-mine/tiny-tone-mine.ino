/* TinyTone for ATtiny85 */

int Speaker = 1;

// random range in minutes
//int rangeStart = 40; // inclusive
//int rangeEnd   = 80; // exclusive
// works with:
//   1, 2 
//   5, 6
//   10,11
//   20,21
// did not work with:
//  30,90
int rangeStart = 30;
int rangeEnd   = 36;

// 1 minute = 60000 miliseconds
int rangeMultiplier = 60000;
// 1 second = 1000 miliseconds
//int rangeMultiplier = 1000;

int toneDurationMs = 1000;

unsigned long lastToneStartMs = 0;
unsigned long silenceMs = 0;

void setup()
{
  pinMode(Speaker, OUTPUT);
}

void loop()
{

  unsigned long currentMs = millis();
  // the arduino site says millis resets overflows (goes to zero)
  // after about 50 days.
  if (currentMs < lastToneStartMs) {
    lastToneStartMs = 0;
  }

  if (currentMs - lastToneStartMs  > silenceMs) 
  {
    startTone();
    delay(toneDurationMs);  // let the tone sound for a bit
    TCCR1 = 0x90;           // stop tone (stop the counter)

    lastToneStartMs = currentMs;

    int randomInsideRange = random(rangeStart, rangeEnd);
    silenceMs = randomInsideRange * (long)rangeMultiplier;
    //silenceMs = randomInsideRange*1000;
    //silenceMs = 5000;
  }
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

