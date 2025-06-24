#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "esp_adc_cal.h"

enum BatteryLevel {
    BATTERY_CRITICAL = 0,
    BATTERY_LOW      = 1,
    BATTERY_MEDIUM   = 2,
    BATTERY_HIGH     = 3
};

class Battery {
public:
    Battery(adc1_channel_t channel, adc_unit_t adcUnit);

    void setup();
    int getBatteryPercentage() const;  // returns 0 - 100%
    int getBatteryLevel() const;       // returns e.g., Battery_LOW

private:
    adc_unit_t adcUnit;
    adc1_channel_t adcChannel;
    esp_adc_cal_characteristics_t adcChars;

    const float fullVoltage = 4.2;
    const float emptyVoltage = 3.0;
    const float R1 = 22.0;                   // kohm
    const float R2 = 10.0;                   // kohm
    const float voltageDividerRatio = (R1 + R2) / R2; 

    const int batteryModerateThreshold = 70;     // percentage 
    const int batteryWarningThreshold = 30;     // percentage
    const int batteryCriticalThreshold = 7;     // percentage

    float readBatteryVoltage() const;  // returns voltage in V
};

#endif