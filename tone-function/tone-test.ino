int blinkPin = 1;

void setup()
{
  pinMode(blinkPin, OUTPUT);
}

void loop()
{
  
  //pin:       the pin on which to generate the tone
  //frequency: the frequency of the tone in hertz - unsigned int
  //duration:  the duration of the tone in milliseconds (optional) - unsigned long
  tone(blinkPin, 1000, 5000);
  noTone(blinkPin);
  delay(200);
}

