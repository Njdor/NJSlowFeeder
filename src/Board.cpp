#include "Board.h"
#include <Arduino.h>
// #include "driver/rtc_io.h"

// Preserve motor speed after deep sleep
RTC_DATA_ATTR float rtcMotorVoltage = -1.0f;

Board::Board()
    : motor(IN1MotorPin, IN2MotorPin),
    buzzer(buzzerPin),
    loadCell(HX_DOUT, HX_CLK, CALIBRATION_FACTOR),
    battery(batteryChannel, batteryAdcUnit),
    buttonUp(buttonUpPin, BUTTON_PULLDOWN, true, 50),
	buttonDown(buttonDownPin, BUTTON_PULLDOWN, true, 50) {}

void Board::setup() {
    Serial.begin(115200);
    delay(2500);
    Serial.println("Starting up...");

    motor.setup();
    Serial.println("Setting up motor");

    rtcMotorVoltage = rtcMotorVoltage < 0.0f ? motor.getMinVoltage() : rtcMotorVoltage;

    if (HAS_LOADCELL) {
        loadCell.setup();
        Serial.println("Setting up loadCell");
    }
    else {
        Serial.println("Load Cell not detected");
    }

    if (HAS_BATTERYMONITOR) {
        battery.setup();
        batteryLevel = battery.getBatteryLevel();
        Serial.println("Battery monitor detected");
    }
    else {
        Serial.println("Battery monitor not detected");
    }

    if(HAS_BUZZER) {
        buzzer.setup();
        speakerPtr = &buzzer;
    }
    else {
        speakerPtr = &motor;
    }

    printWakeupReason();

    buttonUp.releaseHandler(onRelease);
    buttonUp.holdHandler(onHold, 1000);

    buttonDown.releaseHandler(onRelease);
    buttonDown.holdHandler(onHold, 1000);

    delay(1000);
    if (HAS_BATTERYMONITOR && (batteryLevel == BATTERY_CRITICAL)) {
        playBatteryCriticalChime(speakerPtr);
    }
    else {
        playStartupChime(speakerPtr);
    }

	// Refresh buttons
    buttonUp.process();
    buttonDown.process();
    buttonUp.buttonstatus = BUTTON_IDLE;
    buttonDown.buttonstatus = BUTTON_IDLE;
}

void Board::handleDoubleClick(Button& button, bool& pendingClick, unsigned long& clickStartTime) {
    unsigned long now = millis();

    if (button.buttonstatus == BUTTON_CLICK) {
        if (pendingClick && (now - clickStartTime <= doubleClickInterval)) {
            button.buttonstatus = BUTTON_DOUBLE_CLICK;
            pendingClick = false;
        } 
        else {
            // Temporarily assign to idle to wait for second click decision
            button.buttonstatus = BUTTON_IDLE;
            pendingClick = true;
            clickStartTime = now;
        }
    }

    if (pendingClick && (now - clickStartTime > doubleClickInterval)) {
        button.buttonstatus = BUTTON_CLICK;
        pendingClick = false;
    }
}

void Board::updateButtons() {
    // Update button response
    buttonUp.process();
    buttonDown.process();

    // Double-click detection 
    handleDoubleClick(buttonUp, pendingClickUp, clickUpStartTime);
    handleDoubleClick(buttonDown, pendingClickDown, clickDownStartTime);
    // Serial.print("Down button status: ");
    // Serial.println(buttonDown.buttonstatus);
}

void Board::playBatteryLevelChime(Speaker* speaker) {
    if (batteryLevel == BATTERY_CRITICAL) {
        playBatteryCriticalChime(speaker);
    }
    for (int i = 0; i < batteryLevel; ++i) {
        speaker->makeSound(800, 300);
        delay(150);
    }
}

void Board::playBatteryCriticalChime(Speaker* speaker) {
    speaker->makeSound(1000, 150);
    speaker->makeSound(800, 150);
    speaker->makeSound(600, 200);
}

void Board::playStartupChime(Speaker* speaker) {
    speaker->makeSound(1000, 150);
    speaker->makeSound(1300, 150);
    speaker->makeSound(1600, 150);
    speaker->makeSound(2000, 200);
}

void Board::playDeepSleepChime(Speaker* speaker) {
	speaker->makeSound(1800, 150);
    speaker->makeSound(1400, 150);
    speaker->makeSound(1000, 150);
    speaker->makeSound(600, 300);
}

bool Board::shouldSleep() {
    unsigned long now = millis();
    bool timeout = (now - lastMotorActiveTime > sleepTimeoutTime) && (now - lastButtonActiveTime > sleepTimeoutTime);
	return timeout || buttonDown.buttonstatus == BUTTON_DOUBLE_CLICK;
}

void Board::configureRtcPin(gpio_num_t pin, rtc_gpio_mode_t mode, bool enablePullup, bool enablePulldown) {
    rtc_gpio_init(pin);
    rtc_gpio_set_direction(pin, mode);

    if (enablePullup) {
        rtc_gpio_pullup_en(pin);
    } 
    else {
        rtc_gpio_pullup_dis(pin);
    }

    if (enablePulldown) {
        rtc_gpio_pulldown_en(pin);
    } 
    else {
        rtc_gpio_pulldown_dis(pin);
    }
}

void Board::enterDeepSleep() {
    delay(500);
    playDeepSleepChime(speakerPtr);

    if (HAS_LOADCELL){
        // RTC hx711 CLK pin high power saving
        configureRtcPin(HX711CLK_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY, true, false);

        pinMode(HX_CLK, OUTPUT);
        digitalWrite(HX_CLK, LOW);
        delayMicroseconds(1);
        digitalWrite(HX_CLK, HIGH);
        delayMicroseconds(100);
        pinMode(HX_DOUT, INPUT);
    }
    
    // RTC motor pin power saving
    configureRtcPin(IN1_GPIO, RTC_GPIO_MODE_DISABLED, false, false);

    // Turn off motor pins
    pinMode(IN1MotorPin, OUTPUT);
    digitalWrite(IN1MotorPin, LOW);
    delay(10);
    pinMode(IN1MotorPin, INPUT);

    pinMode(IN2MotorPin, OUTPUT);
    digitalWrite(IN2MotorPin, LOW);
    delay(10);
    pinMode(IN2MotorPin, INPUT);

    // Turn batteryPin to input
    if (HAS_BATTERYMONITOR){
        configureRtcPin(BATTERYPIN_GPIO, RTC_GPIO_MODE_DISABLED, false, false);
        pinMode(batteryPin, INPUT);
    }

    if (HAS_BUZZER) {
        configureRtcPin(BUZZER_GPIO, RTC_GPIO_MODE_DISABLED, false, false);
        pinMode(buzzerPin, INPUT);
    }
    // Put buttonDownPin to input with pullup to avoid floating
    pinMode(buttonDownPin, INPUT_PULLUP);

    // Wakeup config
    configureRtcPin(WAKEUP_GPIO, RTC_GPIO_MODE_INPUT_ONLY, false, true);
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, HIGH);

    // Report
    Serial.print("System idle for (s): ");
    Serial.println(min((millis() - lastMotorActiveTime), (millis() - lastButtonActiveTime)) / 1000);

    Serial.println("Going to deep sleep");
    Serial.flush();
    delay(50);
    esp_deep_sleep_start();
}

void Board::printWakeupReason() const {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
        default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
}

void Board::onRelease(Button& button) {
    // Serial.print("onRelease: ");
    // Serial.println(button.pin);
	switch (button.buttonstatus)
	{
		case BUTTON_IDLE:
			// Serial.print("Click: ");
			// Serial.println(b.pin);
			button.buttonstatus = BUTTON_CLICK;
			break;
		case BUTTON_HOLD:
			// Serial.print("HoldRelease: ");
			// Serial.println(b.pin);
			button.buttonstatus = BUTTON_IDLE;
		default:
			break;
    } 	
}

void Board::onHold(Button& button) {
    // Serial.print("onHold: ");
    // Serial.println(button.pin);
    button.buttonstatus = BUTTON_HOLD;
}

void Board::handleButtonAction() {
    if (HAS_BATTERYMONITOR && (batteryLevel == BATTERY_CRITICAL)) {
        if(buttonUp.buttonstatus != BUTTON_IDLE || (buttonDown.buttonstatus != BUTTON_IDLE && buttonDown.buttonstatus != BUTTON_DOUBLE_CLICK)) {
            playBatteryCriticalChime(speakerPtr);  
            buttonUp.buttonstatus = BUTTON_IDLE;
            buttonDown.buttonstatus = BUTTON_IDLE;
        }
        return;
    }

	switch (buttonUp.buttonstatus) {
        case BUTTON_HOLD:  
            // Serial.print("Holding Up; Current Voltage: ");
            // Serial.print("Holding Up; New Voltage: ");
            // Serial.println(constrain(currVoltage + motor.getVoltageStep(), 0, motor.getBusVoltage()), 2);
            lastButtonActiveTime = millis();
            if (motor.getVoltage() > 0) {
                motor.setVoltage(IN1MotorPin, motor.getVoltage() + motor.getVoltageStep());
                // rtcMotorVoltage = motor.getVoltage();
            }
            break;

        case BUTTON_CLICK:  // Click
            // loadCell.tare();
            // motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            // buttonUp.buttonstatus = 0;
            // break;
            lastButtonActiveTime = millis();
            if (!motor.started()) {
                motor.setMotorStartTime();
            }
            // Serial.print("Read rtc: ");
            // Serial.println(rtcMotorVoltage, 3);
            motor.setVoltage(IN1MotorPin, firstUpPress ? rtcMotorVoltage : motor.getMinVoltage());
            delayStartTime = millis();
            waitingAfterClick = true;
			buttonUp.buttonstatus = BUTTON_IDLE;
            firstUpPress = false;
            break;

		case BUTTON_DOUBLE_CLICK:
            lastButtonActiveTime = millis();
            if (!motor.started()) {
                motor.setMotorStartTime();
            }
			motor.setVoltage(IN1MotorPin, motor.getMaxVoltage(), true);
            delayStartTime = millis();
            waitingAfterClick = true;
			buttonUp.buttonstatus = BUTTON_IDLE;
			break;

        default:
            // No action needed
            break;
    }

    switch (buttonDown.buttonstatus) {
        case BUTTON_HOLD:  
            lastButtonActiveTime = millis();
            if (motor.getVoltage() > 0) {
                // ensure motor doesn't stop when holding down button
                float newVoltage = motor.getVoltage() - motor.getVoltageStep();
                newVoltage = max(newVoltage, motor.getMinVoltage());
                motor.setVoltage(IN1MotorPin, newVoltage);
                // rtcMotorVoltage = motor.getVoltage();
            }
            break;

        case BUTTON_CLICK:
            lastButtonActiveTime = millis();
            if (motor.getVoltage() > 0) {
                resetSystem();
            }
            else if (HAS_BATTERYMONITOR) {
                battery.getBatteryPercentage();
                playBatteryLevelChime(speakerPtr);
            }
            buttonDown.buttonstatus = BUTTON_IDLE;
            break;

        default:
            // No action needed
            break;
    }
}

bool Board::shouldStopMotor() {
    if (buttonUp.buttonstatus != BUTTON_IDLE || buttonDown.buttonstatus != BUTTON_IDLE){
        return false;
    }
    if (HAS_LOADCELL) {
        return motor.shouldStop() || loadCell.shouldStop();
    }
    return motor.shouldStop();
}

void Board::resetSystem() {
    firstUpPress = true;
    rtcMotorVoltage = motor.getVoltage();
    // Serial.print("Saved rtc: ");
    // Serial.println(rtcMotorVoltage, 3);
    motor.setVoltage(IN1MotorPin, 0);
    motor.reset();
    if (HAS_LOADCELL){
        loadCell.reset();
    }
}

void Board::processFeedingCycle() {
	if (HAS_LOADCELL && waitingAfterClick) {
		// delay for 1s after clicking button
		// to avoid flucuations in readings
		if(millis() - delayStartTime >= delayAfterClick) {
			// reset to wait afte click
			waitingAfterClick = false;
			delayStartTime = millis();
		}
	}
	else {
		if (motor.getVoltage() > 0) {
			lastMotorActiveTime = millis();
            if (HAS_LOADCELL){ loadCell.update(); }
			if (shouldStopMotor()) { resetSystem(); }
		}   
	}
}
