#include <LowPower.h>
#include <Servo.h>

#define INTERNAL_REF_REAL_VOLTAGE 1.1

#define LINE_STATUS 2
#define LED 13
#define UPS_POWER_STATUS 7
#define BAT_5V_REGULATOR 6
#define SERVO_POWER 9
#define SERVO_CONTROL 10
#define COMP_POWER_OFF_COMMAND 8
#define COMP_STATUS 11
#define MAIN_POWER_STATUS 12

#define COMP_NOT_READY_1_ERROR 500
#define LOW_BATTERY_ERROR 3000
#define SWITCHING_POWER_ERROR 1000

#define TIME_FOR_COMP_POWEROFF_AFTER_LINE_DOWN 60000
#define TIME_FOR_UPS_POWEROFF_AFTER_COMP 30000
#define TIME_BEFORE_COMP_POWERON 30000
#define TIME_BEFORE_DEEP_SLEEP 30000

volatile unsigned long _eventTime;
bool _compIsOn = true;
bool _isMainPower = true;
Servo _srv;
volatile bool _lineIsOk = true;

void setup()
{
	pinMode(LED, OUTPUT);
	digitalWrite(LED, LOW);
	pinMode(UPS_POWER_STATUS, INPUT);
	pinMode(LINE_STATUS, INPUT);
	pinMode(SERVO_POWER, OUTPUT);
	digitalWrite(SERVO_POWER, LOW);
	pinMode(SERVO_CONTROL, INPUT);
	pinMode(COMP_POWER_OFF_COMMAND, OUTPUT);
	pinMode(COMP_STATUS, INPUT);
	pinMode(MAIN_POWER_STATUS, INPUT);

	compPowerOn();
	checkLine();
	if (!_lineIsOk) blink(-1);
	delay(1000);
	if (!digitalRead(MAIN_POWER_STATUS)) blink(SWITCHING_POWER_ERROR);
	if (!digitalRead(COMP_STATUS)) blink(COMP_NOT_READY_1_ERROR);
	prepareADCForVCCmeasuring();
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
		_compIsOn = false;
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
		compPowerOn();
		waitForCompPowerOn();
		_compIsOn = true;
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
		compPowerOn();
		waitForCompPowerOn();
		_compIsOn = true;
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

void switchUPS()
{
	_isMainPower = digitalRead(MAIN_POWER_STATUS);
	if (_isMainPower) digitalWrite(BAT_5V_REGULATOR, HIGH);
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
		if (VCC < 4.6)
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
}

void waitForCompPowerOn()
{
	while (!digitalRead(COMP_STATUS)) {}
}

void attachInterruptToLineDown()
{
	attachInterrupt(digitalPinToInterrupt(LINE_STATUS), onLineDown, FALLING);
}
void attachInterruptToLineUp()
{
	attachInterrupt(digitalPinToInterrupt(LINE_STATUS), onLineDown, RISING);
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
		ADCSRA |= (1 << ADSC) ;	//Начали преобразование. 
		while (ADCSRA & (1 << ADSC)) {} //Ждём пока не закончится преобразование (тогда ADSC снова станет нулём)
		ADC_sum += ADC;
	}
	return INTERNAL_REF_REAL_VOLTAGE * 1023.0 / ((float)ADC_sum / (float)n);
}