#include <LowPower.h>
#include <Servo.h>

#define INTERNAL_REF_REAL_VOLTAGE 1.1
#define SERVO_ANGLE 50

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
#define LINE_IS_DOWN_ERROR 100

#define TIME_FOR_COMP_POWEROFF_AFTER_LINE_DOWN 10000
#define TIME_FOR_UPS_POWEROFF_AFTER_COMP 10000
#define TIME_BEFORE_COMP_POWERON 10000
#define TIME_BEFORE_DEEP_SLEEP 10000

unsigned long _eventTime = 0;
bool _compIsOn = true;
bool _isMainPower = true;
Servo _srv;
bool _lineIsOk = true;
bool _needSleep = false;
volatile bool _needCheckLine = true;

void setup()
{
  pinMode(LINE_STATUS, INPUT);
  pinMode(MAIN_POWER_STATUS, INPUT);
  pinMode(COMP_STATUS, INPUT);

  pinMode(LED, OUTPUT);
  pinMode(SERVO_POWER, OUTPUT);
  pinMode(COMP_POWER_OFF_COMMAND, OUTPUT);
  pinMode(BAT_5V_REGULATOR, OUTPUT);

  digitalWrite(LED, HIGH);
  digitalWrite(BAT_5V_REGULATOR, HIGH);

  rotateServo(0);
  compPowerOn();
  if (!digitalRead(MAIN_POWER_STATUS)) blink(SWITCHING_POWER_ERROR);
  digitalWrite(BAT_5V_REGULATOR, LOW);
  prepareADCForVCCmeasuring();
  delay(3000); //На зарядку конденсатора перед первой проверкой линии
  checkLine();
  if (!_lineIsOk) blink(LINE_IS_DOWN_ERROR);
  if (!digitalRead(COMP_STATUS)) blink(COMP_NOT_READY_1_ERROR);
}

void onLineDown()
{
  detachInterrupt(digitalPinToInterrupt(LINE_STATUS));
  _needCheckLine = true;
  digitalWrite(LED, HIGH);
}

void onLineUp()
{
  detachInterrupt(digitalPinToInterrupt(LINE_STATUS));
  _needCheckLine = true;
  digitalWrite(LED, HIGH);
}

void loop()
{
  if (_isMainPower && _compIsOn && _lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_DEEP_SLEEP && !_needCheckLine)
  {
    _needSleep = true;
  }
  else if (_isMainPower && _compIsOn && !_lineIsOk && (millis() - _eventTime) >= TIME_FOR_COMP_POWEROFF_AFTER_LINE_DOWN  && !_needCheckLine)
  {
    compPowerOff();
    waitForCompPowerOff();
    _eventTime = millis();
  }
  else if (_isMainPower && !_compIsOn && !_lineIsOk && (millis() - _eventTime) >= TIME_FOR_UPS_POWEROFF_AFTER_COMP && !_needCheckLine)
  {
    switchUPS();
    _needSleep = true;
  }
  else if (!_isMainPower && !_compIsOn && _lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_COMP_POWERON && !_needCheckLine)
  {
    switchUPS();
    waitForCompPowerOn();
    _needSleep = true;
  }
  else if (!_isMainPower && !_compIsOn && !_lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_DEEP_SLEEP && !_needCheckLine)
  {
    _needSleep = true;
  }
  else if (_isMainPower && !_compIsOn && _lineIsOk && (millis() - _eventTime) >= TIME_BEFORE_COMP_POWERON && !_needCheckLine)
  {
    switchUPS();
    delay(3000);
    switchUPS();
    waitForCompPowerOn();
    _needSleep = true;
  }
  else if (!_isMainPower && _compIsOn && !_needCheckLine)
  {
    _compIsOn = false;
  }
  checkLine();
  if (_needSleep)
  {
    _needSleep = false;
    sleep();
  }
}

void compPowerOff()
{
  digitalWrite(COMP_POWER_OFF_COMMAND, HIGH);
}

void compPowerOn()
{
  digitalWrite(COMP_POWER_OFF_COMMAND, LOW);
}

void rotateServo(int angle)
{
  pinMode(SERVO_CONTROL, OUTPUT);
  digitalWrite(SERVO_POWER, HIGH);
  delay(3);
  _srv.attach(SERVO_CONTROL);
  _srv.write(angle);
  delay(500);
  if (angle > 0)
  {
    _srv.write(0);
    delay(500);
  }
  _srv.detach();
  digitalWrite(SERVO_POWER, LOW);
  pinMode(SERVO_CONTROL, INPUT);
}

void switchUPS()
{
  _isMainPower = digitalRead(MAIN_POWER_STATUS);
  if (_isMainPower) digitalWrite(BAT_5V_REGULATOR, HIGH);
  rotateServo(SERVO_ANGLE);
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
  digitalWrite(LED, LOW);
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
  if (_needCheckLine)
  {
    _needCheckLine = false;
    delay(50);
    bool currentValue = !digitalRead(LINE_STATUS);
    if (currentValue != _lineIsOk)
    {
      _lineIsOk = currentValue;
      _eventTime = millis();
      _needSleep = false;
    }
    if (_lineIsOk)
    {
      attachInterruptToLineDown();
    }
    else
    {
      attachInterruptToLineUp();
    }
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