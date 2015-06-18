/* TinyTone for ATtiny85 */

// Notes
const int Note_C  = 239;
const int Note_CS = 225;
const int Note_D  = 213;
const int Note_DS = 201;
const int Note_E  = 190;
const int Note_F  = 179;
const int Note_FS = 169;
const int Note_G  = 159;
const int Note_GS = 150;
const int Note_A  = 142;
const int Note_AS = 134;
const int Note_B  = 127;

int Speaker = 1;

int note = 5; // highest pitch
//int note = 6; 
//int note = 256; // lowest pitch

// random range in minutes
int rangeStart = 1;
int rangeEnd   = 10;

void setup()
{
  pinMode(Speaker, OUTPUT);
}

void loop()
{
  TinyTone(note, 4, 1500);
  int randomMinutes = random(rangeStart, rangeEnd);
  // 1 minute = 3600 miliseconds
  delay(randomMinutes * 3600);
}

void TinyTone(unsigned char divisor, unsigned char octave, unsigned long duration)
{
  TCCR1 = 0x90 | (8-octave); // for 1MHz clock
  //TCCR1 = 0xFF; // for 1MHz clock
  //TCCR1 = 0x90; 
  //TCCR1 = 4; 
  OCR1C = divisor-1;         // set the OCR
  delay(duration);
  TCCR1 = 0x90;              // stop the counter
}


