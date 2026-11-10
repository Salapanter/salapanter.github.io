#include <Wire.h>
#include <avr/sleep.h>

#define I2C_ADDRESS 0x08

const byte buttonPin = 2;      // D2 = INT0

const byte criticalLed = 8;    // Rod  - torrt
const byte normalLed   = 9;    // Gul  - fuktig
const byte goodLed     = 10;   // Gronn - vatt

const byte sensorPin = A0;

volatile bool localButtonPressed = false;
volatile bool measureRequested   = false;  // satt fra I2C interrupt

byte lastStatus = 0;

enum Status
{
  CRITICAL = 1,   // Torrt
  NORMAL   = 2,   // Fuktig
  GOOD     = 3    // Vatt
};

unsigned long ledStartTime = 0;
bool ledActive = false;

//--------------------------------------------------

void wakeISR()
{
  localButtonPressed = true;
}

//--------------------------------------------------

// Tar 5 malinger og returnerer snittet som raaverdi
int lesRaaverdi()
{
  const int ANTALL_MALINGER = 5;
  const int PAUSE_MS        = 100;

  long sum = 0;
  for (int i = 0; i < ANTALL_MALINGER; i++)
  {
    sum += analogRead(sensorPin);
    delay(PAUSE_MS);
  }
  int snitt = sum / ANTALL_MALINGER;

  Serial.println(snitt);

  return snitt;
}

//--------------------------------------------------

byte evaluateSensor()
{
  int raaverdi = lesRaaverdi();

  if (raaverdi > 430)
    return CRITICAL;   // Torrt  (> 430)

  if (raaverdi >= 230)
    return NORMAL;     // Greit  (230 - 430)

  return GOOD;         // Vatt   (< 230)
}

//--------------------------------------------------

void turnOffLeds()
{
  digitalWrite(criticalLed, LOW);
  digitalWrite(normalLed,   LOW);
  digitalWrite(goodLed,     LOW);
}

//--------------------------------------------------

void showStatus(byte status)
{
  turnOffLeds();

  switch (status)
  {
    case CRITICAL:
      digitalWrite(criticalLed, HIGH);
      break;

    case NORMAL:
      digitalWrite(normalLed, HIGH);
      break;

    case GOOD:
      digitalWrite(goodLed, HIGH);
      break;
  }

  ledStartTime = millis();
  ledActive = true;
}

//--------------------------------------------------

void performMeasurement()
{
  lastStatus = evaluateSensor();
  showStatus(lastStatus);
}

//--------------------------------------------------

// Kalles fra I2C interrupt -- kun sett flagg, ingen analogRead/millis her
void receiveEvent(int bytesReceived)
{
  if (bytesReceived > 0)
  {
    char cmd = Wire.read();

    if (cmd == 'R')
    {
      measureRequested = true;
    }
  }
}

//--------------------------------------------------

void requestEvent()
{
  Wire.write(lastStatus);
}

//--------------------------------------------------

void enterIdleSleep()
{
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_mode();
  sleep_disable();
}

//--------------------------------------------------

void setup()
{
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(criticalLed, OUTPUT);
  pinMode(normalLed,   OUTPUT);
  pinMode(goodLed,     OUTPUT);

  turnOffLeds();

  attachInterrupt(
    digitalPinToInterrupt(buttonPin),
    wakeISR,
    FALLING);

  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

//--------------------------------------------------

void loop()
{
  if (localButtonPressed)
  {
    localButtonPressed = false;
    performMeasurement();
  }

  if (measureRequested)
  {
    measureRequested = false;
    performMeasurement();
  }

  if (ledActive && millis() - ledStartTime >= 5000)
  {
    turnOffLeds();
    ledActive = false;
  }

  if (!ledActive)
  {
    enterIdleSleep();
  }
}
