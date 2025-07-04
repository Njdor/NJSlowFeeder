#include "Motor.h"

Motor::Motor(int in1Pin, int in2Pin)
    : in1Pin(in1Pin), in2Pin(in2Pin) {}

void Motor::setup() {
	rtc_gpio_hold_dis((gpio_num_t)in2Pin);  
	pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);
    analogWriteFrequency(motorPWMFrequency);
    analogWrite(in2Pin, 0);
	setVoltage(in1Pin, 0);  
    motorVoltage = 0; 
	reset();
}

void Motor::reset() {
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
		// Serial.println("Motor stop: max time reached");
		return true; 
	}
	return false;
}

void Motor::setVoltage(int driverPin, float newVoltage, bool forceSet) {
	if (!forceSet && lastMotorUpdate == 0) {
		lastMotorUpdate = millis();
		return;
	}
	else if (!forceSet && (millis() - lastMotorUpdate < motorUpdateInterval)) {
		return;
	}
	newVoltage = constrain(newVoltage, 0, motorMaxVoltage);
	int setpoint = (int)((newVoltage / motorMaxVoltage) * 255);
	analogWrite(driverPin, setpoint);
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
	setVoltage(in1Pin, 0.3, true);
	delay(duration);
	setVoltage(in1Pin, 0, true);
	analogWriteFrequency(motorPWMFrequency);
}