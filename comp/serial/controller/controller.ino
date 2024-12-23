#include <LowPower.h>

#define LED 13
#define COMP_POWER_OFF_COMMAND 2
#define COMP_STATUS 3
#define SEND_PERIOD 700
#define WAIT_COMP_AFTER_START_TIME 180000
#define WAIT_COMP_ERROR 1000
#define COMP_OFF_COMMAND_AFTER_START_ERROR 500
#define BLINK_ERROR_TIME 600000

unsigned long _timer;
bool _receivePowerOffSignal = false;
bool _needSleep = false;
bool _afterHighEvent = false;
bool _afterLowEvent = true;
volatile bool _ifInterruptIsAttached = false;

/*
  Сигнал COMP_STATUS инверсный
*/

void setup()
{
  pinMode(COMP_POWER_OFF_COMMAND, INPUT);
  pinMode(COMP_STATUS, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  digitalWrite(COMP_STATUS, HIGH);

  if (digitalRead(COMP_POWER_OFF_COMMAND)) blink(COMP_OFF_COMMAND_AFTER_START_ERROR);

  unsigned long timerForSend = millis();
  unsigned long timerForWait = millis();
  while (true)
  {
    if (millis() - timerForSend >= SEND_PERIOD)
    {
      Serial.println('a');
      timerForSend = millis();
    }
    if (Serial.available())
    {
      if (Serial.read() == '+')
      {
        digitalWrite(COMP_STATUS, LOW);
        _timer = millis();
        break;
      }
    }
    if (millis() - timerForWait >= WAIT_COMP_AFTER_START_TIME)
    {
      blink(WAIT_COMP_ERROR);
    }
  }
  _timer = millis();
}

void onCompPowerOffCommand()
{

}

void loop()
{
  if (_ifInterruptIsAttached) detachInterrupt(digitalPinToInterrupt(COMP_POWER_OFF_COMMAND));
  if (_receivePowerOffSignal)
  {
    if (millis() - _timer >= SEND_PERIOD)
    {
      Serial.println('-');
      _timer = millis();
    }
  }
  else
  {
    if (digitalRead(COMP_POWER_OFF_COMMAND))
    {
      if (_afterLowEvent)
      {
        _afterLowEvent = false;
        _afterHighEvent = true;
        _timer = millis();
      }
      if ((millis() - _timer) >= 100)
      {
        _receivePowerOffSignal = true;
        _timer = millis() + SEND_PERIOD;
      }
    }
    else
    {
      if (_afterHighEvent)
      {
        _afterHighEvent = false;
        _afterLowEvent = true;
        _timer = millis();
      }
      if (!_ifInterruptIsAttached && (millis() - _timer) >= 100)
      {
        attachInterrupt(digitalPinToInterrupt(COMP_POWER_OFF_COMMAND), onCompPowerOffCommand, HIGH);
        _ifInterruptIsAttached = true;
        sleep();
      }
    }
  }
}

void sleep()
{
  digitalWrite(LED, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void blink(int period)
{
  digitalWrite(LED, HIGH);
  delay(period);
  unsigned long timer = millis();
  while (true)
  {
    digitalWrite(LED, LOW);
    delay(period);
    digitalWrite(LED, HIGH);
    delay(period);
    if (millis() - timer >= BLINK_ERROR_TIME) sleep();
  }
}