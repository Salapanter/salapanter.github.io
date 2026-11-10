#include <Wire.h>
#include <avr/sleep.h>

#define SLAVE_ADDRESS 0x08

const byte buttonPin = 2;      // D2 = INT0

const byte criticalLed = 8;
const byte normalLed   = 9;
const byte goodLed     = 10;

volatile bool buttonPressed = false;

enum Status
{
  CRITICAL = 1,
  NORMAL   = 2,
  GOOD     = 3
};

unsigned long ledStartTime = 0;
bool ledActive = false;

//--------------------------------------------------

void wakeISR()
{
  buttonPressed = true;
}

//--------------------------------------------------

void turnOffLeds()
{
  digitalWrite(criticalLed, LOW);
  digitalWrite(normalLed, LOW);
  digitalWrite(goodLed, LOW);
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

byte requestStatusFromSlave()
{
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write('R');
  Wire.endTransmission();

  delay(600);

  Wire.requestFrom(SLAVE_ADDRESS, 1);

  if (Wire.available())
  {
    return Wire.read();
  }

  return 0;
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
  Wire.begin();

  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(criticalLed, OUTPUT);
  pinMode(normalLed, OUTPUT);
  pinMode(goodLed, OUTPUT);

  turnOffLeds();

  attachInterrupt(
    digitalPinToInterrupt(buttonPin),
    wakeISR,
    FALLING);
}

//--------------------------------------------------

void loop()
{
  if (buttonPressed)
  {
    buttonPressed = false;

    byte status = requestStatusFromSlave();

    if (status != 0)
    {
      showStatus(status);
    }
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