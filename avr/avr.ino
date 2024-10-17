#include <LowPower.h>
#include <Servo.h>

#define LINE_STATUS 2
#define LED 13
#define UPS_POWER 7
#define BAT_POWER 8
#define SERVO_POWER 9
#define SERVO_CONTROL 10

#define COMP_NOT_READY_1_ERROR 500
#define COMP_NOT_READY_2_ERROR 1000
#define LOW_BATTERY_ERROR 3000

#define TIME_FOR_COMP_POWEROFF 60000
#define TIME_FOR_UPS_POWEROFF_AFTER_COMP 60000
#define TIME_BEFORE_COMP_POWERON 30000

volatile unsigned long _eventTime;
bool _compIsOn = true;
Servo _srv;

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(UPS_POWER, OUTPUT);
  pinMode(BAT_POWER, OUTPUT);
  pinMode(LINE_STATUS, INPUT);
  pinMode(SERVO_POWER, OUTPUT);
  digitalWrite(SERVO_POWER, LOW);
  pinMode(SERVO_CONTROL, INPUT);

  switchToMainPower();
  if (!lineIsOk()) blink(-1);
  Serial.begin(9600);
  Serial.println("status");
  delay(1000);
  if (!Serial.available()) blink(COMP_NOT_READY_1_ERROR);
  if (Serial.readString() != "ok") blink(COMP_NOT_READY_2_ERROR);

  prepareADCForVCCmeasuring();
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
  digitalWrite(SERVO_POWER, HIGH);
  delay(3);
  pinMode(SERVO_CONTROL, OUTPUT);
  _srv.attach(SERVO_CONTROL);
  _srv.write(31);
  delay(20);
  for (int angle = 32; angle < 43; angle++)
  {
    _srv.write(angle);
    delay(20);
  }
  delay(100);
  _srv.write(0);
  delay(500);
  _srv.detach();
  delay(100);
  digitalWrite(SERVO_POWER, LOW);
  delay(3);
  pinMode(SERVO_CONTROL, INPUT);
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
  digitalWrite(BAT_POWER, LOW);
  delay(2);
  digitalWrite(UPS_POWER, HIGH);
}

void switchToBatteryPower()
{
  digitalWrite(UPS_POWER, LOW);
  delay(2);
  digitalWrite(BAT_POWER, HIGH);
  if (getVCC() < 4.7) blink(LOW_BATTERY_ERROR);
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

float getVCC()
{
  int n = 3; //Количество преобразований.
  ADCSRA |= (1 << ADEN); //Включили АЦП.
  int ADC_sum = 0;
  for (int i = 0; i < n; i++)
  {
    ADCSRA |= (1 << ADSC) ;  //Начали преобразование. 
    while (ADCSRA & (1 << ADSC)) {} //Ждём пока не закончится преобразование (тогда ADSC снова станет нулём)
    ADC_sum += ADC;
  }
  ADCSRA |= (0 << ADEN); //Выключили АЦП.
  return 1.1 * 1023.0 / ((float)ADC_sum / (float)n);
}