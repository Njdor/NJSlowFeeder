#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "esp_adc_cal.h"
#include "driver/rtc_io.h"
#include <math.h>

enum BatteryLevel {
    BATTERY_CRITICAL = 0,
    BATTERY_LOW      = 1,
    BATTERY_MEDIUM   = 2,
    BATTERY_HIGH     = 3
};

class Battery {
public:
    Battery(gpio_num_t battery_gpio, adc1_channel_t channel, adc_unit_t adcUnit);

    void setup();
    int getBatteryLevel() const;                // returns e.g., Battery_LOW

private:
	gpio_num_t battery_gpio;
    adc_unit_t adcUnit;
    adc1_channel_t adcChannel;
    esp_adc_cal_characteristics_t adcChars;

    const float fullVoltage = 4.2f;
    const float emptyVoltage = 3.0f;
    const float R1 = 22.0f;                      // kohm
    const float R2 = 10.0f;                      // kohm
    const float voltageDividerRatio = (R1 + R2) / R2; 

    const int numReading = 10;                  // num readings to average for a accurate measurement
    const int numCalibrations = 10;             // num of times to sample voltage to calibrate voltage limits

    const int batteryModerateThreshold = 70;    // percentage 
    const int batteryWarningThreshold = 30;     // percentage
    const int batteryCriticalThreshold = 7;     // percentage

    const float voltageCalibOffset = 0.1f;
    const float voltageCalibEpsilon = 0.01f;
    const float learningRate = 0.3f;

    int getBatteryPercentage() const;           // returns 0 - 100%
    float readRawVoltage() const;               // returns voltage in V
    float getStableVoltage() const;             // returns voltage in V
    void maybeUpdateFullVoltage(float voltage);
    void maybeUpdateEmptyVoltage(float voltage);
    void calibrateVoltageLimits();
    float voltageToPercentage(float voltage) const;
};

#endif