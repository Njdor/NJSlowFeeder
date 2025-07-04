#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include "Speaker.h"  // Base class
#include "driver/rtc_io.h"

class Buzzer : public Speaker {
public:
    Buzzer(int pin, int channel = 0);
    void setup();                    
    void makeSound(int frequency, int duration) override;

private:
    int buzzerPin;
    int buzzerChannel;
};

#endif // BUZZER_H