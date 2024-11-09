#include <LowPower.h>
#include <Servo.h>

#define INTERNAL_REF_REAL_VOLTAGE 1.1

#define LED 13
#define LINE_STATUS 2
#define BAT_5V_REGULATOR 10
#define SERVO_POWER 7
#define SERVO_CONTROL 4
#define COMP_POWER_OFF_COMMAND 5
#define COMP_STATUS 6
#define MAIN_POWER_STATUS 3

#define COMP_NOT_READY_1_ERROR 500
#define LOW_BATTERY_ERROR 3000
#define SWITCHING_POWER_ERROR 1000

#define TIME_FOR_COMP_POWEROFF_AFTER_LINE_DOWN 10000
#define TIME_FOR_UPS_POWEROFF_AFTER_COMP 10000
#define TIME_BEFORE_COMP_POWERON 10000
#define TIME_BEFORE_DEEP_SLEEP 10000

volatile unsigned long _eventTime;
bool _compIsOn = true;
bool _isMainPower = true;
Servo _srv;
volatile bool _lineIsOk = true;

void setup()
{
  pinMode(LINE_STATUS, INPUT);
  pinMode(MAIN_POWER_STATUS, INPUT);
  pinMode(COMP_STATUS, INPUT);

  pinMode(LED, OUTPUT);
  pinMode(SERVO_POWER, OUTPUT);
  pinMode(COMP_POWER_OFF_COMMAND, OUTPUT);
  pinMode(BAT_5V_REGULATOR, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(BAT_5V_REGULATOR, HIGH);

  setServoToZero();

  compPowerOn();
  if (!digitalRead(MAIN_POWER_STATUS)) blink(SWITCHING_POWER_ERROR);
  digitalWrite(BAT_5V_REGULATOR, LOW);
  checkLine();
  if (!_lineIsOk) blink(-1);
  delay(1000);
  if (!digitalRead(COMP_STATUS)) blink(COMP_NOT_READY_1_ERROR);
  prepareADCForVCCmeasuring();
  attachInterruptToLineDown();
}

void onLineDown()
{
  detachInterrupt(digitalPinToInterrupt(LINE_STATUS));
  checkLine();
}

void onLineUp()
{
  detachInterrupt(digitalPinToInterrupt(LINE_STATUS));
  checkLine();
}

void loop()
{
  if (_isMainPower && _compIsOn && _lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_DEEP_SLEEP)
  {
    attachInterruptToLineDown();
    sleep();
  }
  else if (_isMainPower && _compIsOn && !_lineIsOk && (millis() - _eventTime) >= TIME_FOR_COMP_POWEROFF_AFTER_LINE_DOWN)
  {
    compPowerOff();
    waitForCompPowerOff();
    _eventTime = millis();
  }
  else if (_isMainPower && !_compIsOn && !_lineIsOk && (millis() - _eventTime) >= TIME_FOR_UPS_POWEROFF_AFTER_COMP)
  {
    switchUPS();
    attachInterruptToLineUp();
    sleep();
  }
  else if (!_isMainPower && !_compIsOn && _lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_COMP_POWERON)
  {
    switchUPS();
    waitForCompPowerOn();
    attachInterruptToLineDown();
    sleep();
  }
  else if (!_isMainPower && !_compIsOn && !_lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_DEEP_SLEEP)
  {
    attachInterruptToLineUp();
    sleep();
  }
  else if (_isMainPower && !_compIsOn && _lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_COMP_POWERON)
  {
    switchUPS();
    delay(3000);
    switchUPS();
    waitForCompPowerOn();
    attachInterruptToLineDown();
    sleep();
  }
  else if (!_isMainPower && _compIsOn)
  {
    _compIsOn = false;
  }
  checkLine();
}

void compPowerOff()
{
  digitalWrite(COMP_POWER_OFF_COMMAND, HIGH);
}

void compPowerOn()
{
  digitalWrite(COMP_POWER_OFF_COMMAND, LOW);
}

void setServoToZero()
{
  pinMode(SERVO_CONTROL, OUTPUT);
  _srv.attach(SERVO_CONTROL);
  _srv.write(0);
  delay(500);
  _srv.detach();
  pinMode(SERVO_CONTROL, INPUT);
  digitalWrite(SERVO_POWER, LOW);
}

void switchUPS()
{
  _isMainPower = digitalRead(MAIN_POWER_STATUS);
  if (_isMainPower) digitalWrite(BAT_5V_REGULATOR, HIGH);
  digitalWrite(SERVO_POWER, HIGH);
  delay(3);
  pinMode(SERVO_CONTROL, OUTPUT);
  _srv.attach(SERVO_CONTROL);
  _srv.write(20);
  delay(200);
  for (int angle = 22; angle < 43; angle++)
  {
    _srv.write(angle);
    delay(20);
  }
  delay(50);
  _srv.write(0);
  delay(500);
  _srv.detach();
  pinMode(SERVO_CONTROL, INPUT);
  digitalWrite(SERVO_POWER, LOW);
  unsigned long time = millis();
  while (_isMainPower == digitalRead(MAIN_POWER_STATUS))
  {
    if (millis() - time > 5000) //Основное питание с ардуины должно пропасть или появится не позже, чем через 5 секунд.
    {
      if (_isMainPower) digitalWrite(BAT_5V_REGULATOR, LOW);
      blink(SWITCHING_POWER_ERROR);
    }
  }
  _isMainPower = !_isMainPower;
  if (_isMainPower)
  {
    digitalWrite(BAT_5V_REGULATOR, LOW);
  }
  else
  {
    float VCC = getVCC();
    if (VCC < 4.2)
    {
      blink(LOW_BATTERY_ERROR);
    }
  }
}

void sleep()
{
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void waitForCompPowerOff()
{
  while (digitalRead(COMP_STATUS)) {}
  compPowerOn();
   _compIsOn = false;
}

void waitForCompPowerOn()
{
  while (!digitalRead(COMP_STATUS)) {}
   _compIsOn = true;
}

void attachInterruptToLineDown()
{
  attachInterrupt(digitalPinToInterrupt(LINE_STATUS), onLineDown, HIGH);
}
void attachInterruptToLineUp()
{
  attachInterrupt(digitalPinToInterrupt(LINE_STATUS), onLineUp, LOW);
}

void checkLine()
{
  bool currentValue = !digitalRead(LINE_STATUS);
  if (currentValue != _lineIsOk)
  {
    _lineIsOk = currentValue;
    _eventTime = millis();
  }
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

void prepareADCForVCCmeasuring()
{
  /*
    Настройка АЦП на считывание значения из внутреннего ИОН. При этом опорное напряжение - это напряжение питания.
    REFS1 = 0 и REFS0 = 1 - означает, что опорное напряжение - это напряжения питания. (стр. 217 datasheet)
    MUX[3..0] = 1110 - Подключает мультиплексор к внутреннему опорнику 1.1В. (стр. 218 datasheet)
    ADLAR = 0 - Означает, что первые 8 бит будут в ADCL и ещё 2 бита - в ADCH (стр. 219 datasheet)
  */
  ADMUX |= (0 << REFS1) | (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0) | (0 << ADLAR);
  ADCSRA |= (0 << ADEN) | (0 << ADATE);; //Пока выключаем АЦП. (ADATE = 0 - выключает постоянное считывание значение по триггеру)
}

void turnOnADC()
{
  ADCSRA |= (1 << ADEN); //Включили АЦП.
}

void turnOffADC()
{
  ADCSRA |= (0 << ADEN); //Выключили АЦП.
}

float getVCC()
{
  turnOnADC();
  delay(20);
  float VCC = getVCC_common();
  turnOffADC();
  return VCC;
}

float getVCC_common()
{
  int n = 3; //Количество преобразований.
  int ADC_sum = 0;
  for (int i = 0; i < n; i++)
  {
    ADCSRA |= (1 << ADSC) ;  //Начали преобразование. 
    while (ADCSRA & (1 << ADSC)) {} //Ждём пока не закончится преобразование (тогда ADSC снова станет нулём)
    ADC_sum += ADC;
  }
  return INTERNAL_REF_REAL_VOLTAGE * 1023.0 / ((float)ADC_sum / (float)n);
}