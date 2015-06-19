/* TinyTone for ATtiny85 */

int Speaker = 1;

// random range in minutes
//int rangeStart = 40; // inclusive
//int rangeEnd   = 80; // exclusive
// works with:
//   1, 2 
//   10,11
int rangeStart = 10;
int rangeEnd   = 11;

unsigned long lastStopToneMs = 0;
unsigned long silenceMs = 0;

void setup()
{
  pinMode(Speaker, OUTPUT);
}

void loop()
{

  unsigned long currentMs = millis();

  if (currentMs - lastStopToneMs > silenceMs) 
  {
    startTone();
    delay(1500);    // let the tone sound for a bit
    TCCR1 = 0x90;   // stop the counter (stop tone)

    lastStopToneMs = currentMs;

    int randomInsideRange = random(rangeStart, rangeEnd);
    // 1 minute = 60000 miliseconds
    silenceMs = randomInsideRange*60000;
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

