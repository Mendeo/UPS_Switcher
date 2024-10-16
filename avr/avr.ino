#include <LowPower.h>

#define LED 13
#define POWER_UPS 10
#define POWER_BAT 11
#define LINE_STATUS 12

#define COMP_NOT_READY_1 500
#define COMP_NOT_READY_2 1000
#define LOW_BATTERY 3000

#define TIME_FOR_COMP_POWEROFF 60000
#define TIME_FOR_UPS_POWEROFF_AFTER_COMP 60000
#define TIME_BEFORE_COMP_POWERON 30000

volatile unsigned long _eventTime;
bool _compIsOn = true;

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(POWER_UPS, OUTPUT);
  pinMode(POWER_BAT, OUTPUT);
  pinMode(LINE_STATUS, INPUT);

  switchToMainPower();
  if (!lineIsOk()) blink(-1);
  Serial.begin(9600);
  Serial.println("status");
  delay(1000);
  if (!Serial.available()) blink(COMP_NOT_READY_1);
  if (Serial.readString() != "ok") blink(COMP_NOT_READY_2);
  attachInterruptToLineDown();
  sleep();
}

void onLineDown()
{
  detachInterrupt(digitalPinToInterrupt(LINE_STATUS));
  _eventTime = millis();
}
void onLineUp()
{
  detachInterrupt(digitalPinToInterrupt(LINE_STATUS));
  _eventTime = millis();
}

void loop()
{
  if (_compIsOn && !lineIsOk() && (millis() - _eventTime) >= TIME_FOR_COMP_POWEROFF)
  {
    Serial.println("poweroff");
    waitForCompPowerOff();
    _eventTime = millis();
    _compIsOn = false;
  }
  else if (!_compIsOn && !lineIsOk() && (millis() - _eventTime) >= TIME_FOR_UPS_POWEROFF_AFTER_COMP)
  {
    switchToBatteryPower();
    switchUPS();
    attachInterruptToLineUp();
    sleep();
  }
  else if (!_compIsOn && lineIsOk() && (millis() - _eventTime) >= TIME_BEFORE_COMP_POWERON)
  {
    switchUPS();
    switchToMainPower();
    _compIsOn = true;
    waitForCompPowerOn();
    attachInterruptToLineDown();
    sleep();
  }
}

void switchUPS()
{

}

void sleep()
{
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void waitForCompPowerOff()
{
  while (true)
  {
    while (Serial.available())
    {
      Serial.read();
    }
    delay(500);
    if (!Serial.available()) break;
  }
}

void waitForCompPowerOn()
{
  unsigned long time = millis();
  while (!Serial.available())
  {
    if (millis() - time > 180000) break; //Ждём не больше 3-х минут.
  }
  while (Serial.available())
  {
    Serial.read();
  }
}

void attachInterruptToLineDown()
{
  attachInterrupt(digitalPinToInterrupt(LINE_STATUS), onLineDown, FALLING);
}
void attachInterruptToLineUp()
{
  attachInterrupt(digitalPinToInterrupt(LINE_STATUS), onLineDown, RISING);
}

bool lineIsOk()
{
  return digitalRead(LINE_STATUS);
}

void switchToMainPower()
{
  digitalWrite(POWER_BAT, LOW);
  delay(2);
  digitalWrite(POWER_UPS, HIGH);
}

void switchToBatteryPower()
{
  digitalWrite(POWER_UPS, LOW);
  delay(2);
  digitalWrite(POWER_BAT, HIGH);
}

void blink(int period)
{
  digitalWrite(LED, HIGH);
  delay(period);
  if (period > 0)
  {
    while (true)
    {
      digitalWrite(LED, LOW);
      delay(period);
      digitalWrite(LED, HIGH);
      delay(period);
    }
  }
  else
  {
    while (true) {}
  }
}