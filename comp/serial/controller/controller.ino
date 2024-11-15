#include <LowPower.h>

#define LED 13
#define COMP_POWER_OFF_COMMAND 7
#define COMP_STATUS 8
#define SEND_PERIOD 700
#define WAIT_COMP_PERIOD 120000
#define WAIT_COMP_ERROR 1000
#define COMP_DEAD_ERROR 250
#define COMP_OFF_COMMAND_AFTER_START_ERROR 500
#define BLINK_ERROR_TIME 600000

unsigned long _timer;
bool _receivePowerOffSignal = false;

/*
  Сигналы COMP_POWER_OFF_COMMAND и COMP_STATUS инверсные
*/

void setup()
{
  pinMode(COMP_POWER_OFF_COMMAND, INPUT_PULLUP);
  pinMode(COMP_STATUS, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  digitalWrite(COMP_STATUS, LOW);

  if (!digitalRead(COMP_POWER_OFF_COMMAND)) blink(COMP_OFF_COMMAND_AFTER_START_ERROR);

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
}

void loop()
{
  if (millis() - _timer > SEND_PERIOD)
  {
    if (_receivePowerOffSignal) Serial.print('-');
    if (Serial.available())
    {
      if (Serial.read() != '+') ifCompDead();
    }
    else
    {
      ifCompDead();
    }
    _timer = millis();
  }
  if (!digitalRead(COMP_POWER_OFF_COMMAND))
  {
    delay(500);
    if (!digitalRead(COMP_POWER_OFF_COMMAND)) _receivePowerOffSignal = true;
  }
}

void ifCompDead()
{
  pinMode(COMP_STATUS, INPUT_PULLUP);
  if (_receivePowerOffSignal)
  {
    sleep();
  }
  else
  {
    blink(COMP_DEAD_ERROR);
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
  if (period > 0)
  {
    while (true)
    {
      digitalWrite(LED, LOW);
      delay(period);
      digitalWrite(LED, HIGH);
      delay(period);
      if (millis() - timer >= BLINK_ERROR_TIME) sleep();
    }
  }
  else
  {
    while (true)
    {
      if (millis() - timer >= BLINK_ERROR_TIME) sleep();
    }
  }
}