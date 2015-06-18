int piezoPin = 1;
int delayMs = 100;

void setup()
{
  pinMode(piezoPin, OUTPUT);
}

void loop()
{ 
  digitalWrite(piezoPin, HIGH);
  delay(delayMs);
  digitalWrite(piezoPin, LOW);
  delay(delayMs);
}

