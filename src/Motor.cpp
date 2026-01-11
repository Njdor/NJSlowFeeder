#include "Motor.h"
// Preserve motor speed after deep sleep
RTC_DATA_ATTR float rtcMotorVoltage = -1.0f;

Motor::Motor(int pwmPin, int directionPin)
    : pwmPin(pwmPin), directionPin(directionPin) {}

void Motor::setup() {
	rtc_gpio_hold_dis((gpio_num_t)directionPin);
	rtc_gpio_hold_dis((gpio_num_t)pwmPin);  
	pinMode(pwmPin, OUTPUT);
    pinMode(directionPin, OUTPUT);
    analogWriteFrequency(motorPWMFrequency);
	analogWrite(pwmPin, 0);
    analogWrite(directionPin, 0);
	reset();
}

void Motor::reset() {
	setVoltage(0, true);
	motorStartTime = 0;
}

void Motor::setMotorStartTime() {
	motorStartTime = millis();
}

bool Motor::shouldStop() const {
	unsigned long elapsed = millis() - motorStartTime;
	if (elapsed < minMotorRunTime){
		return false;
	}
	else if (elapsed > maxMotorRunTime) { 
		Serial.println("Motor stop: max time reached");
		return true; 
	}
	return false;
}

void Motor::setVoltage(float newVoltage, bool forceSet) {
	if (!forceSet && lastMotorUpdate == 0) {
		lastMotorUpdate = millis();
		return;
	}
	else if (!forceSet && (millis() - lastMotorUpdate < motorUpdateInterval)) {
		return;
	}
	newVoltage = constrain(newVoltage, 0, motorMaxVoltage);
	int setpoint = (int)((newVoltage / motorMaxVoltage) * 255);
	analogWrite(pwmPin, setpoint);
	motorVoltage = newVoltage;
	lastMotorUpdate = millis();
}

float Motor::getVoltage() const {
    return motorVoltage;
}

float Motor::getMinVoltage() const {
    return motorMinVoltage;
}

float Motor::getMaxVoltage() const {
    return motorMaxVoltage;
}

float Motor::getVoltageStep() const {
    return motorVoltageStep;
}

void Motor::makeSound(int frequency, int duration) {
	// Serial.println("Making Noise");
	analogWriteFrequency(frequency);
	setVoltage(0.3, true);
	delay(duration);
	setVoltage(0, true);
	analogWriteFrequency(motorPWMFrequency);
}