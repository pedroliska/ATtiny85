int PIN = 1;
int toneDurationMs = 500;
int silenceMs = 2000;
int pulseLength = 4;

void setup()
{
  pinMode(PIN, OUTPUT);
}

void loop()
{
    startTone(pulseLength);
    delay(toneDurationMs);  // let the tone sound for a bit
    TCCR1 = 0x90;           // stop tone (stop the counter)
    pulseLength += 1;
    delay(silenceMs);
}

void startTone(int pulseLength) {
  //TCCR1 = 0x94; // faster clock
  //TCCR1 = 0x99;   
  //TCCR1 = 0x9F;   // slower clock (max value)

  //OCR1C = 4; // highest pitch (hard to listen)
  //OCR1C = 5;
  //OCR1C = 200;
  //OCR1C = 255; // lowest pitch (max value)

//  // hi pitch combo (perfect with 3v no resistor, and no transistor)
//  TCCR1 = 0x94;
//  OCR1C = 4;

//  // old phone combo (too silent with 3v, no resistor, and no transistor)
//  TCCR1 = 0x99;   
//  OCR1C = 255;

//  // audible combo
//  TCCR1 = 0x94;
//  OCR1C = 20;

  TCCR1 = 0x94;
  OCR1C = pulseLength;
  
}

