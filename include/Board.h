#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <Button.h>
#include "LoadCell.h"
#include "Motor.h"
#include "Buzzer.h"
#include "Battery.h"
#include "driver/rtc_io.h"

#define HAS_LOADCELL       true
#define HAS_BATTERYMONITOR true
#define HAS_BUZZER         true
#define CALIBRATION_FACTOR 2300.0f

enum ButtonStatus {
    BUTTON_IDLE         = 0,
    BUTTON_HOLD         = 1,
    BUTTON_CLICK        = 2,
    BUTTON_DOUBLE_CLICK = 3,
};

class Board {
public:
    Board();
    void setup();
    void updateButtons();
    bool shouldSleep();  
    void enterDeepSleep();
    void handleButtonAction();
    void processFeedingCycle();

private:
    // GPIO Pins
    static const int IN1MotorPin               = 44; //change to 44 if motor runs in reverse
    static const int IN2MotorPin               = 7; //change to 7 if motor runs in reverse
    static const int buttonUpPin               = 5; //labeled as D4 on the silkscreen for xiao
    static const int buttonDownPin             = 6; //labeled as D5 on the slikscreen for xiao
    static const int HX_DOUT                   = 9; //hx711, labeled as D10 on the silkscreen for xiao
    static const int HX_CLK                    = 8; //hx711, labeled as D9 on the silkscreen for xiao
    static const int batteryPin                = A2; // ADC input from voltage divider
    static const int buzzerPin                 = 4;
    static const gpio_num_t HX711CLK_GPIO      = GPIO_NUM_8;
    // static const gpio_num_t IN1_GPIO           = GPIO_NUM_7;
    // static const gpio_num_t IN2_GPIO           = GPIO_NUM_44;  //non-rtc I/O, default is OUTPUT with Pullup Resistor enabled.
    static const gpio_num_t IN1_GPIO           = GPIO_NUM_44; //non-rtc I/O, default is OUTPUT with Pullup Resistor enabled.
    static const gpio_num_t IN2_GPIO           = GPIO_NUM_7;  
    static const gpio_num_t BUTTON_UP_GPIO     = GPIO_NUM_5;
    static const gpio_num_t BUTTON_DOWN_GPIO   = GPIO_NUM_6;
    static const gpio_num_t BATTERYPIN_GPIO    = GPIO_NUM_3;
    static const gpio_num_t BUZZER_GPIO        = GPIO_NUM_4;
    static const adc1_channel_t batteryChannel = ADC1_CHANNEL_2;
    static const adc_unit_t batteryAdcUnit     = ADC_UNIT_1;

    // Components
    Motor motor;
    Buzzer buzzer;
    LoadCell loadCell;
    Battery battery;
    Button buttonUp;
    Button buttonDown;
    Speaker* speakerPtr = nullptr;

    bool firstUpPress = true;

    // Timing tracking
    unsigned long lastMotorActiveTime = 0;
    unsigned long lastButtonActiveTime = 0;
    
    // loadCell
    unsigned long delayStartTime = 0;
    bool waitingAfterClick = false;
    const unsigned long delayAfterClick = 1000;

    // batteryMonitor
    int batteryLevel;

    // Double click tracking
    bool pendingClickUp = false;
    bool pendingClickDown = false;
	unsigned long clickUpStartTime = 0;
    unsigned long clickDownStartTime = 0; 
    const unsigned long doubleClickInterval = 400;

    // Config
    const int sleepTimeoutTime = 30000; // inactive time before system goes to sleep

    // Internal callbacks
    static void onRelease(Button& button);
    static void onHold(Button& button);

    // Buttons
    void handleUpClick();
    void handleDoubleClick(Button& button, bool& pendingClick, unsigned long& clickStartTime);

    // Chimes
    void playBatteryLevelChime(Speaker* speaker);
    void playBatteryCriticalChime(Speaker* speaker);
    void playStartupChime(Speaker* speaker);
    void playDeepSleepChime(Speaker* speaker);
    void playTareChime(Speaker* speaker);
    void printWakeupReason() const;

    bool shouldStopMotor();
    void resetSystem();
    
    // Power saving
    void configureRtcPin(gpio_num_t pin, 
                        rtc_gpio_mode_t mode=RTC_GPIO_MODE_DISABLED, 
                        bool enablePullup=false, 
                        bool enablePulldown=false);
    
    void isolateRtcPin(gpio_num_t pin);
};

#endif // BOARD_H
