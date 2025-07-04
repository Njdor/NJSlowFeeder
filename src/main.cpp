#include <Arduino.h>
#include <Button.h> //uh, buttons
#include "driver/rtc_io.h" //deepsleep
#include "LoadCell.h" //loadcell setting
#include "Motor.h" //Motor setting
#include "Board.h" //Board setting

//deepsleep/wake defines, using EXT0 cause internal pullup power domain is needed.
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_5     // Only RTC IO are allowed

Board board;

void setup() {
    board.setup();
}

unsigned long buttonHighStartTime = 0;
unsigned long buttonHighDuration = 0;
// bool buttonWasHigh = false;

void loop() {
    if (board.shouldSleep()) {
        board.enterDeepSleep();
    }

    //check button states
    board.updateButtons();
    
    //button status evaluations
    board.handleButtonAction();
    board.processFeedingCycle();

    delay(10);
}