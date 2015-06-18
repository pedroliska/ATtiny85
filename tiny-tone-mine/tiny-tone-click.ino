/* TinyTone for ATtiny85 */

int Speaker = 1;

// random range in minutes
//int rangeStart = 30;
//int rangeEnd   = 60;
int rangeStart = 5;
int rangeEnd   = 10;

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
    silenceMs = 4000;

    //delay(1000);
  }

  

  
  //int randomMinutes = random(rangeStart, rangeEnd);
  //// 1 minute = 3600 miliseconds
  //delay(randomMinutes * 3600);
}

void startTone() {
  //TCCR1 = 0x94; // faster clock
  //TCCR1 = 0x99;   
  //TCCR1 = 0x9F;   // slower clock (max value)

  //OCR1C = 4; // highest pitch (hard to listen)
  //OCR1C = 5;
  //OCR1C = 200;
  //OCR1C = 255; // lowest pitch (max value)

  // hi pitch combo
  //TCCR1 = 0x94;
  //OCR1C = 4;

  // old phone combo
  //TCCR1 = 0x99;   
  //OCR1C = 255;

  // audible combo
  TCCR1 = 0x94;
  OCR1C = 20;
}

