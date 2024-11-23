#include <LowPower.h>

#define LED 13
#define COMP_POWER_OFF_COMMAND 2
#define COMP_STATUS 3
#define SEND_PERIOD 700
#define WAIT_COMP_PERIOD 120000
#define WAIT_COMP_ERROR 1000
#define COMP_DEAD_ERROR 250
#define COMP_OFF_COMMAND_AFTER_START_ERROR 500
#define BLINK_ERROR_TIME 600000

unsigned long _timer;
bool _receivePowerOffSignal = false;
volatile bool _resetTimer = true;
bool _needSleep = false;

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
      Serial.print('a');
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
    if (millis() - timerForWait >= WAIT_COMP_PERIOD)
    {
      blink(WAIT_COMP_ERROR);
    }
  }
  _timer = millis();
}

void onCompPowerOffCommand()
{
  detachInterrupt(digitalPinToInterrupt(COMP_POWER_OFF_COMMAND));
  _resetTimer = true;
}

void loop()
{
  if (_receivePowerOffSignal && millis() - _timer > SEND_PERIOD)
  {
    Serial.print('-');
    _resetTimer = true;
  }
  if (!_receivePowerOffSignal)
  {
    if (digitalRead(COMP_POWER_OFF_COMMAND))
    {
      if ((millis() - _timer) >= 100) _receivePowerOffSignal = true;
    }
    else
    {
      attachInterrupt(digitalPinToInterrupt(COMP_POWER_OFF_COMMAND), onCompPowerOffCommand, HIGH);
      _needSleep = true;

    }
  }
  if (_resetTimer)
  {
    _timer = millis();
    _resetTimer = false;
  }
  if (_needSleep)
  {
    _needSleep = false;
    sleep();
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