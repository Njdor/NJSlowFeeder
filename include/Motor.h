#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include "Speaker.h"
#include "driver/rtc_io.h"

extern RTC_DATA_ATTR float rtcMotorVoltage;

class Motor : public Speaker {
public:
	Motor(int pwmPin, int directionPin);

    void setup();
    void reset();
    void makeSound(int frequency, int duration) override;
    float getVoltage() const;
    float getMinVoltage() const;
    float getMaxVoltage() const;
    float getVoltageStep() const;
    void setVoltage(float newVoltage, bool forceSet=false);
    void setMotorStartTime();
    bool shouldStop() const;

private:
    int pwmPin;
    int directionPin;

	unsigned long minMotorRunTime = 3000; 
	const unsigned long maxMotorRunTime = 180000;
	unsigned long motorStartTime = 0;

	unsigned long lastMotorUpdate = 0;
	const unsigned long motorUpdateInterval = 500; //milliseconds, controls how frequently the motor voltage gets updated

    float motorVoltage;
	float motorMinVoltage = 2.5; //voltage the system will default to when clicking or holding up. Use 1.5V for 1000Hz analog freq, 2.7 for 20000Hz.
	float motorMaxVoltage = 3.3; //PWM Logic Level
	float motorVoltageStep = 0.2; //voltage the motor will step up per MotorUpdateTime interval when a button is held  Use 0.2 for 1000Hz, 0.1 for 20000Hz
	int motorPWMFrequency = 20000; //motor PWM frequency, 20000 you can't hear, 1000 has more granular range.
};

#endif